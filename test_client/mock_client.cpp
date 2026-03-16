#include <iostream>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include "client_gateway.pb.h" // 你的外网协议
// 如果你的 MsgID.h 在 gateway_server 目录下，记得包含它的相对路径
// #include "../gateway_server/MsgID.h" 

// 假设你的 MsgID 是这么定义的，这里直接写死方便测试
const uint32_t MSG_LOGIN_REQ = 1001;
const uint32_t MSG_LOGIN_RESP = 1002;
const uint32_t MSG_JOIN_MATCH_REQ = 2001;
const uint32_t MSG_JOIN_MATCH_RESP = 2002;
const uint32_t MSG_MATCH_SUCCESS_PUSH = 2003;

// 发包工具：封装 [4字节长度 + 4字节MsgID + Protobuf数据]
void SendPacket(int sock, uint32_t msg_id, const std::string& pb_data) {
    uint32_t res_len = 4 + pb_data.size();
    uint32_t net_len = htonl(res_len);
    uint32_t net_msg_id = htonl(msg_id);

    std::string send_buf;
    send_buf.append((char*)&net_len, 4);
    send_buf.append((char*)&net_msg_id, 4);
    send_buf.append(pb_data);

    send(sock, send_buf.c_str(), send_buf.size(), 0);
}

// 收包工具：阻塞接收一整个完整的包
bool RecvPacket(int sock, uint32_t& out_msg_id, std::string& out_pb_data) {
    uint32_t net_len;
    if (recv(sock, &net_len, 4, 0) <= 0) return false;
    uint32_t res_len = ntohl(net_len);

    uint32_t net_msg_id;
    if (recv(sock, &net_msg_id, 4, 0) <= 0) return false;
    out_msg_id = ntohl(net_msg_id);

    int pb_len = res_len - 4;
    if (pb_len > 0) {
        char* buffer = new char[pb_len];
        int total_read = 0;
        while (total_read < pb_len) {
            int n = recv(sock, buffer + total_read, pb_len - total_read, 0);
            if (n <= 0) break;
            total_read += n;
        }
        out_pb_data.assign(buffer, pb_len);
        delete[] buffer;
    }
    else {
        out_pb_data = "";
    }
    return true;
}

int main() {
    std::cout << "=== Mock UE5 Client Starting ===" << std::endl;

    // 1. 连接网关 8001 端口
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8001);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed!" << std::endl;
        return -1;
    }
    std::cout << "[Client] Connected to Gateway successfully." << std::endl;

    // 2. 发起登录请求
    game::client::ClientLoginRequest login_req;
    login_req.set_username("admin");
    login_req.set_password("123456");
    std::string login_data;
    login_req.SerializeToString(&login_data);

    SendPacket(sock, MSG_LOGIN_REQ, login_data);
    std::cout << "[Client] Sent Login Request." << std::endl;

    // 3. 循环接收服务器的各种消息
    uint32_t msg_id;
    std::string pb_data;
    while (RecvPacket(sock, msg_id, pb_data)) {
        if (msg_id == MSG_LOGIN_RESP) {
            game::client::ClientLoginResponse login_resp;
            login_resp.ParseFromString(pb_data);
            std::cout << "[Client] Received Login Response! UID: " << login_resp.user_id() << std::endl;

            // 登录成功后，立刻发起匹配请求！
            game::client::ClientJoinMatchRequest match_req;
            std::string match_data;
            match_req.SerializeToString(&match_data);
            SendPacket(sock, MSG_JOIN_MATCH_REQ, match_data);
            std::cout << "[Client] Sent Join Match Request." << std::endl;
        }
        else if (msg_id == MSG_JOIN_MATCH_RESP) {
            game::client::ClientJoinMatchResponse match_resp;
            match_resp.ParseFromString(pb_data);
            std::cout << "[Client] Received Join Match Response: " << match_resp.errmsg() << std::endl;
            std::cout << "[Client] Waiting for Match Success Push..." << std::endl;
        }
        else if (msg_id == MSG_MATCH_SUCCESS_PUSH) {
            game::client::ClientMatchSuccessPush push_msg;
            push_msg.ParseFromString(pb_data);
            std::cout << "\n============================================\n";
            std::cout << "[Client] MATCH SUCCESS PUSH RECEIVED!\n";
            std::cout << "Battle Server IP: " << push_msg.battle_server_ip() << std::endl;
            std::cout << "Battle Server Port: " << push_msg.battle_server_port() << std::endl;
            std::cout << "============================================\n\n";
            break; // 匹配成功，测试完成退出
        }
        else {
            std::cout << "[Client] Unknown MsgID received: " << msg_id << std::endl;
        }
    }

    close(sock);
    return 0;
}