#ifndef MSG_ID_H
#define MSG_ID_H

#include <cstdint>

namespace game 
{
namespace net
{

// 外网通信的 MsgID 枚举
enum MsgID : uint32_t 
{
    // 登录模块 (1000起步)
    MSG_LOGIN_REQ = 1001,
    MSG_LOGIN_RESP = 1002,

    // 匹配模块 (2000起步)
    MSG_JOIN_MATCH_REQ = 2001,
    MSG_JOIN_MATCH_RESP = 2002,
    MSG_MATCH_SUCCESS_PUSH = 2003, // 服务端主动推给 UE5 的
};

} // namespace net
} // namespace game

#endif