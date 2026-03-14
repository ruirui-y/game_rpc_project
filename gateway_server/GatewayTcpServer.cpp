#include "GatewayTcpServer.h"
#include <mymuduo/Log/Logger.h>
#include <any>
#include <arpa/inet.h>
using namespace std::placeholders;

GatewayTcpServer::GatewayTcpServer(EventLoop* loop, const std::string& ip, uint16_t port)
    : server_(loop, ip, port, 100)
{
    server_.SetConnectionCallback(std::bind(&GatewayTcpServer::OnConnection, this, _1));
    server_.SetMessageCallback(std::bind(&GatewayTcpServer::OnMessage, this, _1, _2));
}

void GatewayTcpServer::Start(int thread_num)
{
    server_.Start(thread_num);
}

bool GatewayTcpServer::PushMessageToClient(int32_t uid, uint32_t msg_type, const std::string& content)
{
    std::shared_ptr<TcpConnection> target_conn = nullptr;
    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        auto it = user_sessions_.find(uid);
        if (it != user_sessions_.end())
        {
            target_conn = it->second;
        }
    }

    if (target_conn)
    {
        target_conn->Send(content);
        return true;
    }



    return false;
}

void GatewayTcpServer::OnConnection(const std::shared_ptr<TcpConnection>& conn)
{
    // ¶Ï¿ªÁ¬½Ó£¬ÒÆ³ýÓ³Éä
    if (!conn->Connected())
    {
        if (conn->GetContext().has_value())
        {
            int32_t uid = std::any_cast<int32_t>(conn->GetContext());
            std::lock_guard<std::mutex> lock(session_mutex_);
            user_sessions_.erase(uid);
            LOG_INFO << "[Gateway] Player " << uid << " disconnected. Session removed.";
        }
    }
    else
    {
        LOG_INFO << "[Gateway] New external client connected!";
    }
}

void GatewayTcpServer::OnMessage(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer)
{
    while (buffer->ReadableBytes() >= 8)
    {
        uint32_t total_len = buffer->PeekInt32();


    }

}