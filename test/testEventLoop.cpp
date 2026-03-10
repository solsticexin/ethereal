#include "eirian/Channel.hpp"
#include "eirian/EventLoop.hpp"

#include <iostream>
#include <unistd.h>

int main() {
    int fds[2] = {-1, -1};
    if (pipe(fds) != 0) {
        std::cerr << "pipe failed\n";
        return 1;
    }

    eirian::EventLoop loop;
    eirian::Channel channel(&loop, fds[0]);

    channel.enableReading();
    loop.removeChannel(&channel);

    close(fds[0]);
    close(fds[1]);

    std::cout << "testEventLoop passed\n";
    return 0;
}
