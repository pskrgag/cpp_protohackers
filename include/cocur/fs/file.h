/*
 * brief:  File
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#include <cocur/linux/fd.h>
#include <cocur/linux/waiters.h>
#include <cocur/scheduler/task.h>
#include <span>

namespace cocur {
class File : public Fd {
public:
    virtual int fd() const {
        return fd_;
    }

    Task<ssize_t> read(std::span<std::byte> span) {
        auto res = co_await detail::Read{*this, span};
        co_return res;
    }

private:
    File(int fd) : fd_(fd) {
    }

    int fd_;
};
}; // namespace cocur
