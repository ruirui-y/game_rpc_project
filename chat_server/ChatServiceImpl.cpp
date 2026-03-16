#include "ChatServiceImpl.h"
#include <mymuduo/Log/Logger.h>
#include "RedisClient.h"
#include "MyChannel.h"
#include "gateway.pb.h"

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
			// 构建一个RPC客户端，通过调用网关的PushMessage(RPC方法)，将消息转发回去
			// 解析出 IP 和 端口
			std::string ip = target_gateway.substr(0, target_gateway.find(':'));
			int port = std::stoi(target_gateway.substr(target_gateway.find(':') + 1));

			// 初始化RPC通道
			MyChannel channel(ip, port);
			GatewayService_Stub stub(&channel);

			// 组装发给网关的推送请求
			PushMessageRequest push_req;
			push_req.set_user_id(target_id);
			push_req.set_msg_type(3003);
			push_req.set_content(message);

			// 响应
			PushMessageResponse push_res;
			
			// 发起RPC调用
			stub.PushMessage(nullptr, &push_req, &push_res, nullptr);

			// 消息路由成功
			if (push_res.errcode() == 0)
			{
				response->set_errcode(0);
				response->set_errmsg("Send Success");
				LOG_INFO << "ChatServer Send Success To Target UID: " << target_id
					<< "Location Gateway: " << target_gateway;
			}
			else
			{
				response->set_errcode(500);
				response->set_errmsg("Gateway PushMessage Failed");
                LOG_INFO << "ChatServer Send Failed To Target UID: " << target_id
					<< "Location Gateway: " << target_gateway;
			}
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

std::string game::rpc::ChatServiceImpl::GetGatewayAddressFromRedis(int32_t target_uid)
{
	RedisClient redis;
	
	// 如果 Redis 连不上，直接返回空，让外层判断玩家不在线
	if (!redis.Connect("127.0.0.1", 6379))
	{
		LOG_INFO << "[ChatServer] 警告：无法连接 Redis 获取路由表！";
		return "";
	}

	std::string uid_str = std::to_string(target_uid);
	std::string target_gateway = redis.HGet("session:users", uid_str);

	return target_gateway;
}
