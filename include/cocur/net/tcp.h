/*
 * brief:  Tcp sockets
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/helpers/concepts.h>
#include <cocur/linux/socket.h>
#include <cocur/linux/waiters.h>
#include <cocur/net/endpoint.h>
#include <cocur/scheduler/task.h>
#include <memory>

namespace cocur {

class TcpClient;

class TcpSocket : public ClientSocket {
public:
protected:
    TcpSocket(int fd) : ClientSocket(fd) {
    }

    TcpSocket(Socket &&socket) : ClientSocket(std::move(socket)) {
    }
};

class TcpClient : public TcpSocket {
    TcpClient(Socket &&socket) : TcpSocket(std::move(socket)) {
    }

    friend class TcpListner;
    TcpClient(int fd) : TcpSocket(fd) {
    }

public:
    static Task<TcpClient> connect(std::string_view server_addr) {
        auto addr = Endpoint(server_addr);

        ClientSocket sock(Socket::SocketType::Tcp);

        int res = co_await sock.connect((struct sockaddr *)addr.addr(), addr.addrlen());
        if (res < 0)
            throw std::runtime_error("Failed to connect " + std::to_string(errno));

        co_return TcpClient(std::move(sock));
    }
};

class TcpListner : public TcpSocket {
public:
    TcpListner(const std::string &to) : TcpSocket(create_listner(to, SOCK_STREAM)) {
    }

    Task<std::shared_ptr<TcpClient>> accept() {
        int fd = co_await detail::Accept{*this};
        co_return std::shared_ptr<TcpClient>(new TcpClient(fd));
    }
};
} // namespace cocur
