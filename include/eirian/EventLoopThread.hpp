//
// Created by xin on 2026/3/12.
//

#ifndef EIRIAN_EVENTLOOPTHREAD_HPP
#define EIRIAN_EVENTLOOPTHREAD_HPP
#include <thread>
#include <future>
#include <mutex>
namespace eirian {
    class EventLoop;
    class EventLoopThread {
        EventLoop* loop_{nullptr}; //指向子线程的loop
        bool exiting_{false};
        std::thread thread_;
        std::mutex mutex_;

        // 线程真正在后台执行的函数
        void threadFunction(std::promise<EventLoop*>& promise);
    public:
        EventLoopThread()=default;
        ~EventLoopThread();
        //启动线程，并返回子线程里创建好的 EventLoop 的指针
        EventLoop* startLoop();
    };
} // eirian

#endif //EIRIAN_EVENTLOOPTHREAD_HPP