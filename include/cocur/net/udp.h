/*
 * brief:  UDP sockets
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/linux/socket.h>
#include <cocur/linux/waiters.h>
#include <cocur/net/endpoint.h>
#include <cocur/net/helpers.h>
#include <cocur/scheduler/task.h>

namespace cocur {

class UdpListner;

class UdpClient : public ClientSocket {
public:
    static Task<UdpClient> connect(const std::string &to) {
        auto addr = Endpoint(to);

        ClientSocket sock(Socket::SocketType::Udp);

        int res = co_await sock.connect((struct sockaddr *)addr.addr(), addr.addrlen());
        if (res < 0)
            throw std::runtime_error("Failed to connect " + std::to_string(errno));

        co_return UdpClient(std::move(sock));
    }

    friend class UdpListner;

protected:
    virtual bool retryRead() const override {
        return false;
    }

private:
    UdpClient(Socket &&socket) : ClientSocket(std::move(socket)) {
    }

    UdpClient(int fd) : ClientSocket(fd) {
    }
};

class UdpListner : public Socket {
public:
    UdpListner(const std::string &to) : Socket(create_listner(to, SOCK_DGRAM)) {
    }

    template <Pod T>
    Task<void> recvFrom(T &span, Endpoint &from) {
        ssize_t read = co_await detail::Recv{*this, span, from.addr()};
        if (read != span.len())
            std::runtime_error{"Failed to send data"};
    }

    template <ByteRange T>
    Task<ssize_t> recvFrom(T &span, Endpoint &from) {
        ssize_t read = co_await detail::Recv{*this, span, from.addr()};
        co_return read;
    }

    // template <Pod T>
    // Task<ssize_t> sendTo(const T &span, const Endpoint &to) {
    //     ssize_t read = co_await detail::Send{*this, span, to.addr()};
    //     co_return read;
    // }

    template <ByteRange T>
    Task<void> sendTo(const T &span, const Endpoint &to) {
        const auto *ptr = reinterpret_cast<const std::byte *>(std::ranges::data(span));
        const size_t len = std::ranges::size(span);

        ssize_t sent = co_await detail::Send{*this, std::span(ptr, len), to.addr()};
        if (sent != len)
            std::runtime_error{"Failed to send data"};
    }
};

} // namespace cocur
