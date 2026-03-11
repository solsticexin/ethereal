#include "eirian/Buffer.h"
#include "eirian/EventLoop.hpp"
#include "eirian/InetAddress.hpp"
#include "eirian/logger.hpp"
#include "eirian/socket.hpp"
#include "eirian/Channel.hpp"

int main() {
    eirian::log::info("测试服务器开始");
    try {
        //创建事件循环,内部会创建epoll.,
        eirian::EventLoop loop{};
        //创建server socket
        const eirian::Socket server_socket{};
        //绑定
        server_socket.bind(eirian::InetAddress{"127.0.0.1",8888});
        //监听
        server_socket.listen();
        //设置非阻塞
        if (!server_socket.setNonBlocking()) {
            eirian::log::warn("设置server socket为非阻塞模式失败但不是致命错误");
        }
        eirian::log::info("服务器已绑定 127.0.0.1:8888，开始监听");

        eirian::Channel server_channel{&loop,server_socket.getFd()};

        //没有完善tcp connection 临时存放在main函数中
        std::unordered_map<int, eirian::Socket> client_sockets;
        std::unordered_map<int, std::unique_ptr<eirian::Channel>> client_channels;
        std::unordered_map<int, eirian::Buffer> client_buffers; // 每个客户端发一个水缸

        // 4. 核心：给监听 Channel 设置回调函数。
        // 当有新客户端连接时，Epoll 会触发可读事件，进而调用这个 Lambda 函数！
        server_channel.setReadCallback([&]() {
            eirian::InetAddress client_addr{"0.0.0.0",0};
            if (auto client_sock_optional{server_socket.accept(client_addr)}) {
                eirian::Socket client_socket{std::move(client_sock_optional.value())};
                if (!client_socket.setNonBlocking())
                    eirian::log::warn("client socket为非阻塞模式失败但不是致命错误");
                auto client_fd{client_socket.getFd()};
                eirian::log::info("新客户端已连接! FD: {}", client_fd);
                // 保存 Socket，防止被析构
                client_sockets.emplace(client_fd, std::move(client_socket));
                // 为这个新客户端创建一个专属的 Channel
                auto client_channel = std::make_unique<eirian::Channel>(&loop, client_fd);
                // 给客户端 Channel 设置读数据回调 (这就是咱们刚才讨论的 ssize_t 处理逻辑)
                client_channel->setReadCallback([client_fd, &client_sockets, &client_channels, &client_buffers, &loop]() {
                    char buf[1024];
                    // 使用极薄封装的 recv
                    const ssize_t n = client_sockets[client_fd].receive(buf, sizeof(buf),0);

                    if (n > 0) {
                        // 收到数据，倒进对应的水缸
                        client_buffers[client_fd].append(buf, n);
                        // 像信道一样取出完整字符串
                        std::string msg = client_buffers[client_fd].retrieveAllAsString();

                        eirian::log::info("收到客户端(FD:{})消息: {}", client_fd, msg);

                        // Echo 测试：原样发回去
                        if (const auto server_ch{client_sockets[client_fd].send(msg.c_str(), msg.length(),0)}; server_ch<0) {
                            eirian::log::warn(" Echo 测试：原样发回去 失败！！！");
                        }

                    } else if (n == 0) {
                        // 客户端主动断开
                        eirian::log::info("客户端(FD:{})正常断开连接", client_fd);
                        loop.removeChannel(client_channels[client_fd].get());
                        client_channels.erase(client_fd);
                        client_sockets.erase(client_fd);
                        client_buffers.erase(client_fd);

                    } else {
                        // n == -1，发生错误
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            eirian::log::error("客户端(FD:{})发生异常错误, 强制断开", client_fd);
                            loop.removeChannel(client_channels[client_fd].get());
                            client_channels.erase(client_fd);
                            client_sockets.erase(client_fd);
                            client_buffers.erase(client_fd);
                        }
                    }
                });

                // 启动客户端 Channel 的事件监听
                client_channel->enableReading();
                client_channels[client_fd] = std::move(client_channel);
            }
        });
        // 5. 启动监听 Channel 的事件监听 (让 Epoll 开始关注它)
        server_channel.enableReading();

        // 6. 引擎点火！死循环开始，接管一切！
        eirian::log::info("EventLoop 发动机启动...");
        loop.loop();
    } catch (const std::exception& e) {
        eirian::log::error("服务器发生致命异常: {}", e.what());
    }
}
