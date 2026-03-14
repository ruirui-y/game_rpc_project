#ifndef MY_MATCH_SERVICE_H
#define MY_MATCH_SERVICE_H

#include <iostream>
#include <string>
#include <mutex>
#include <vector>
#include <mymuduo/Log/Logger.h>
#include "Channel.h"
#include "match.pb.h"


using namespace game::rpc;

// 模拟一个简单的玩家匹配信息结构体
struct PlayerInfo 
{
    int32_t user_id;
    int32_t elo_score;
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
                match_pool_.push_back({ uid, score });
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

        if (match_pool_.size() > 2)
        {
            LOG_INFO << "Match Success! Found 2 players. Preparing to push results...";

            for (int i = 0; i < 2; ++i)
            {
                int32_t target_uid = match_pool_[i].user_id;

                // 获取客户端
                Channel gateway_channel;


            }
        }
    }

private:
    std::vector<PlayerInfo> match_pool_;                                                        // 匹配池
    std::mutex match_mutex_;                                                                    // 保护匹配池的锁
};

#endif