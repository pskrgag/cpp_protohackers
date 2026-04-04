/*
 * brief:  Network waiters
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/net/socket.h>
#include <cocur/uring/engine.h>
#include <errno.h>

namespace cocur {
namespace detail {

template <typename Call = void>
class AsyncSyscall {
public:
    AsyncSyscall() : sleeping_(false) {};

    bool await_ready() const noexcept {
        // Don't block on the first call
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle) noexcept {
        int res = static_cast<Call *>(this)->Call();

        if (res == -1 && errno == EWOULDBLOCK) {
            sleeping_ = true;
            handle_ = handle;
            static_cast<Call *>(this)->PrepareSleep(handle, this);
            return true;
        }

        return_value_ = res;
        return false;
    }

    ssize_t await_resume() {
        return return_value_;
    }

    void resume(ssize_t result) {
        return_value_ = result;
        handle_.resume();
    }

private:
    bool sleeping_;
    ssize_t return_value_;
    std::coroutine_handle<> handle_ = std::noop_coroutine();
};

class Accept : public AsyncSyscall<Accept> {
public:
    Accept(const Socket &socket) : socket_(socket) {
    }

    int Call() noexcept {
        return ::accept(socket_.fd(), nullptr, nullptr);
    }

    void PrepareSleep(std::coroutine_handle<> handle, AsyncSyscall<Accept> *parent) noexcept {
        socket_.engine_.attachAccept(socket_, parent);
    }

private:
    const Socket &socket_;
};

class Read : public AsyncSyscall<Read> {
public:
    Read(const Socket &socket, std::span<std::byte> &span) : socket_(socket), span_(span) {
    }

    int Call() noexcept {
        return ::read(socket_.fd(), span_.data(), span_.size());
    }

    void PrepareSleep(std::coroutine_handle<> handle, AsyncSyscall<Read> *parent) noexcept {
        socket_.engine_.attachRead(socket_, span_.data(), span_.size(), parent);
    }

private:
    const Socket &socket_;
    std::span<std::byte> &span_;
};

class Write : public AsyncSyscall<Write> {
public:
    Write(const Socket &socket, const std::span<std::byte> &span) : socket_(socket), span_(span) {
    }

    int Call() noexcept {
        return ::write(socket_.fd(), span_.data(), span_.size());
    }

    void PrepareSleep(std::coroutine_handle<> handle, AsyncSyscall<Write> *parent) noexcept {
        socket_.engine_.attachWrite(socket_, span_.data(), span_.size(), parent);
    }

private:
    const Socket &socket_;
    const std::span<std::byte> &span_;
};

} // namespace detail
} // namespace cocur
