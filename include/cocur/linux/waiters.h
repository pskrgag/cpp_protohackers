/*
 * brief:  Network waiters
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/linux/socket.h>
#include <cocur/scheduler/context.h>
#include <cocur/scheduler/waiter.h>
#include <cocur/uring/engine.h>
#include <errno.h>

namespace cocur {
namespace detail {
static bool shouldWait(ssize_t res) {
    return res == -1 && (errno == EWOULDBLOCK || errno == EINPROGRESS);
}

class Accept : public AsyncSyscall {
public:
    Accept(const Socket &socket) : socket_(socket) {
    }

protected:
    virtual bool call(ssize_t &res) noexcept override {
        res = ::accept(socket_.fd(), nullptr, nullptr);
        return shouldWait(res);
    }

    virtual void prepareSleep(AsyncSyscall *parent) noexcept override {
        current_context()->engine().ring().attachAccept(socket_, parent);
    }

private:
    const Socket &socket_;
};

class Read : public AsyncSyscall {
public:
    Read(const Socket &socket, std::span<std::byte> span) : socket_(socket), span_(span) {
    }

protected:
    virtual bool call(ssize_t &res) noexcept override {
        res = ::read(socket_.fd(), span_.data(), span_.size());
        return shouldWait(res);
    }

    virtual void prepareSleep(AsyncSyscall *parent) noexcept override {
        current_context()->engine().ring().attachRead(socket_, span_.data(), span_.size(), parent);
    }

    const Socket &socket_;
    std::span<std::byte> span_;
};

class ReadExact : public Read {
public:
    ReadExact(const Socket &socket, std::span<std::byte> span)
        : Read(socket, span), origSize_(span.size()) {
    }

protected:
    virtual bool call(ssize_t &res) noexcept override {
        res = ::read(socket_.fd(), span_.data(), span_.size());
        if (res == span_.size()) {
            return false;
        } else if (res > 0) {
            span_ = span_.subspan(res);
            return true;
        } else if (res == 0) {
            res = -1;
            return false;
        } else {
            return shouldWait(res);
        }
    }

    virtual bool isFinished(ssize_t &res) noexcept override {
        if (res > 0 && res != span_.size()) {
            assert(res <= span_.size());

            span_ = span_.subspan(res);
            prepareSleep(this);
            return false;
        }

        res = origSize_;
        return true;
    };

private:
    size_t origSize_;
};

class Write : public AsyncSyscall {
public:
    Write(const Socket &socket, std::span<const std::byte> span)
        : socket_(socket), span_(span), origSize_(span.size()) {
    }

protected:
    virtual bool call(ssize_t &res) noexcept override {
        res = ::write(socket_.fd(), span_.data(), span_.size());

        // TODO: debug
        assert(res == span_.size());

        if (res == span_.size()) {
            return false;
        } else if (res > 0) {
            span_ = span_.subspan(res);
            return true;
        } else {
            return shouldWait(res);
        }
    }

    virtual void prepareSleep(AsyncSyscall *parent) noexcept override {
        current_context()->engine().ring().attachWrite(socket_, span_.data(), span_.size(), parent);
    }

    virtual bool isFinished(ssize_t &res) noexcept override {
        if (res > 0 && res != span_.size()) {
            assert(res <= span_.size());

            span_ = span_.subspan(res);
            prepareSleep(this);
            return false;
        }

        res = origSize_;
        return true;
    };

private:
    size_t origSize_;
    const Socket &socket_;
    std::span<const std::byte> span_;
};

class Connect : public AsyncSyscall {
public:
    Connect(const Socket &socket, struct sockaddr *addr, size_t size)
        : socket_(socket), addr_(addr), size_(size) {
    }

protected:
    virtual bool call(ssize_t &res) noexcept override {
        res = ::connect(socket_.fd(), addr_, size_);
        return shouldWait(res);
    }

    virtual void prepareSleep(AsyncSyscall *parent) noexcept override {
        current_context()->engine().ring().attachConnect(socket_, addr_, size_, parent);
    }

private:
    const Socket &socket_;
    struct sockaddr *addr_;
    size_t size_;
};

} // namespace detail
} // namespace cocur
