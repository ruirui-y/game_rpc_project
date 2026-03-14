#ifndef MY_MATCH_SERVICE_H
#define MY_MATCH_SERVICE_H

#include <iostream>
#include <string>
#include <mutex>
#include <vector>
#include <mymuduo/Log/Logger.h>
#include "MyChannel.h"
#include "match.pb.h"
#include "gateway.pb.h"

using namespace game::rpc;

// 模拟一个简单的玩家匹配信息结构体
struct PlayerInfo 
{
    int32_t user_id;
    int32_t elo_score;
    std::string gateway_ip;    
    int32_t gateway_port;      
};

class MyMatchService : public game::rpc::MatchService
{
public:
    virtual void JoinQueue(::google::protobuf::RpcController* controller,
        const ::game::rpc::JoinMatchRequest* request,
        ::game::rpc::JoinMatchResponse* response,
        ::google::protobuf::Closure* done) override
    {
        int32_t uid = request->user_id();
        int32_t score = request->elo_score();
        std::string gw_ip = request->gateway_ip();
        int32_t gw_port = request->gateway_port();

        LOG_INFO << "Player " << uid << " (Score: " << score << ") requests to join match queue.";

        // 1. 将玩家加入到服务端的内存匹配池中
        {
            std::lock_guard<std::mutex> lock(match_mutex_);
            // 简单防重校验
            bool is_exist = false;
            for (const auto& p : match_pool_) 
            {
                if (p.user_id == uid) 
                {
                    is_exist = true; break;
                }
            }

            if (!is_exist) 
            {
                match_pool_.push_back({ uid, score, gw_ip, gw_port });
                response->set_errcode(0);
                response->set_errmsg("Join queue success");
            }
            else
            {
                response->set_errcode(1);
                response->set_errmsg("Already in queue");
            }
        }

        // 2. 立即将“加入队列成功”的响应返回给网关/客户端
        done->Run();
    }

    // 后台线程定时执行检查匹配池中的玩家是否满足匹配条件
    void DoMatch() 
    {
        std::lock_guard<std::mutex> lock(match_mutex_);

        if (match_pool_.size() >= 2)
        {
            LOG_INFO << "Match Success! Found 2 players. Preparing to push results...";

            for (int i = 0; i < 2; ++i)
            {
                int32_t target_uid = match_pool_[i].user_id;
                std::string target_gw_ip = match_pool_[i].gateway_ip;
                int32_t target_gw_port = match_pool_[i].gateway_port;

                // 获取客户端
                MyChannel gateway_channel(target_gw_ip, target_gw_port);
                GatewayService_Stub stub(&gateway_channel);
                
                PushMessageRequest push_req;
                push_req.set_user_id(target_uid);
                push_req.set_msg_type(1);                                                       // 1代表匹配成功通知
                push_req.set_content("");
                
                PushMessageResponse push_resp;
                stub.PushMessage(nullptr, &push_req, &push_resp, nullptr);

                if (push_resp.errcode() == 0) 
                {
                    LOG_INFO << "Successfully pushed match result to Gateway for User: " << target_uid;
                }
                else
                {
                    LOG_ERROR << "Push failed for User: " << target_uid;
                }
            }
            match_pool_.erase(match_pool_.begin(), match_pool_.begin() + 2);
        }
    }

private:
    std::vector<PlayerInfo> match_pool_;                                                        // 匹配池
    std::mutex match_mutex_;                                                                    // 保护匹配池的锁
};

#endif