#include "Channel.h"
#include <string>
#include "rpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <mymuduo/Log/Logger.h>

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
    int param_size = request->ByteSize();

    // 2. 构造请求头
    rpc::core::RpcHeader header;
    header.set_service_name(method->service()->name());
    header.set_method_name(method->name());
    header.set_args_size(param_size);

    // 3. 序列化请求头
    std::string header_str;
    header.SerializeToString(&header_str);
    uint32_t header_size = header_str.size();

    // 4. 序列化请求数据
    std::string request_str;
    request->SerializeToString(&request_str);

    // 5. 发送数据
    int len = sizeof(uint32_t) + header_str.size() + request_str.size();
    char buf[len];
    memcpy(buf, &header_size, sizeof(int));
    memcpy(buf, header_str.c_str(), header_str.size());
    memcpy(buf + header_str.size(), request_str.c_str(), request_str.size());

    // 复用连接
}