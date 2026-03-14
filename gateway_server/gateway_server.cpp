#include <iostream>
#include <mymuduo/Log/Logger.h>
#include "RPCServer.h"
#include "MyGatewayService.h"

int main(int argc, char** argv)
{
    LOG_INFO << "=== Gateway Server is starting on port 8000 ===";

    // 网关自己开一个 RPCServer，专门用来接收后端的反向调用
    RPCServer gateway_server("127.0.0.1", 8000);
    MyGatewayService gateway_service;
    gateway_server.RegisterService(&gateway_service);

    // 阻塞运行
    gateway_server.Run();

    return 0;
}