#ifndef MY_LOGIN_SERVICE_H
#define MY_LOGIN_SERVICE_H

#include <string>
#include "login.pb.h"
#include <mymuduo/Log/Logger.h>
using namespace game::rpc;

class MyLoginService : public game::rpc::LoginService
{
public:
    // 真正的本地业务逻辑
    bool Login(std::string name, std::string pwd)
    {
        LOG_INFO << "Execute Login Service";
        LOG_INFO << "user_name = " << name.c_str() << " pwd = " << pwd.c_str();
        return true;
    }

    virtual void Login(::google::protobuf::RpcController* controller,
        const ::LoginRequest* request,
        ::LoginResponse* response,
        ::google::protobuf::Closure* done)
    {
        // 1. 框架给业务吐出请求参数
        std::string name = request->username();
        std::string pwd = request->password();

        // 2. 执行本地业务
        bool login_result = Login(name, pwd);                                                       

        // 3. 把响应写回给框架
        response->set_errcode(login_result == true ? 0 : 1);
        response->set_errmsg("");                                                                   
        response->set_token("11111111");

        // 4. 执行回调操作
        // 这里的 done 本质是 RpcProvider 传进来的 SendRpcResponse 回调
        done->Run();                                                                                
    }
};

#endif