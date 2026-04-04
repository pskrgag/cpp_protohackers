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
    static int connect(const std::string &server_addr);

    TcpClient(int fd, IOEngine &engine) : Socket(fd, engine) {
    }

public:
    friend class TcpListner;

    TcpClient(const std::string &to, IOEngine &engine) : Socket(connect(to), engine) {
    }
};
} // namespace cocur
