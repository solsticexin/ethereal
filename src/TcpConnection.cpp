//
// Created by xin on 2026/3/11.
//

#include "../include/eirian/TcpConnection.hpp"
#include "eirian/EventLoop.hpp"
#include "eirian/logger.hpp"
namespace eirian {
    TcpConnection::TcpConnection(EventLoop *loop, Socket &&socket )
    :loop_(loop),
    socket_(std::make_unique<Socket>(std::move(socket))),
    channel_(std::make_unique<Channel>(loop, socket_->getFd())){
        channel_->setReadCallback([this]() {
            this->handleRead();
        });
        channel_->setCloseCallback([this]() {
            this->handleClose();
        });
        this->channel_->setWriteCallback([this]() {
            this->handleWrite();
        });
    }

    TcpConnection::~TcpConnection() {
        log::info("TcpConnection(FD: {}) 析构，资源已释放", socket_->getFd());
    }

    void TcpConnection::connectEstablished() const {
        channel_->enableReading();
    }

    void TcpConnection::connectDestroyed() const {
        // 从 Epoll 中移除监听
        loop_->removeChannel(channel_.get());
    }

    void TcpConnection::handleRead() {
        char buf[4096];

        if (const ssize_t n = socket_->receive(buf, sizeof(buf)); n > 0) {
            // 1. 数据入缸
            inputBuffer_.append(buf, static_cast<int>(n));

            // 2. 取出字符串
            std::string msg = inputBuffer_.retrieveAllAsString();

            // 3. 触发业务回调！将自己(shared_from_this)和消息传给上层
            if (messageCallback_) {
                messageCallback_(shared_from_this(), msg);
            }
        } else if (n == 0) {
            // 对端正常关闭
            handleClose();
        } else {
            // 发生错误
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                log::error("TcpConnection(FD: {}) 发生异常", socket_->getFd());
                handleError();
            }
        }
    }

    void TcpConnection::handleClose() {
        log::info("TcpConnection(FD: {}) 连接已断开", socket_->getFd());
        // 触发关闭回调，告诉上层（比如 Server）把自己从 map 里删掉
        if (closeCallback_) {
            closeCallback_(shared_from_this());
        }
    }

    void TcpConnection::handleError() {
        // 错误处理，通常直接走 close 逻辑
        handleClose();
    }

    void TcpConnection::send(const std::string& msg) {
        ssize_t nwrote = 0;
        size_t remaining = msg.length();
        bool faultError = false;

        // 如果此时 outputBuffer_ 里已经有积压的数据没发完，
        // 我们绝不能直接调用底层 send，必须按顺序排在后面，防止数据乱序！
        if (outputBuffer_.readableBytes() == 0) {
            // 尝试直接发送
            nwrote = socket_->send(msg.data(), msg.length());

            if (nwrote >= 0) {
                remaining = msg.length() - nwrote;
                if (remaining == 0) {
                    // eirian::log::info("数据一次性发送完毕！");
                    // (如果有 writeCompleteCallback，可以在这里触发)
                }
            } else {
                nwrote = 0;
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    eirian::log::error("TcpConnection::send 发生致命错误");
                    faultError = true;
                }
            }
        }

        // 如果没有发生致命错误，且还有数据没发完（或者 outputBuffer_ 原本就有积压）
        if (!faultError && remaining > 0) {
            // 把没发完的数据追加到 outputBuffer_ 里面
            outputBuffer_.append(msg.data() + nwrote, remaining);

            // 重点：告诉 Epoll，一旦内核发送缓冲区有空间了，立刻触发 EPOLLOUT 事件！
            if (!channel_->isWriting()) {
                channel_->enableWriting();
            }
        }
    }
    void TcpConnection::handleWrite() {
        // 只有当前 Channel 真的在关注写事件时，才处理
        if (channel_->isWriting()) {
            // 尝试把 outputBuffer_ 里的所有积压数据全发出去
            ssize_t n = socket_->send(outputBuffer_.peek(), outputBuffer_.readableBytes());

            if (n > 0) {
                // 从水缸里把已经成功发给内核的数据删掉
                outputBuffer_.retrieve(n);

                // 如果水缸空了，说明全发完了！
                if (outputBuffer_.readableBytes() == 0) {
                    // 重点：立刻取消关注可写事件！
                    // 否则 Epoll 会陷入死循环的疯狂通知（因为内核缓冲区大部分时间都是空的、可写的）
                    channel_->disableWriting();

                    // (如果有 writeCompleteCallback，可以在这里触发)
                }
            } else {
                eirian::log::error("TcpConnection::handleWrite 发送失败");
            }
        }
    }
} // eirian
