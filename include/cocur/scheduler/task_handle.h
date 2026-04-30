/*
 * brief:  Task state
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <atomic>
#include <coroutine>
#include <functional>
#include <memory>

namespace cocur {

namespace detail {
struct TaskCanceled {};

struct TaskState {
    // Context it run on
    void *context_ = nullptr;

    // Active sleep in io_uring
    std::atomic<void *> active_call = nullptr;

    // Coroutine frame
    std::coroutine_handle<> coroutine_ = nullptr;

    // Coroutine task sleep on
    std::atomic<std::shared_ptr<detail::TaskState>> child_ = nullptr;

    // Parent coroutine that awaits this task
    std::weak_ptr<detail::TaskState> parent_;

    // Completion callback
    std::function<void(void)> on_complete_;

    // Cancel request
    std::atomic<bool> canceled_ = false;

    // Completion flag
    std::atomic<bool> completed_ = false;

    void completeOnce() {
        bool expected = false;

        if (completed_.compare_exchange_strong(expected, true, std::memory_order_release) &&
            on_complete_) {
            completed_.notify_all();
            on_complete_();
        }
    }

    void detachFromParent() {
        if (auto parent = parent_.lock()) {
            std::shared_ptr<detail::TaskState> expected;

            do {
                expected = parent->child_.load(std::memory_order_acquire);
                if (expected.get() != this)
                    return;
            } while (!parent->child_.compare_exchange_weak(
                expected, nullptr, std::memory_order_release, std::memory_order_acquire));
        }
    }
};
}; // namespace detail

class TaskHandle {
public:
    void cancel();

    void join() {
        bool old = false;
        state_->completed_.wait(old, std::memory_order_acquire);
    }

    TaskHandle(std::shared_ptr<detail::TaskState> state) : state_(state) {
    }

private:
    std::shared_ptr<detail::TaskState> state_;
};

} // namespace cocur
