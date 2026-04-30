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
    Read(const Fd &file, std::span<std::byte> span) : file_(file), span_(span) {
    }

protected:
    virtual bool call(ssize_t &res) noexcept override {
        res = ::read(file_.fd(), span_.data(), span_.size());
        return shouldWait(res);
    }

    virtual void prepareSleep(AsyncSyscall *parent) noexcept override {
        current_context()->engine().ring().attachRead(file_, span_.data(), span_.size(), parent);
    }

    const Fd &file_;
    std::span<std::byte> span_;
};

class Recv : public AsyncSyscall {
public:
    Recv(const Socket &socket, std::span<std::byte> span, struct sockaddr_in *addr = nullptr)
        : socket_(socket), span_(span), addr_(addr) {
    }

protected:
    virtual bool call(ssize_t &res) noexcept override {
        socklen_t len = sizeof(struct sockaddr_in);

        res =
            ::recvfrom(socket_.fd(), span_.data(), span_.size(), 0, (struct sockaddr *)addr_, &len);
        return shouldWait(res);
    }

    virtual void prepareSleep(AsyncSyscall *parent) noexcept override {
        iovec_.iov_base = span_.data();
        iovec_.iov_len = span_.size();
        hdr_.msg_iovlen = 1;

        hdr_.msg_iov = &iovec_;
        hdr_.msg_name = addr_;
        hdr_.msg_namelen = sizeof(*addr_);

        current_context()->engine().ring().attachRecv(socket_, &hdr_, parent);
    }

    struct sockaddr_in *addr_;
    const Socket &socket_;
    struct msghdr hdr_;
    struct iovec iovec_;
    std::span<std::byte> span_;
};

class Send : public AsyncSyscall {
public:
    Send(const Socket &socket, std::span<const std::byte> span, const struct sockaddr_in *addr)
        : socket_(socket), span_(span), addr_(addr) {
    }

protected:
    virtual bool call(ssize_t &res) noexcept override {
        res = ::sendto(socket_.fd(), span_.data(), span_.size(), 0, (struct sockaddr *)addr_,
                       sizeof(*addr_));
        return shouldWait(res);
    }

    virtual void prepareSleep(AsyncSyscall *parent) noexcept override {
        current_context()->engine().ring().attachSend(socket_, span_.data(), span_.size(),
                                                      (struct sockaddr *)addr_, sizeof(sockaddr_in),
                                                      parent);
    }

    const struct sockaddr_in *addr_;
    const Socket &socket_;
    std::span<const std::byte> span_;
};

class ReadExact : public Read {
public:
    ReadExact(const Socket &socket, std::span<std::byte> span, bool retry = false)
        : Read(socket, span), origSize_(span.size()), retry_(retry) {
    }

protected:
    virtual bool call(ssize_t &res) noexcept override {
        res = ::read(file_.fd(), span_.data(), span_.size());
        if (res == span_.size()) {
            return false;
        } else if (res > 0 && retry_) {
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
    bool retry_;
    size_t origSize_;
};

class Write : public AsyncSyscall {
public:
    Write(const Fd &socket, std::span<const std::byte> span)
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
    const Fd &socket_;
    std::span<const std::byte> span_;
};

class Connect : public AsyncSyscall {
public:
    Connect(const Socket &socket, struct sockaddr *addr, size_t size)
        : socket_(socket), addr_(addr), size_(size) {
    }

protected:
    virtual bool call(ssize_t &res) noexcept override {
        return true;
    }

    virtual void prepareSleep(AsyncSyscall *parent) noexcept override {
        current_context()->engine().ring().attachConnect(socket_, addr_, size_, parent);
    }

private:
    const Socket &socket_;
    struct sockaddr *addr_;
    size_t size_;
};

class Timeout : public AsyncSyscall {
public:
    Timeout(struct timespec ts) : ts_(ts) {
    }

    virtual bool call(ssize_t &res) noexcept override {
        return true;
    }

    virtual void prepareSleep(AsyncSyscall *parent) noexcept override {
        current_context()->engine().ring().attachTimout(ts_, parent);
    }

private:
    struct timespec ts_;
};

} // namespace detail
} // namespace cocur
