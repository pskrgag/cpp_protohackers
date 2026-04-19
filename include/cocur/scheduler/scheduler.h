/*
 * brief: Coroutine scheduler
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <atomic>
#include <cocur/scheduler/context.h>
#include <cocur/scheduler/policy/policy.h>

namespace cocur {

template <typename Policy = detail::RoundRobin>
class Scheduler {
public:
    Scheduler() : contexts_() {
        for (std::uint16_t i = 0; i < numContexts(); ++i) {
            contexts_.emplace_back(i);
        }

        policy_.init(numContexts());
    }

    void spawn(Task<> &&task) {
        auto ctx = policy_.pickContext();
        auto task_handle = task.takeOwnership();

        task_handle.promise().on_complete_ = [&]() {
            auto left = active_tasks_.fetch_sub(1, std::memory_order_relaxed);
            if (left == 1) {
                active_tasks_.notify_one();
            }
        };

        active_tasks_.fetch_add(1, std::memory_order_relaxed);
        contexts_[ctx].spawn(task_handle);
    }

    void runToTheEnd() {
        while (true) {
            auto current = active_tasks_.load(std::memory_order_relaxed);

            active_tasks_.wait(current, std::memory_order_relaxed);
            if (active_tasks_.load(std::memory_order_relaxed) == 0)
                break;
        }
    }

private:
    static size_t numContexts() {
        return std::thread::hardware_concurrency();
    }

    std::atomic<unsigned> active_tasks_;
    Policy policy_;
    std::deque<Context> contexts_;
};

}; // namespace cocur
