#include <iostream>
#include <thread>
#include <chrono>
#include "RPCServer.h"
#include "MyGatewayService.h"
#include "GatewayTcpServer.h"
#include "match.pb.h"
#include "MyChannel.h"

// 模拟 UE5 客户端通过网关发起匹配
void SimulateClientsJoinMatch()
{
    // 等 2 秒，确保网关和匹配服都完全启动了
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 连接到匹配服 (9091端口)
    MyChannel channel("127.0.0.1", 9091);
    game::rpc::MatchService_Stub stub(&channel);

    // ---------------- 模拟玩家 100 加入 ----------------
    game::rpc::JoinMatchRequest req1;
    req1.set_user_id(100);
    req1.set_elo_score(1500);
    req1.set_gateway_ip("127.0.0.1");                                                   // 告诉匹配服：我在这个网关！
    req1.set_gateway_port(8000);                                                        // 告诉匹配服：往这推结果！

    game::rpc::JoinMatchResponse resp1;
    stub.JoinQueue(nullptr, &req1, &resp1, nullptr);
    LOG_INFO << "[Gateway] Player 100 Join Queue Result: " << resp1.errmsg();

    std::this_thread::sleep_for(std::chrono::seconds(1)); // 故意隔 1 秒

    // ---------------- 模拟玩家 200 加入 ----------------
    game::rpc::JoinMatchRequest req2;
    req2.set_user_id(200);
    req2.set_elo_score(1505);
    req2.set_gateway_ip("127.0.0.1");
    req2.set_gateway_port(8000);

    game::rpc::JoinMatchResponse resp2;
    stub.JoinQueue(nullptr, &req2, &resp2, nullptr);
    LOG_INFO << "[Gateway] Player 200 Join Queue Result: " << resp2.errmsg();
}

int main(int argc, char** argv)
{
    LOG_INFO << "=== Gateway Server is starting on port 8000 ===";

    // 1. 启动外网网关
    EventLoop client_loop;
    GatewayTcpServer tcp_server(&client_loop, "0.0.0.0", 8001);
    tcp_server.Start(4);

    // 在后台线程中监听外部的长连接
    std::thread client_thread([&client_loop]()
        {
            LOG_INFO << "[Gateway] External TCP Server listening on 8001...";
            client_loop.Loop();
        });
    client_thread.detach();

    // 2. 内网RPC
    RPCServer gateway_rpc_server("127.0.0.1", 8000);

    // 把 tcp_server 的指针喂给 RPC 服务，供其余的rpc将消息路由到客户端
    MyGatewayService gateway_service(&tcp_server);
    gateway_rpc_server.RegisterService(&gateway_service);

    LOG_INFO << "[Gateway] Internal RPC Server listening on 8000...";

    // 3. 阻塞主线程运行
    gateway_rpc_server.Run();
    return 0;
}