#include "ChatServiceImpl.h"
#include <mymuduo/Log/Logger.h>

game::rpc::ChatServiceImpl::ChatServiceImpl()
{
	LOG_INFO << "ChatServiceImpl()";
}

game::rpc::ChatServiceImpl::~ChatServiceImpl()
{

}

void game::rpc::ChatServiceImpl::SendMessage(::google::protobuf::RpcController* controller,
	const::game::rpc::ChatMessageRequest* request,
	::game::rpc::ChatMessageResponse* response, 
	::google::protobuf::Closure* done)
{
	int32_t sender_id = request->sender_id();
	int32_t target_id = request->target_id();
	int32_t type = request->chat_type();
	std::string message = request->content();

	LOG_INFO << "ChatServer Receive Chat Dispatch Request: send_id = " << sender_id
		<< " target_id = " << target_id << " type = " << type << " message = " << message;

	// 私聊
	if (type == 1)
	{
		// 获取目标网关的地址
		std::string target_gateway = GetGatewayAddressFromRedis(target_id);
		if (target_gateway.empty())
		{
			response->set_errcode(404);
			response->set_errmsg("Send Failed: target gateway not found");
		}
		else
		{
			// todo: 构建一个RPC客户端，通过调用网关的PushMessage(RPC方法)，将消息转发回去


            response->set_errcode(0);
			response->set_errmsg("Send Success");
			LOG_INFO << "ChatServer Send Success To Target UID: " << target_id
				<< "Location Gateway: " << target_gateway;
		}
	}
	// 群聊
	else
	{
		// todo
	}

	// 返回rpc响应
	if(done) done->Run();
}

std::string game::rpc::ChatServiceImpl::GetGatewayAddressFromRedis(int32_t targid_uid)
{

	return std::string();
}
