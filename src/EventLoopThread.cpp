    //
    // Created by xin on 2026/3/12.
    //

    #include "eirian/EventLoopThread.hpp"

    #include "eirian/EventLoop.hpp"
    #include "eirian/logger.hpp"

    namespace eirian {
        EventLoopThread::~EventLoopThread() {
            exiting_ =true;
            if (loop_ !=nullptr) {
                // TODO: 在这里通知子线程的 EventLoop 退出 (后续我们可以加一个 quit() 方法)
                // loop_->quit();
            }
            if (thread_.joinable()) {
                this->thread_.join();
            }
        }

        EventLoop *EventLoopThread::startLoop() {
            std::promise<EventLoop*> promise{};
            std::future<EventLoop*> future=promise.get_future();
            this ->thread_ =std::thread([this,&promise]() {
                this->threadFunction(promise);
            });
            //主线程阻塞在这里，直到子线程的 EventLoop 创建完毕并 set_value
            EventLoop* loop=future.get();
            return loop;
        }

        void EventLoopThread::threadFunction(std::promise<EventLoop *> &promise) {
            // 1. 在子线程的栈上创建一个 EventLoop 对象
            EventLoop child_thread_loop;
            {
                std::lock_guard<std::mutex> lock{mutex_};
                //这里不会发生悬垂引用 ，因为child_thread_loop是个死循环child_thread_loop会一直有效不会消失
                // NOLINTNEXTLINE
                this->loop_ = &child_thread_loop;
            }
            // 2. 告诉主线程：“我的 loop 创建好了，你可以拿走指针了！”
            // NOLINTNEXTLINE
            promise.set_value(&child_thread_loop);

            // 3. 子线程开启死循环，开始干活！
            log::info("EventLoopThread 子线程引擎启动!");
            child_thread_loop.loop();

            // 4. 当 loop.loop() 退出时，清理现场
            std::lock_guard<std::mutex> lock{mutex_};
            loop_ = nullptr;
        }
    } // eirian