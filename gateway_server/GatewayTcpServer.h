#ifndef GATEWAY_TCP_SERVER_H
#define GATEWAY_TCP_SERVER_H

#include <mymuduo/net/TcpServer.h>
#include <mymuduo/net/EventLoop.h>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <string>

class GatewayTcpServer
{
public:
	GatewayTcpServer(EventLoop* loop, const std::string& ip, uint16_t port);

    // 启动服务
    void Start(int thread_num);

    // 提供给内部 RPC 调用的核心接口：精准推送消息给某个物理客户端
    bool PushMessageToClient(int32_t uid, uint32_t msg_type, const std::string& content);

private:
    // 底层网络回调
    void OnConnection(const std::shared_ptr<TcpConnection>& conn);
    void OnMessage(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer);

private:
    TcpServer server_;

    // 会话管理器 (封装在类内部，绝对安全)
    std::mutex session_mutex_;
    std::unordered_map<int32_t, std::shared_ptr<TcpConnection>> user_sessions_;
};

#endif