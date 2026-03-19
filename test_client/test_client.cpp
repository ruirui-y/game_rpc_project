#include "MockClient.h"
#include <mymuduo/Log/Logger.h>
#include <thread>
#include <chrono>

int main()
{
	LOG_INFO << "=== TensorGate 终极点火测试 ===";

    // 1. 创建两个独立的客户端实例
    MockClient clientA("Player_A");
    if (!clientA.Connect("127.0.0.1", 8001)) return -1;

    MockClient clientB("Player_B");
    if (!clientB.Connect("127.0.0.1", 8001)) return -1;

    // 2. 发起并发登录测试
    clientA.SendLogin("mars", "123456");
    clientB.SendLogin("jupiter", "123456");

    // 等待后台处理完毕 (网关->RPC登录服->写Redis->RPC回传)
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 3. 发起终极链路测试：跨节点聊天推送
    // 假设 A 给 B (这里假设查表后UID是1002) 发送私聊
    clientA.SendChatMsg(2, "hello");

    // 给一点时间让消息飞回来
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 4. 完美退出
    clientA.Close();
    clientB.Close();

    LOG_INFO << "=== 测试执行完毕 ===";
    return 0;
}