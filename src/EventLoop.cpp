//
// Created by xin on 2026/3/10.
//

#include "eirian/EventLoop.hpp"

#include "eirian/Channel.hpp"

namespace eirian {
    void EventLoop::loop() {
        while (!this->quit_) {
            //poll返回epoll_event结构体，在把结构体解构
            for (auto& [events,ev_data]: this->epoll_.poll(10000)) {
                if (this->channels_.contains(ev_data.fd)) {
                    auto* ch=this->channels_[ev_data.fd];
                    ch->setRevents(events);
                    ch->handleEvent();
                }
            }
        }
    }

    void EventLoop::updateChannel(Channel *channel) {
        const auto fd{channel->getFd()};
        this->channels_[fd]=channel;
        if (!channel->getInEpoll()) {
            this->epoll_.add(fd,channel->getEvents());
            channel->setInEpoll(true);
        }else {
            this->epoll_.modify(fd,channel->getEvents());
        }
    }

    void EventLoop::removeChannel(Channel *channel) {
        const auto fd{channel->getFd()};
        this->epoll_.remove(fd);
        this->channels_.erase(fd);
        channel->setInEpoll(false);
    }

    void EventLoop::quit() {
        this->quit_ = true;

        // TODO: 唤醒可能正在阻塞的 epoll_wait (极其重要，下面会讲)
        // if (!isInLoopThread()) {
        //     wakeup();
        // }
    }
} // eirian