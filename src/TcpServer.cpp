//
// Created by xin on 2026/3/11.
//

#include "eirian//TcpServer.hpp"
#include "eirian/EventLoopThreadPool.hpp"
#include "eirian/InetAddress.hpp"
#include "eirian/logger.hpp"

namespace eirian {
    TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr)
    :loop_(loop),
    listenChannel_(loop_,listenSocket_.getFd()),
    threadPool_(std::make_unique<EventLoopThreadPool>(loop)){
        this->listenSocket_.bind(listenAddr);
        this->listenSocket_.listen();
        if (!this->listenSocket_.setNonBlocking()) {
            log::warn("开启非阻塞模式设置失败!Ip:{},port:{}",listenAddr.getIp(),listenAddr.getPort());
        }
        this->listenChannel_.setReadCallback([this]() {
            this->newConnection();
        });
    }

    void TcpServer::start() {
        threadPool_->start();
        this->listenChannel_.enableReading();
        log::info("TcpServer 已启动");
    }

    void TcpServer::newConnection() {
        InetAddress client_addr{"0.0.0.0",0};
        if (auto client_socket_optional{this->listenSocket_.accept(client_addr)}) {
            Socket client_socket{std::move(client_socket_optional.value())};
            if (!client_socket.setNonBlocking()) {
                log::warn("开启非阻塞模式设置失败!Ip:{},port:{}",client_addr.getIp(),client_addr.getPort());
            }
            const auto fd{client_socket.getFd()};
            // const auto conn{std::make_shared<TcpConnection>(this->loop_,std::move(client_socket))};
            // this->connections_[fd]=conn;
            // conn->setMessageCallback(messageCallback_);
            // conn->setCloseCallback([this](const std::shared_ptr<TcpConnection>& c) {
            //     this->removeConnection(c);
            // });
            // conn->connectEstablished();

            EventLoop* ioLoop = threadPool_->getNextLoop();
            log::info("新客户端已连接! FD: {}, 分配给新的 SubLoop", fd);
            auto conn = std::make_shared<TcpConnection>(ioLoop, std::move(client_socket));
            connections_[fd] = conn;
            conn->setMessageCallback(messageCallback_);
            conn->setCloseCallback([this](const std::shared_ptr<TcpConnection>& c) {
                removeConnection(c);
            });

            conn->connectEstablished();
        }
    }

    void TcpServer::removeConnection(const std::shared_ptr<TcpConnection> &conn) {
        auto fd{conn->getFd()};
        conn->connectDestroyed();
        this->connections_.erase(fd);
        log::info("TcpServer: 移除连接 FD:{}", fd);
    }
}
