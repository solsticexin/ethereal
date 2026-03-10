//
// Created by xin on 2026/3/9.
//
#include "eirian/Epoll.h"
#include "eirian/InetAddress.hpp"
#include "eirian/logger.hpp"
#include "eirian/socket.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <exception>
#include <optional>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {
int pick_server_port(const eirian::Socket& server_socket) {
    for (const std::vector<int> candidate_ports{18888, 18889, 18890, 18891, 18892}; const int port : candidate_ports) {
        try {
            server_socket.bind(eirian::InetAddress("127.0.0.1", static_cast<uint16_t>(port)));
            return port;
        } catch (const std::system_error&) {
            eirian::log::warn("port {} unavailable, try next", port);
        }
    }
    throw std::runtime_error("no available test port");
}
} // namespace

int main() {
    std::thread client_thread;
    try {
        eirian::Socket server_socket{};
        const int server_port = pick_server_port(server_socket);
        eirian::log::info("epoll test bind port={}", server_port);

        if (!server_socket.setNonBlocking()) {
            throw std::runtime_error("setNonBlocking failed");
        }
        server_socket.listen();

        eirian::Epoll epoll_{16};
        epoll_.add(server_socket.getFd(), EPOLLIN);
        eirian::log::info("epoll add fd={}, events=EPOLLIN", server_socket.getFd());

        client_thread = std::thread([server_port]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
            const int client_fd = ::socket(AF_INET, SOCK_STREAM, 0);
            if (client_fd == -1) {
                eirian::log::error("client socket create failed: {}", std::strerror(errno));
                return;
            }

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(static_cast<uint16_t>(server_port));
            ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

            if (::connect(client_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
                eirian::log::error("client connect failed: {}", std::strerror(errno));
                ::close(client_fd);
                return;
            }
            eirian::log::info("client connected to 127.0.0.1:{}", server_port);
            ::close(client_fd);
        });

        const auto events = epoll_.poll(2000);
        if (events.empty()) {
            throw std::runtime_error("epoll timeout: no events");
        }

        bool has_server_read_event = false;
        for (const auto& ev : events) {
            if (ev.data.fd == server_socket.getFd() && (ev.events & EPOLLIN) != 0U) {
                has_server_read_event = true;
                break;
            }
        }
        if (!has_server_read_event) {
            throw std::runtime_error("epoll returned events but no EPOLLIN on server fd");
        }
        eirian::log::info("epoll poll returned server readable event");

        eirian::InetAddress client_addr("0.0.0.0", 0);
        std::optional<eirian::Socket> accepted;
        for (int i = 0; i < 10 && !accepted.has_value(); ++i) {
            accepted = server_socket.accept(client_addr);
            if (!accepted.has_value()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
        if (!accepted.has_value()) {
            throw std::runtime_error("accept did not return a client socket");
        }
        eirian::log::info("accept success, client fd={}", accepted->getFd());

        epoll_.remove(server_socket.getFd());
        eirian::log::info("epoll remove fd={}", server_socket.getFd());

        if (client_thread.joinable()) {
            client_thread.join();
        }
        eirian::log::info("epoll test passed");
        return 0;
    } catch (const std::exception& e) {
        if (client_thread.joinable()) {
            client_thread.join();
        }
        eirian::log::error("epoll test failed: {}", e.what());
        return -1;
    }
}
