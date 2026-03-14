#include "RPCServer.h"
#include "rpcheader.pb.h"
#include <mymuduo/Log/Logger.h>

void RPCServer::RegisterService(google::protobuf::Service* service)
{
	services[service->GetDescriptor()->name()] = service;
}

void RPCServer::Run()
{
	TcpServer server(&event_loop_, ip_, port_, 1000);
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
    while (buffer->ReadableBytes() >= 4)
    {
        // 1. 偷看前4个字节（不从buffer剔除），获取 header_size
        uint32_t header_size = buffer->PeekInt32();

        // 2. 检查：连 rpc_header 都还没收全？
        if (buffer->ReadableBytes() < 4 + header_size) 
        {
            break;                                                                                  // 半包，跳出循环等下一次 epoll 唤醒
        }

        // 3. 偷拿 header 数据进行反序列化（依然不剔除 buffer，因为 args 还没确认收全）
        std::string rpc_header_str(buffer->peek() + 4, header_size);
        rpc::core::RpcHeader rpcHeader;
        if (!rpcHeader.ParseFromString(rpc_header_str)) {
            LOG_ERROR << "rpc header parse error!";
            return;
        }

        uint32_t args_size = rpcHeader.args_size();

        // 4. 检查：真正的请求体 (args) 收全了吗？
        if (buffer->ReadableBytes() < 4 + header_size + args_size) 
        {
            break;                                                                                  // 半包，跳出循环继续等
        }

        // ============ 走到这里，说明一个完整的 RPC 请求终于拼齐了！============

        // 5. 正式把这个完整包从 Buffer 里剥离出来
        buffer->retrieve(4);                                                                        // 剥离长度信息
        buffer->retrieve(header_size);                                                              // 剥离 header
        std::string args_str = buffer->RetrieveAsString(args_size);                                 // 剥离并获取真正的参数数据

        // 6. 后续的业务分发逻辑 (找服务、生成 request、CallMethod)
        std::string service_name = rpcHeader.service_name();
        auto it = services.find(service_name);
        if (it == services.end()) {
            LOG_ERROR << service_name << " is not exist!";
            return;
        }

        google::protobuf::Service* service = it->second;
        const google::protobuf::MethodDescriptor* method = service->GetDescriptor()->method(rpcHeader.method_index());

        google::protobuf::Message* request = service->GetRequestPrototype(method).New();
        if (!request->ParseFromString(args_str)) 
        {
            LOG_ERROR << "request parse error!";
            delete request;                                                                         // 防止解析失败时内存泄漏
            return;
        }
        google::protobuf::Message* response = service->GetResponsePrototype(method).New();

        google::protobuf::Closure* done = google::protobuf::NewCallback<RPCServer,
            const std::shared_ptr<TcpConnection>&, google::protobuf::Message*>
            (this, &RPCServer::SendRpcResponse, conn, response);

        service->CallMethod(method, nullptr, request, response, done);

        // 释放堆空间
        if (request) delete request;
        if (response) delete response;
    }
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

        // 转换成网络字节序
        uint32_t net_response_size = htonl(response_size);
        send_str.insert(0, std::string((char*)&net_response_size, 4));

        // 把长度信息以二进制形式放入前 4 个字节
        send_str += response_str;

        conn->Send(send_str);
    }
    else
    {
        LOG_ERROR << "serialize response_str error!";
    }
}