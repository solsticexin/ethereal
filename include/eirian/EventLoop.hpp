//
// Created by xin on 2026/3/10.
//

#ifndef EIRIAN_EVENTLOOP_HPP
#define EIRIAN_EVENTLOOP_HPP
#include "eirian/Epoll.h"
#include <atomic>
#include <unordered_map>

namespace eirian {
    class Channel;
    class EventLoop {
    private:
        Epoll epoll_;
        std::atomic<bool> quit_{false};
        // 核心字典：根据 Epoll 返回的 fd，快速找到对应的 Channel 齿轮
        using FD=int;
        std::unordered_map<FD, Channel*> channels_;
    public:
        EventLoop() = default;
        ~EventLoop() = default;

        // 核心发动机循环
        void loop();

        /// 供 Channel 调用的管理接口
        void updateChannel(Channel* channel);
        void removeChannel(Channel* channel);
    };
} // eirian

#endif //EIRIAN_EVENTLOOP_HPP