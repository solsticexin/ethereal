//
// Created by xin on 2026/3/12.
//
#include "eirian/EventLoop.hpp"
#include "eirian/InetAddress.hpp"
#include "eirian/TcpServer.hpp"
#include "eirian/logger.hpp"

int main() {
    eirian::EventLoop loop;
    const eirian::InetAddress addr("127.0.0.1", 8888);

    eirian::TcpServer server(&loop, addr);

    // 只写业务逻辑：收到消息怎么回？
    server.setMessageCallback([](const std::shared_ptr<eirian::TcpConnection>& conn, std::string& msg) {
        eirian::log::info("Echoing: {}", msg);
        conn->send(msg); // 异步发送
    });

    server.start();
    loop.loop(); // 发动机启动
    return 0;
}
