//
// Created by xin on 2026/3/11.
//
#include "eirian/EventLoop.hpp"
#include "eirian/Channel.hpp"
#include "eirian/TcpConnection.hpp"
#include "eirian/InetAddress.hpp"
#include "eirian/logger.hpp"
#include "eirian/socket.hpp"

#include <unordered_map>
#include <memory>

int main() {
    eirian::log::info("基于 TcpConnection 的回声服务器开始启动...");

    try {
        // 1. 初始化事件循环
        eirian::EventLoop loop;

        // 2. 初始化监听 Socket
        const eirian::Socket server_socket{};
        server_socket.bind(eirian::InetAddress("127.0.0.1", 8888));
        server_socket.listen();

        if (!server_socket.setNonBlocking()/* 必须非阻塞*/) {
            eirian::log::warn("非致命错误设置服务器为非阻塞模式失败！");
        }

        eirian::log::info("服务器已绑定 127.0.0.1:8888，开始监听");

        // 3. 为监听 Socket 创建专属 Channel
        eirian::Channel server_channel(&loop, server_socket.getFd());

        // 【大管家容器】专门用来存放连进来的客户端，保证它们的生命周期
        std::unordered_map<int, std::shared_ptr<eirian::TcpConnection>> connections;

        // 4. 配置监听 Channel 的可读事件（处理新用户连接）
        server_channel.setReadCallback([&]() {
            eirian::InetAddress client_addr("0.0.0.0", 0);

            if (auto client_sock_opt = server_socket.accept(client_addr)) {
                eirian::Socket client_sock = std::move(client_sock_opt.value());
                if (!client_sock.setNonBlocking()){ eirian::log::warn("非致命错误设置客户端为非阻塞模式失败！");} // 客户端 Socket 必须非阻塞
                int fd = client_sock.getFd();

                eirian::log::info("新客户端已连接! FD: {}", fd);

                // ==========================================
                // 核心变化：将一切脏活累活丢给 TcpConnection
                // ==========================================
                const auto conn = std::make_shared<eirian::TcpConnection>(&loop, std::move(client_sock));

                // 注册业务回调：当收到消息时怎么处理？
                conn->setMessageCallback([](const std::shared_ptr<eirian::TcpConnection>& connection, std::string& msg) {
                    std::string log_msg = msg;
                    while (!log_msg.empty() && (log_msg.back() == '\n' || log_msg.back() == '\r')) {
                        log_msg.pop_back();
                    }
                    eirian::log::info("收到来自 FD:{} 的消息: {}", connection->getFd(), log_msg);

                    // 回声测试：直接调用我们写好的强力 send 接口发回去！
                    // 如果发不完，TcpConnection 底层会自动把剩下的塞进 outputBuffer_ 并监听 EPOLLOUT
                    connection->send(msg);
                });

                // 注册关闭回调：当连接断开时怎么处理？
                conn->setCloseCallback([&connections](const std::shared_ptr<eirian::TcpConnection>& connection) {
                    int conn_fd = connection->getFd();
                    eirian::log::info("客户端 FD:{} 彻底断开，从 map 中移除", conn_fd);

                    // 断开时清理 Epoll 监听
                    connection->connectDestroyed();

                    // 从容器中擦除，智能指针引用计数归零，TcpConnection 对象自动析构！
                    connections.erase(conn_fd);
                });

                // 保存这个连接
                connections[fd] = conn;

                // 启动这个连接的读事件监听
                conn->connectEstablished();
            }
        });

        // 5. 启动监听 Channel
        server_channel.enableReading();

        // 6. 发动机点火
        eirian::log::info("EventLoop 发动机启动，全面接管网络 IO...");
        loop.loop();

    } catch (const std::exception& e) {
        eirian::log::error("服务器发生致命异常: {}", e.what());
    }

    return 0;
}
