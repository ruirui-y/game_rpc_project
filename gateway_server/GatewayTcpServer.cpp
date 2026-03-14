#include "GatewayTcpServer.h"
#include <mymuduo/Log/Logger.h>
#include <any>
#include <arpa/inet.h>
#include "client_gateway.pb.h"
#include "MsgID.h"
using namespace std::placeholders;

GatewayTcpServer::GatewayTcpServer(EventLoop* loop, const std::string& ip, uint16_t port)
    : server_(loop, ip, port, 100)
{
    server_.SetConnectionCallback(std::bind(&GatewayTcpServer::OnConnection, this, _1));
    server_.SetMessageCallback(std::bind(&GatewayTcpServer::OnMessage, this, _1, _2));

    // ================== 路由表注册 ==================
    RegisterHandler(game::net::MSG_LOGIN_REQ, std::bind(&GatewayTcpServer::HandleLoginReq, this, _1, _2));
    RegisterHandler(game::net::MSG_JOIN_MATCH_REQ, std::bind(&GatewayTcpServer::HandleJoinMatchReq, this, _1, _2));
}

void GatewayTcpServer::Start(int thread_num)
{
    server_.Start(thread_num);
}

void GatewayTcpServer::RegisterHandler(uint32_t msg_id, MsgHandler handler)
{
    msg_dispatcher_[msg_id] = handler;
}

void GatewayTcpServer::OnConnection(const std::shared_ptr<TcpConnection>& conn)
{
    // 断开连接，移除映射
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
    // len(4) + msg_id(4) + data
    while (buffer->ReadableBytes() >= 8)
    {
        uint32_t total_len = buffer->PeekInt32();
        if (buffer->ReadableBytes() >= total_len + 4)
        {
            uint32_t msg_id = buffer->RetrieveInt32();
            std::string pb_data = buffer->RetrieveAsString(total_len - 4);

            // ================== 路由表分发 ==================
            auto it = msg_dispatcher_.find(msg_id);
            if (it != msg_dispatcher_.end())
            {
                // 找到了对应的处理函数，直接把原始二进制流扔给它去解析！
                it->second(conn, pb_data);
            }
            else
            {
                LOG_ERROR << "[Gateway] Unregistered MsgID: " << msg_id << ". Dropping packet.";
            }
        }
        else
        {
            break;
        }
    }
}

// ================== 具体的业务处理 (Handler) ==================
void GatewayTcpServer::HandleLoginReq(const std::shared_ptr<TcpConnection>& conn, const std::string& pb_data)
{
    // 1. 反序列化外网请求
    game::client::ClientLoginRequest req;
    if (!req.ParseFromString(pb_data)) {
        LOG_ERROR << "[Gateway] Failed to parse ClientLoginRequest!";
        return;
    }

    LOG_INFO << "[Gateway] User attempting login with account: " << req.username();

    // TODO: 真正的业务逻辑应该是化身 RPC Client 去调用 LoginServer 进行验密
    // 这里我们依然先用 Mock 逻辑通过测试
    int32_t mock_uid = 1000;

    // 登录成功，绑定 Session
    conn->SetContext(mock_uid);
    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        user_sessions_[mock_uid] = conn;
    }

    // 2. 组装外网响应
    game::client::ClientLoginResponse resp;
    resp.set_errcode(0);
    resp.set_errmsg("Welcome to UE5 Server");
    resp.set_user_id(mock_uid);

    std::string resp_data;
    resp.SerializeToString(&resp_data);
    PushMessageToClient(mock_uid, game::net::MSG_LOGIN_RESP, resp_data);
}

void GatewayTcpServer::HandleJoinMatchReq(const std::shared_ptr<TcpConnection>& conn, const std::string& pb_data)
{
    // 安全校验：没登录就发匹配请求？直接断开连接防外挂！
    if (!conn->GetContext().has_value())
    {
        conn->ForceClose();
        return;
    }

    int32_t uid = std::any_cast<int32_t>(conn->GetContext());
    LOG_INFO << "[Gateway] Player " << uid << " wants to join match!";
}

// ================== 推送响应回包 ==================
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
        // 组装外网协议发给他 (长度 + MsgID + 数据)
        uint32_t res_len = 4 + content.size();
        uint32_t net_res_len = htonl(res_len);
        uint32_t net_msg_id = htonl(msg_type);

        std::string send_buf;
        send_buf.append((char*)&net_res_len, 4);
        send_buf.append((char*)&net_msg_id, 4);
        send_buf.append(content);

        target_conn->Send(send_buf);
        return true;
    }
    return false; // 玩家不在线
}