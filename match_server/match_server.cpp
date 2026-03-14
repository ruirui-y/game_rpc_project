#include <iostream>
#include <thread>
#include <chrono>
#include "RPCServer.h"
#include "MyMatchService.h"

int main(int argc, char** argv)
{
    // 1. 匹配服监听 9091 端口
    RPCServer server("127.0.0.1", 9091);
    MyMatchService match_service;
    server.RegisterService(&match_service);

    // 后台匹配
    std::thread match_thread([&match_service]() 
        {
            while (true) 
            {
                match_service.DoMatch();
                // 休眠 1 秒，防止死循环把 CPU 跑满
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    match_thread.detach(); // 分离线程，让它自己在后台跑

    LOG_INFO << "=== Match Server is running on port 9091 ===";

    // 2. 阻塞监听网关发来的 JoinQueue 请求
    server.Run();

    return 0;
}