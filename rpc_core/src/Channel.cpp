#include "Channel.h"
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <mymuduo/Log/Logger.h>

#include "rpcheader.pb.h"
#include "ConnectionPool.h"

void Channel::CallMethod(const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const google::protobuf::Message* request,
    google::protobuf::Message* response,
    google::protobuf::Closure* done)
{
    // 1. 获取服务，方法名以及参数长度
    const google::protobuf::ServiceDescriptor* service_descrip = method->service();
    string service_name = service_descrip->name();
    string method_name = method->name();
    uint32_t args_size = 0;
    std::string args_str;
    if (request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }

    // 2. 构造请求头
    rpc::core::RpcHeader header;
    header.set_service_name(method->service()->name());
    header.set_method_name(method->name());
    header.set_method_index(method->index());
    header.set_args_size(args_size);

    // 3. 序列化请求头和请求数据
    std::string header_str;
    header.SerializeToString(&header_str);
    uint32_t header_size = header_str.size();
    uint32_t net_header_size = htonl(header_size);                                                      // 发送网络字节序

    std::string request_str;
    request->SerializeToString(&request_str);

    // 5. 数据整合
    std::string send_rpc_str;;                                                      
    send_rpc_str.insert(0, std::string((char*)&net_header_size, 4));
    send_rpc_str += header_str;
    send_rpc_str += request_str;

    // 6. 复用连接并发送数据
    int sockfd = ConnectionPool::GetInstance().GetConnection("127.0.0.1", 9090);
    if (-1 == send(sockfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))                               // 发送二进制流
    {
        close(sockfd);                                                                                  // 失败关闭
        if (controller) controller->SetFailed("send error! errno:" + std::to_string(errno));            // 记录失败
        return;
    }

    // 设置超时时间
    struct timeval timeout = { 5, 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));
    
    // 7. 接收响应头(4字节)
    uint32_t network_response_size = 0; // 用来存网络字节序的数据
    int offset = 0;
    while (offset < 4)
    {
        int n = recv(sockfd, (char*)&network_response_size + offset, 4 - offset, 0);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                if (controller) controller->SetFailed("RPC recv header timeout!");
            }
            else
            {
                if (controller) controller->SetFailed("recv header error! errno:" + std::to_string(errno));
            }
            close(sockfd);
            return;
        }
        else if (n == 0)
        {
            // 连接断开
            if (controller) controller->SetFailed("server connection closed unexpectedly while reading header!");
            close(sockfd);
            return;
        }
        offset += n;
    }

    // 转换成本地主机字节序
    uint32_t response_size = ntohl(network_response_size);

    if (response_size > 10 * 1024 * 1024)
    {
        if (controller) controller->SetFailed("response is too big! size: " + std::to_string(response_size));
        close(sockfd);
        return;
    }

    // 8. 接收响应体
    std::string response_data;
    response_data.resize(response_size);
    uint32_t total_recv = 0;

    while (total_recv < response_size)
    {
        // 往指定的位置写入指定大小的数据
        int n = recv(sockfd, &response_data[total_recv], response_size - total_recv, 0);

        if (n < 0)                                                                                                  // 发生错误或超时
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                if (controller) controller->SetFailed("RPC recv body timeout!");
            }
            else
            {
                if (controller) controller->SetFailed("recv body error! errno:" + std::to_string(errno));
            }
            close(sockfd);
            return;
        }
        else if (n == 0)                                                                                            // 连接断开
        {
            if (controller) controller->SetFailed("server connection closed unexpectedly while reading body!");
            close(sockfd);
            return;
        }
        total_recv += n;
    }

    // 9. 反序列化响应数据
    if (!response->ParseFromString(response_data))                                                                  // 解析并填充
    {
        close(sockfd);                                                                                              // 失败关闭
        if (controller) controller->SetFailed("parse error! response_str:" + response_data);                        // 解析失败
        return;
    }
}