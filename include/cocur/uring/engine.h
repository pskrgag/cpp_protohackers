/*
 * brief:  io-uring wrapper
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/linux/eventfd.h>
#include <cocur/scheduler/task.h>
#include <cocur/scheduler/waiter.h>
#include <cocur/uring/uring.h>
#include <deque>

namespace cocur {

class IOEngine {
public:
    IOEngine() : ring_(1000), event_() {
        ring_.attachRead(event_, &event_buffer_, sizeof(event_buffer_), nullptr);
    }

    IOring &ring() noexcept {
        return ring_;
    }

    void waitIo() {
        ring_.wait([this](auto *data) {
            if (data->user_data) {
                auto waiter = reinterpret_cast<detail::AsyncSyscall *>(data->user_data);
                auto task = waiter->resume(data->res);

                if (task) {
                    pending_jobs_.push_back(*task);
                }
            }
        });
    }

    void spawn(std::coroutine_handle<> handle) {
        pending_jobs_.push_back(handle);
    }

    void signal() {
        event_.signal();
    }

    bool hasJobs() const {
        return pending_jobs_.size() > 0;
    }

    void executeOne() {
        assert(pending_jobs_.size() != 0);

        auto task = pending_jobs_.front();
        pending_jobs_.pop_front();

        // Task will be dropped by promise (I am not sure it's correct thing to do, but it does not
        // trigger any asan reports, so I will stick with that)
        task.resume();
    }

    ~IOEngine() {
        for (const auto &task : pending_jobs_) {
            task.destroy();
        }
    }

private:
    std::uint64_t event_buffer_;
    detail::EventFd event_;
    IOring ring_;
    std::deque<std::coroutine_handle<>> pending_jobs_;
};

} // namespace cocur
