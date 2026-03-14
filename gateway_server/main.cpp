#include <iostream>
#include "Channel.h"
#include "Log/Logger.h"
#include "login.pb.h"

int main(int argc, char** argv) 
{
    Channel channel;
    LOG_INFO << "=== Gateway Server is starting ===";
    
    game::rpc::LoginService_Stub stub(&channel);
    game::rpc::LoginRequest request;
    game::rpc::LoginResponse response;
    request.set_username("test");
    request.set_password("123456");
    stub.Login(nullptr, &request, &response, nullptr);
    string success = response.errcode() == 0 ? "true" : "false";
    LOG_INFO << "login request " << success;
    LOG_INFO << "login token = " << response.token();
    return 0;
}