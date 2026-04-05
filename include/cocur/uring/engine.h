/*
 * brief:  io-uring wrapper
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/linux/eventfd.h>
#include <cocur/uring/task.h>
#include <cocur/uring/uring.h>
#include <coroutine>

namespace cocur {

IOEngine *current_engine();

class Socket;

class IOEngine {
    IOring ring_;
    std::atomic<size_t> active_tasks_;
    detail::EventFd event_;

public:
    IOEngine() : ring_(1000), active_tasks_(0) {
    }

    void attachRead(const Socket &socket, std::byte *buffer, size_t size, void *data);
    void attachWrite(const Socket &socket, const std::byte *buffer, size_t size, void *data);
    void attachAccept(const Socket &socket, void *data);
    void attachConnect(const Socket &socket, struct sockaddr *addr, size_t size, void *data);

    void block_on(Task<void> task);
    void schedule(std::coroutine_handle<> handle);
    void spawn(Task<void> task);
};
} // namespace cocur
