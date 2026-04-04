/*
 * brief:  Socket interface
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/net/helpers.h>
#include <fcntl.h>
#include <span>
#include <stdexcept>
#include <unistd.h>

namespace cocur {
class IOEngine;

template <typename T>
class Task;

// Allow waiters to set handle to resume
namespace detail {
class Accept;
class Read;
class Write;
} // namespace detail

class Socket {
    int fd_;

protected:
    explicit Socket(int fd, IOEngine &engine) : fd_(fd), engine_(engine) {
        int status = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        if (status != 0)
            throw std::runtime_error{"Failed to set socket to non-blocking mode"};
    }

    friend class detail::Accept;
    friend class detail::Read;
    friend class detail::Write;
    friend class TcpListner;
    friend class IOEngine;
    IOEngine &engine_;

    int fd() const {
        return fd_;
    }

public:
    Socket(const Socket &) = delete;
    Socket operator=(const Socket &) = delete;

    Socket(Socket &&other) : fd_(other.fd_), engine_(other.engine_) {
        if (&other.engine_ != &other.engine_)
            throw std::runtime_error("lol");

        other.fd_ = -1;
    }

    Socket operator=(Socket &&other) {
        return Socket(std::move(other));
    }

    Task<ssize_t> recv(std::span<std::byte> &span);
    Task<ssize_t> send(const std::span<std::byte> &span);

    ~Socket() {
        ::close(fd_);
    }
};
} // namespace cocur
