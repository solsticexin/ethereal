//
// Created by xin on 2026/3/11.
//

#ifndef EIRIAN_TCPSERVER_HPP
#define EIRIAN_TCPSERVER_HPP

#include "eirian/socket.hpp"
#include "eirian/Channel.hpp"
#include "eirian/TcpConnection.hpp"
#include "eirian/EventLoopThreadPool.hpp"

#include <memory>
#include <unordered_map>
#include <functional>
#include <string>

namespace eirian {
    class EventLoop;
    class TcpServer {
    private:
        EventLoop* loop_;
        Socket listenSocket_;
        Channel listenChannel_;

        // 核心：管理所有存活的连接
        std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;

        std::function<void(std::shared_ptr<TcpConnection>,std::string&)> messageCallback_;

        std::unique_ptr<EventLoopThreadPool> threadPool_;
        void newConnection();
        void removeConnection(const std::shared_ptr<TcpConnection>& conn);
    public:
        TcpServer(EventLoop* loop, const InetAddress& listenAddr);
        ~TcpServer()=default;
        // 设置业务层关注的回调
        void setMessageCallback(std::function<void(std::shared_ptr<TcpConnection>, std::string&)> cb) {
            messageCallback_ = std::move(cb);
        }
        void start();
        void setThreadNum(const int numThreads) const {
            threadPool_->setThreadNum(numThreads);
        }
    };
}


#endif //EIRIAN_TCPSERVER_HPP
