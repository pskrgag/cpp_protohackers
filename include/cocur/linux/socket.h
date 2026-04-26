/*
 * brief:  Socket interface
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/helpers/concepts.h>
#include <cocur/linux/fd.h>
#include <cocur/net/helpers.h>
#include <cocur/scheduler/task.h>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>

namespace cocur {

class Socket : public Fd {
    int fd_;

protected:
    explicit Socket(int fd) : fd_(fd) {
        int status = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        if (status != 0)
            throw std::runtime_error{"Failed to set socket to non-blocking mode"};
    }

    friend class TcpListner;

public:
    enum SocketType : int {
        Tcp = SOCK_STREAM,
        Udp = SOCK_DGRAM,
    };

    Socket(const Socket &) = delete;
    Socket operator=(const Socket &) = delete;

    virtual int fd() const override {
        return fd_;
    }

    Task<std::vector<std::byte>> readToTheEnd(void);

    Socket(SocketType type) {
        fd_ = socket(PF_INET, type | (int)SOCK_NONBLOCK, 0);
        if (fd_ < 0)
            throw std::runtime_error{"Failed to open socket"};
    }

    Socket(Socket &&other) : fd_(other.fd_) {
        other.fd_ = -1;
    }

    Socket operator=(Socket &&other) {
        return Socket(std::move(other));
    }

    ~Socket() {
        ::close(fd_);
    }
};

class ClientSocket : public Socket {
protected:
    explicit ClientSocket(int fd) : Socket(fd) {
    }

    Task<ssize_t> recvImpl(std::span<std::byte> span);
    Task<ssize_t> sendImpl(std::span<const std::byte> span);
    Task<ssize_t> recvExactImpl(std::span<std::byte> span);

    virtual bool retryRead() const {
        return true;
    }

public:
    ClientSocket(SocketType type) : Socket(type) {
    }

    ClientSocket(Socket &&other) : Socket(std::move(other)) {
    }

    Task<ssize_t> connect(struct sockaddr *addr, size_t size);

    template <Pod T>
    Task<ssize_t> recv(T &span) {
        auto *ptr = reinterpret_cast<std::byte *>(&span);
        const size_t len = sizeof(span);

        return recvExactImpl(std::span(ptr, len));
    }

    template <ByteRange T>
    Task<ssize_t> recvExact(T &span) {
        auto *ptr = reinterpret_cast<std::byte *>(std::ranges::data(span));
        const size_t len = std::ranges::size(span);

        return recvExactImpl(std::span(ptr, len));
    }

    template <ByteRange T>
    Task<ssize_t> recv(T &span) {
        auto *ptr = reinterpret_cast<std::byte *>(std::ranges::data(span));
        const size_t len = std::ranges::size(span);

        return recvImpl(std::span(ptr, len));
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
};
} // namespace cocur
