/*
 * brief:  Socket interface
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/net/socket.h>
#include <cocur/uring/task.h>
#include <memory>

namespace cocur {

class IOEngine;

class TcpClient;

class TcpListner : public Socket {
    static int create_and_listen(const std::string &server_addr);

public:
    TcpListner(const std::string &to, IOEngine &engine) : Socket(create_and_listen(to), engine) {
    }

    Task<std::shared_ptr<TcpClient>> accept();
};

class TcpClient : public Socket {
    TcpClient(int fd, IOEngine &engine) : Socket(fd, engine) {
    }

    TcpClient(Socket &&socket) : Socket(std::move(socket)) {
    }

public:
    friend class TcpListner;

    static Task<TcpClient> connect(const std::string &to, IOEngine &engine);
};
} // namespace cocur
