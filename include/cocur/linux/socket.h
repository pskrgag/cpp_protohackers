/*
 * brief:  Socket interface
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/linux/fd.h>
#include <cocur/net/helpers.h>
#include <cocur/scheduler/task.h>
#include <fcntl.h>
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
class ReadExact;
class Write;
class IOring;
} // namespace detail

template <typename T>
concept ByteRange = std::ranges::contiguous_range<T> && std::ranges::sized_range<T> &&
                    sizeof(std::ranges::range_value_t<T>) == 1;
template <typename T>
concept Pod = std::is_trivial_v<T> && std::is_standard_layout_v<T> && !std::is_array_v<T>;

class Socket : public Fd {
    int fd_;

    Task<ssize_t> sendImpl(std::span<const std::byte> span);
    Task<ssize_t> recvImpl(std::span<std::byte> span);
    Task<ssize_t> recvExactImpl(std::span<std::byte> span);

protected:
    explicit Socket(int fd) : fd_(fd) {
        int status = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        if (status != 0)
            throw std::runtime_error{"Failed to set socket to non-blocking mode"};
    }

    friend class detail::Accept;
    friend class detail::Read;
    friend class detail::Write;
    friend class detail::Connect;
    friend class detail::ReadExact;
    friend class IOring;

    friend class TcpListner;
    friend class IOEngine;

public:
    Socket(const Socket &) = delete;
    Socket operator=(const Socket &) = delete;

    virtual int fd() const  override {
        return fd_;
    }

    Socket() {
        fd_ = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (fd_ < 0)
            throw std::runtime_error{"Failed to open socket"};
    }

    Socket(Socket &&other) : fd_(other.fd_) {
        other.fd_ = -1;
    }

    Socket operator=(Socket &&other) {
        return Socket(std::move(other));
    }

    Task<std::vector<std::byte>> recv(void);
    Task<ssize_t> connect(struct sockaddr *addr, size_t size);

    template <Pod T>
    Task<ssize_t> recv(T &span) {
        auto *ptr = reinterpret_cast<std::byte *>(&span);
        const size_t len = sizeof(span);

        return recvExactImpl(std::span(ptr, len));
    }

    template <ByteRange T>
    Task<ssize_t> recv(T &span) {
        auto *ptr = reinterpret_cast<std::byte *>(std::ranges::data(span));
        const size_t len = std::ranges::size(span);

        return recvImpl(std::span(ptr, len));
    }

    template <ByteRange T>
    Task<ssize_t> recvExact(T &span) {
        auto *ptr = reinterpret_cast<std::byte *>(std::ranges::data(span));
        const size_t len = std::ranges::size(span);

        return recvExactImpl(std::span(ptr, len));
    }

    template <ByteRange T>
    Task<ssize_t> send(const T &span) {
        const auto *ptr = reinterpret_cast<const std::byte *>(std::ranges::data(span));
        const size_t len = std::ranges::size(span);

        return sendImpl(std::span(ptr, len));
    }

    template <Pod T>
    Task<ssize_t> send(const T &span) {
        const auto *ptr = reinterpret_cast<const std::byte *>(&span);
        const size_t len = sizeof(span);

        return sendImpl(std::span(ptr, len));
    }

    ~Socket() {
        ::close(fd_);
    }
};
} // namespace cocur
