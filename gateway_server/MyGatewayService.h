#ifndef MY_GATEWAY_SERVICE_H
#define MY_GATEWAY_SERVICE_H

#include "gateway.pb.h"
#include <mymuduo/Log/Logger.h>
#include <mymuduo/net/TcpConnection.h>
#include <unordered_map>
#include <mutex>

using namespace game::rpc;

class MyGatewayService : public game::rpc::GatewayService
{
public:
    virtual void PushMessage(::google::protobuf::RpcController* controller,
        const ::game::rpc::PushMessageRequest* request,
        ::game::rpc::PushMessageResponse* response,
        ::google::protobuf::Closure* done) override
    {
        int32_t uid = request->user_id();
        int32_t type = request->msg_type();
        std::string content = request->content();

        LOG_INFO << "Gateway received push request from backend! Target User: " << uid;

        // 【核心路由逻辑】
        // 未来这里会去一个类似 user_connections 的 Map 里找到 uid 对应的 TcpConnection
        // 然后调用 conn->Send(content); 把数据发给真正的手机/PC客户端。

        // 现阶段我们先打印出来，模拟推送成功
        LOG_INFO << "Pushing to Client ---> Type: " << type << ", Content: " << content;

        response->set_errcode(0);
        response->set_errmsg("Push success");

        done->Run();
    }

private:
    std::unordered_map<int32_t, std::shared_ptr<TcpConnection>> user_connections_;
};

#endif