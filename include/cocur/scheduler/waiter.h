/*
 * brief:  Network waiters
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <coroutine>
#include <sys/types.h>
#include <optional>

namespace cocur::detail {

class AsyncSyscall {
public:
    AsyncSyscall() {};

    bool await_ready() const noexcept {
        // Don't block on the first call
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle) noexcept {
        ssize_t res;
        bool sleep = call(res);

        if (sleep) {
            handle_ = handle;
            prepareSleep(this);
            return true;
        }

        return_value_ = res;
        return false;
    }

    ssize_t await_resume() {
        return return_value_;
    }

    std::optional<std::coroutine_handle<>> resume(ssize_t result) {
        if (isFinished(result)) {
            return_value_ = result;
            return handle_;
        }

        return {};
    }

protected:
    // Actual asynchronous syscall
    virtual bool call(ssize_t &res) noexcept = 0;

    // Prepares io_uring sleep
    virtual void prepareSleep(AsyncSyscall *parent) noexcept = 0;

    // Called when syscall returns. If return true then syscall is finished. Otherwise syscall was
    // resubmitted
    virtual bool isFinished(ssize_t &res) noexcept {
        return true;
    };

private:
    ssize_t return_value_;
    std::coroutine_handle<> handle_ = std::noop_coroutine();
};
}; // namespace cocur::detail
