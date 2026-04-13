/*
 * brief:  Socket interface
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/linux/socket.h>
#include <cocur/scheduler/task.h>
#include <memory>

namespace cocur {

class TcpClient;

class TcpListner : public Socket {
    static int create_and_listen(const std::string &server_addr);

public:
    TcpListner(const std::string &to) : Socket(create_and_listen(to)) {
    }

    Task<std::shared_ptr<TcpClient>> accept();
};

class TcpClient : public Socket {

    TcpClient(Socket &&socket) : Socket(std::move(socket)) {
    }

    friend class TcpListner;

public:
    TcpClient(int fd) : Socket(fd) {
    }
    static Task<TcpClient> connect(const std::string &to);
};
} // namespace cocur
