//
// Created by xin on 2026/3/12.
//

#include "eirian/EventLoopThreadPool.hpp"
#include "eirian/EventLoop.hpp"
#include "eirian/EventLoopThread.hpp"
#include "eirian/logger.hpp"

namespace eirian {
    EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop)
        : baseLoop_(baseLoop) {}

    EventLoopThreadPool::~EventLoopThreadPool() = default;

    void EventLoopThreadPool::start() {
        started_ = true;

        for (int i = 0; i < numThreads_; ++i) {
            auto t = std::make_unique<EventLoopThread>();

            // 启动底层线程，并把线程里创建好的 EventLoop 指针收集起来
            loops_.push_back(t->startLoop());

            // 妥善保存线程对象，防止被析构
            threads_.push_back(std::move(t));
        }

        if (numThreads_ == 0) {
            log::info("EventLoopThreadPool 启动，当前为单线程模式(退化为 MainLoop)");
        } else {
            log::info("EventLoopThreadPool 启动，成功创建了 {} 个子线程(SubLoop)", numThreads_);
        }
    }

    EventLoop* EventLoopThreadPool::getNextLoop() {
        // 如果没有子线程，那就默认让主线程（老板自己）处理
        EventLoop* loop = baseLoop_;

        if (!loops_.empty()) {
            // 核心轮询算法 (Round-Robin)
            loop = loops_[next_];
            ++next_;
            if (next_ >= loops_.size()) {
                next_ = 0; // 绕回头部
            }
        }
        return loop;
    }
} // eirian
