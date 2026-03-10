#include "eirian/Channel.hpp"
#include "eirian/EventLoop.hpp"

#include <cassert>
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>

int main() {
    int fds[2] = {-1, -1};
    if (pipe(fds) != 0) {
        std::cerr << "pipe failed\n";
        return 1;
    }

    eirian::EventLoop loop;
    eirian::Channel channel(&loop, fds[0]);

    bool read_called = false;
    bool close_called = false;

    channel.setReadCallback([&]() { read_called = true; });
    channel.setCloseCallback([&]() { close_called = true; });

    channel.enableReading();

    channel.setRevents(EPOLLIN);
    channel.handleEvent();
    assert(read_called);
    assert(!close_called);

    eirian::Channel hup_channel(&loop, fds[0]);
    hup_channel.setCloseCallback([&]() { close_called = true; });
    hup_channel.setRevents(EPOLLHUP);
    hup_channel.handleEvent();
    assert(close_called);

    close(fds[0]);
    close(fds[1]);

    std::cout << "testChannel passed\n";
    return 0;
}
