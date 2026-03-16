#ifndef MOCK_CLIENT_H
#define MOCK_CLIENT_H

#include <string>
#include <thread>
#include <vector>
#include <google/protobuf/message.h>

class MockClient
{
public:
	MockClient(const std::string& name);
	~MockClient();

	// 核心接口
	bool Connect(const std::string& ip, int port);
	void SendLogin(const std::string& user_name, const std::string& password);
	void SendChatMsg(int32_t target_uid, const std::string& content);
	void Close();

public:
	std::string name_;
	int fd_;
	int32_t uid_;
	std::thread recv_thread_;

private:
	// 底层网络与解包
	void SendMsg(uint32_t msg_id, const google::protobuf::Message& msg);
	void RecvLoop();
	void HandleMsg(uint32_t msg_id, const std::string& pb_data);
};

#endif