//
// Created by xin on 2026/3/12.
//

#ifndef EIRIAN_EVENTLOOPTHREADPOOL_HPP
#define EIRIAN_EVENTLOOPTHREADPOOL_HPP
#include <vector>
#include <memory>
namespace eirian {
    class EventLoop;
    class EventLoopThread;
    class EventLoopThreadPool {
    private:
        EventLoop* baseLoop_; // 主线程的 Loop（老板）
        bool started_{false};
        int numThreads_{0};   // 需要创建几个子线程
        int next_{0};         // 轮询分配的下标

        // 存放线程对象的容器（负责生命周期）
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        // 存放所有子线程里 EventLoop 裸指针的容器（用于快速分配）
        std::vector<EventLoop*> loops_;

    public:
        explicit EventLoopThreadPool(EventLoop* baseLoop);
        ~EventLoopThreadPool(); // 依赖 unique_ptr 自动析构管理子线程

        // 设置线程数量（一定要在 start 之前调用）
        void setThreadNum(int numThreads) { numThreads_ = numThreads; }

        // 启动所有子线程
        void start();

        // 核心：每次有新连接时，调用此方法获取一个处理该连接的 EventLoop
        EventLoop* getNextLoop();
    };
} // eirian

#endif //EIRIAN_EVENTLOOPTHREADPOOL_HPP
