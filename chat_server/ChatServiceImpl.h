#ifndef CHAT_SERVICE_IMPL_H
#define CHAT_SERVICE_IMPL_H

#include "chat.pb.h"
#include <string>
namespace game 
{
namespace rpc
{
class ChatServiceImpl : public ChatService
{
public:
	ChatServiceImpl();
	~ChatServiceImpl();

	void SendMessage(::google::protobuf::RpcController* controller,
		const ::game::rpc::ChatMessageRequest* request,
		::game::rpc::ChatMessageResponse* response,
		::google::protobuf::Closure* done) override;

private:
	std::string GetGatewayAddressFromRedis(int32_t target_uid);
};
}
}

#endif