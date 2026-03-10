//
// Created by xin on 2026/3/10.
//

#include "../include/eirian/Channel.hpp"

#include <sys/epoll.h>

#include "eirian/EventLoop.hpp"

namespace eirian{
    Channel::Channel(EventLoop* loop,const int fd):loop_(loop),fd_(fd),events_(0),revents_(0) {}

    void Channel::handleEvent() const {
        // 1. 处理对方挂断事件
        if ((this->revents_ & EPOLLHUP) && !(this->events_ & EPOLLIN)) {
            if (closeCallback_) closeCallback_();
        }
        // 2. 处理可读事件
        if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
            if (readCallback_) readCallback_();
        }
        // 3. 处理可写事件
        if (revents_ & EPOLLOUT) {
            if (writeCallback_) writeCallback_();
        }
    }

    void Channel::enableReading() {
        this->events_ |= EPOLLIN | EPOLLPRI;
        loop_->updateChannel(this);
    }
}
