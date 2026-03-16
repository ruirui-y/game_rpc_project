#include "MockClient.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// 引入你的底层日志库
#include <mymuduo/Log/Logger.h>

// 引入外网 Proto 协议和 MsgID
#include "client_gateway.pb.h"
#include "MsgID.h"

using namespace game::client;

MockClient::MockClient(const std::string& name) : name_(name), fd_(-1), uid_(0) {}

MockClient::~MockClient() 
{
    Close();
}

bool MockClient::Connect(const std::string& ip, int port)
{
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (connect(fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        LOG_ERROR << "[" << name_ << "] 连接网关失败!";
        return false;
    }
    LOG_INFO << "[" << name_ << "] 成功建立与 Gateway(8001) 的物理连接!";

    recv_thread_ = std::thread(&MockClient::RecvLoop, this);
    return true;
}

void MockClient::SendLogin(const std::string& username, const std::string& password)
{
    ClientLoginRequest req;
    req.set_username(username);
    req.set_password(password);

    SendMsg(game::net::MSG_LOGIN_REQ, req);
    LOG_INFO << "[" << name_ << "] 发起登录请求 -> 账号: " << username;
}

void MockClient::SendChatMsg(int32_t target_uid, const std::string& content)
{
    ClientChatRequest req;
    
    req.set_target_id(target_uid);
    req.set_chat_type(1);                                           // 私聊
    req.set_content(content);

    SendMsg(game::net::MSG_CHAT_REQ, req);
}

void MockClient::Close()
{
    if (fd_ > 0)
    {
        close(fd_);
        fd_ = -1;
    }
    if (recv_thread_.joinable())
    {
        recv_thread_.join();
    }
}

void MockClient::SendMsg(uint32_t msg_id, const google::protobuf::Message& msg)
{
    std::string pb_data;
    msg.SerializeToString(&pb_data);

    uint32_t res_len = 4 + pb_data.size();
    uint32_t net_len = htonl(res_len);
    uint32_t net_msg_id = htonl(msg_id);

    std::string send_buf;
    send_buf.append((char*)&net_len, 4);
    send_buf.append((char*)&net_msg_id, 4);
    send_buf.append(pb_data);

    send(fd_, send_buf.c_str(), send_buf.size(), 0);
}

void MockClient::RecvLoop()
{
    while (true)
    {
        uint32_t net_len;
        if (recv(fd_, &net_len, 4, 0) <= 0) break;
        uint32_t len = ntohl(net_len);

        uint32_t net_msg_id;
        recv(fd_, &net_msg_id, 4, 0);
        uint32_t msg_id = ntohl(net_msg_id);

        int pb_len = len - 4;
        std::vector<char> buf(pb_len);
        int recv_bytes = 0;
        while (recv_bytes < pb_len)
        {
            int ret = recv(fd_, buf.data() + recv_bytes, pb_len - recv_bytes, 0);
            if(ret <= 0) break;
            recv_bytes += ret;
        }

        std::string pb_data(buf.begin(), buf.end());
        HandleMsg(msg_id, pb_data);
    }
}

void MockClient::HandleMsg(uint32_t msg_id, const std::string& pb_data)
{
    // 登录响应
    if (msg_id == game::net::MSG_LOGIN_RESP)
    {
        ClientLoginResponse resp;
        resp.ParseFromString(pb_data);
        if (resp.errcode() == 0)
        {
            uid_ = resp.user_id();
            LOG_INFO << ">>> [" << name_ << "] 登录成功! 分配 UID: " << uid_;
        }
        else 
        {
            LOG_ERROR << ">>> [" << name_ << "] 登录失败: " << resp.errmsg();
        }
    }

    // chat 广播响应
    if (msg_id == game::net::MSG_CHAT_PUSH)
    {
        ClientChatPush push;
        push.ParseFromString(pb_data);
        LOG_INFO << "🚀🚀🚀 [" << name_ << "] 收到全局推送消息! 🚀🚀🚀 | "
            << "发送者 UID: " << push.sender_id() << " | "
            << "频道: " << (push.chat_type() == 1 ? "私聊" : "世界") << " | "
            << "内容: " << push.content();
    }
}