/*
 * brief:  Socket interface
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/net/helpers.h>
#include <cocur/uring/task.h>
#include <fcntl.h>
#include <print>
#include <span>
#include <stdexcept>
#include <unistd.h>

namespace cocur {
class IOEngine;

// Allow waiters to set handle to resume
namespace detail {
class Accept;
class Connect;
class Read;
class Write;
} // namespace detail

template <typename T>
concept ByteRange = std::ranges::contiguous_range<T> && std::ranges::sized_range<T> &&
                    sizeof(std::ranges::range_value_t<T>) == 1;

class Socket {
    int fd_;

    Task<ssize_t> sendImpl(std::span<const std::byte> span);

protected:
    explicit Socket(int fd, IOEngine &engine) : fd_(fd), engine_(engine) {
        int status = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        if (status != 0)
            throw std::runtime_error{"Failed to set socket to non-blocking mode"};
    }

    friend class detail::Accept;
    friend class detail::Read;
    friend class detail::Write;
    friend class detail::Connect;
    friend class TcpListner;
    friend class IOEngine;
    IOEngine &engine_;

    int fd() const {
        return fd_;
    }

public:
    Socket(const Socket &) = delete;
    Socket operator=(const Socket &) = delete;

    Socket(IOEngine &engine) : engine_(engine) {
        fd_ = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (fd_ < 0)
            throw std::runtime_error{"Failed to open socket"};
    }

    Socket(Socket &&other) : fd_(other.fd_), engine_(other.engine_) {
        if (&other.engine_ != &other.engine_)
            throw std::runtime_error("lol");

        other.fd_ = -1;
    }

    Socket operator=(Socket &&other) {
        return Socket(std::move(other));
    }

    Task<ssize_t> recv(std::span<std::byte> &span);
    Task<std::vector<std::byte>> recv(void);
    Task<ssize_t> connect(struct sockaddr *addr, size_t size);

    template <ByteRange T>
    Task<ssize_t> send(const T &span) {
        const auto *ptr = reinterpret_cast<const std::byte *>(std::ranges::data(span));
        const size_t len = std::ranges::size(span);

        return sendImpl(std::span(ptr, len));
    }

    ~Socket() {
        ::close(fd_);
    }
};
} // namespace cocur
