//
// Created by xin on 2026/3/10.
//

#ifndef EIRIAN_CHANNEL_HPP
#define EIRIAN_CHANNEL_HPP
#include <cstdint>
#include <functional>


namespace eirian {
    class Socket;
    class EventLoop;
    class Channel{
    private:
        EventLoop* loop_;
        int fd_;
        uint32_t events_;   //正在监听事件
        uint32_t revents_;  //epoll返回事件
        bool in_epoll_{false};// 标记自己是否已经被添加到了 Epoll 中
        // 信道的回调函数
        std::function<void()> readCallback_;
        std::function<void()> writeCallback_;
        std::function<void()> closeCallback_;

    public:
        Channel(EventLoop* loop, int fd);
        ~Channel() = default;

        // 核心：当 Epoll 发现这个 fd 有事件时，调用这个函数
        void handleEvent() const;

        // 开启监听可读事件
        void enableReading();

        // 注册回调函数（提供给上层使用）
        void setReadCallback(std::function<void()> cb) { readCallback_ = std::move(cb); }
        void setCloseCallback(std::function<void()> cb) { closeCallback_ = std::move(cb); }

        // 供 EventLoop 使用的接口
        [[nodiscard]] int getFd() const { return fd_; }
        [[nodiscard]]uint32_t getEvents() const { return events_; }
        void setRevents(const uint32_t ev) { revents_ = ev; }
        [[nodiscard]] bool getInEpoll() const { return in_epoll_; }
        void setInEpoll(bool in = true) { in_epoll_ = in; }
    };
}


#endif //EIRIAN_CHANNEL_HPP