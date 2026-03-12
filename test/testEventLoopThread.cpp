#include "eirian/EventLoopThread.hpp"
#include "eirian/EventLoop.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    // EventLoop 暂无 quit 接口，测试中故意泄漏以避免析构阻塞 join。
    auto* loop_thread = new eirian::EventLoopThread();
    eirian::EventLoop* loop = loop_thread->startLoop();
    assert(loop != nullptr);

    // 让子线程有时间进入 loop()
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::cout << "testEventLoopThread passed\n";
    return 0;
}
