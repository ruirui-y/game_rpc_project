#include "RPCServer.h"
#include "rpcheader.pb.h"
#include <mymuduo/Log/Logger.h>

void RPCServer::RegisterService(google::protobuf::Service* service)
{
	services[service->GetDescriptor()->name()] = service;
}

void RPCServer::Run()
{
	TcpServer server(&event_loop_, ip_, port_, 10);
	server.SetMessageCallback(
		[this](const std::shared_ptr<TcpConnection>& conn, Buffer* buffer)
		{
			OnMessage(conn, buffer);
		});

	server.Start(0);
	event_loop_.Loop();
}

void RPCServer::OnMessage(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer)
{
    std::string recv_buf = buffer->RetrieveAllAsString();                                               

    // 1. 从字符流中读取前4个字节的内容
    uint32_t header_size = 0;                                                                           
    recv_buf.copy((char*)&header_size, 4, 0);                                                           

    // 2. 根据header_size读取数据头的原始字符流，反序列化数据
    std::string rpc_header_str = recv_buf.substr(4, header_size);                                       
    rpc::core::RpcHeader rpcHeader;                                                                     
    std::string service_name;                                                                           
    std::string method_name;                                                                            
    uint32_t method_index;                                                                              
    uint32_t args_size;                                                                                 

    if (rpcHeader.ParseFromString(rpc_header_str))                                                      
    {
        service_name = rpcHeader.service_name();                                                        
        method_name = rpcHeader.method_name();                                                          
        method_index = rpcHeader.method_index();
        args_size = rpcHeader.args_size();                                                              
    }
    else
    {
        LOG_ERROR << "rpc_header_str: " << rpc_header_str.c_str() << " parse error!";                                                                                                                                                                                                                                                                                                                                           // 记录解析失败
        return;
    }

    // 3. 获取rpc方法参数的二进制数据
    std::string args_str = recv_buf.substr(4 + header_size, args_size);                                 

    // 4. 获取service对象和method对象
    auto it = services.find(service_name);                                                              
    if (it == services.end())
    {
        LOG_ERROR << "%s is not exist! " << service_name.c_str();                                       
        return;
    }

    google::protobuf::Service* service = it->second;                                                    
    const google::protobuf::ServiceDescriptor* service_desc = service->GetDescriptor();                         
    const google::protobuf::MethodDescriptor* method = service_desc->method(method_index);              

    // 5. 生成rpc方法调用的请求request和响应response参数
    google::protobuf::Message* request = service->GetRequestPrototype(method).New();                    
    if (!request->ParseFromString(args_str))                                                            
    {
        LOG_ERROR << "request parse error, content: " << args_str.c_str();                              
        return;
    }
    google::protobuf::Message* response = service->GetResponsePrototype(method).New();                  

    // 6. 设置回调函数，将rpc响应发送给客户端
    google::protobuf::Closure* done = google::protobuf::NewCallback<RPCServer,
        const std::shared_ptr<TcpConnection>&,
        google::protobuf::Message*>
        (this, &RPCServer::SendRpcResponse, conn, response);

    // 7. 当前服务会根据method调用自己的rpc方法
    service->CallMethod(method, nullptr, request, response, done);
}

// 长度(4B) + 内容
void RPCServer::SendRpcResponse(const std::shared_ptr<TcpConnection>& conn, google::protobuf::Message* response)
{
    std::string response_str;
    if (response->SerializeToString(&response_str))
    {
        // --- 添加 4 字节长度报头 ---
        uint32_t response_size = response_str.size();
        std::string send_str;
        // 把长度信息以二进制形式放入前 4 个字节
        send_str.insert(0, std::string((char*)&response_size, 4));
        send_str += response_str;

        conn->Send(send_str);
    }
    else
    {
        LOG_ERROR << "serialize response_str error!";
    }
}