#ifndef MY_CHANNEL_H
#define MY_CHANNEL_H

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <string>

class MyChannel : public google::protobuf::RpcChannel
{
public:
    MyChannel(const std::string& ip, int port)
        : ip_(ip), port_(port) {}
    ~MyChannel() {}

public:
    void CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done);

private:
    std::string ip_;
    int port_;
};
#endif