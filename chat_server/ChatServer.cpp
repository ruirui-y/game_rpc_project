#include "ChatServiceImpl.h"
#include "RPCServer.h"
#include <mymuduo/Log/Logger.h>

using namespace game::rpc;

int main(int argc, char** argv)
{
    LOG_INFO << "=== Gateway Server is starting on port 9111 ===";

    // 1. 注册聊天服务
    RPCServer chat_rpc_server("127.0.0.1", 9111);
    ChatServiceImpl chat_service;
    chat_rpc_server.RegisterService(&chat_service);

    // 2. 启动事件循环
    chat_rpc_server.Run();
    return 0;
}