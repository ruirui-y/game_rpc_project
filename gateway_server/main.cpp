#include <iostream>
#include "Channel.h"
#include "Log/Logger.h"

int main(int argc, char** argv) 
{
    Channel channel;
    LOG_INFO << "=== Gateway Server is starting ===";
    // 稍后这里会启动 MyMuduo 的 TcpServer，并接收客户端连接
    return 0;
}