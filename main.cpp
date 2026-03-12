#include "eirian/InetAddress.hpp"
#include "eirian/EventLoop.hpp"
#include "eirian/logger.hpp"
#include "eirian/TcpServer.hpp"

int main() {
    eirian::EventLoop mainLoop;
    const eirian::InetAddress addr("127.0.0.1", 8888);
    eirian::TcpServer server(&mainLoop, addr);

    // 设置 4 个子线程（通常设置为 CPU 核心数）
    // 你现在的服务器是一个 MainLoop + 4 个 SubLoop 的终极架构了！
    server.setThreadNum(4); 

    server.setMessageCallback([](const std::shared_ptr<eirian::TcpConnection>& conn, const std::string& msg) {
        eirian::log::info("收到消息，正在异步处理...");
        conn->send(msg);
    });

    server.start();
    mainLoop.loop(); // 老板线程开始阻塞监听新连接
    return 0;
}