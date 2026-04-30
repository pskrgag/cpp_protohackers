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
#include <cocur/scheduler/task_handle.h>
#include <print>

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

    template <typename T>
    TaskHandle spawn(Task<T> &&task) {
        auto ctx = policy_.pickContext();
        auto task_handle = task.takeOwnership();
        auto state = std::make_shared<detail::TaskState>();

        state->context_ = &contexts_[ctx];
        state->coroutine_ = task_handle;
        task_handle.promise().state_ = state;
        state->on_complete_ = [&]() {
            auto left = active_tasks_.fetch_sub(1, std::memory_order_relaxed);
            if (left == 1) {
                active_tasks_.notify_one();
            }
        };

        active_tasks_.fetch_add(1, std::memory_order_relaxed);
        contexts_[ctx].spawn(task_handle);

        return TaskHandle(state);
    }

    void runToTheEnd() {
        while (true) {
            auto current = active_tasks_.load(std::memory_order_relaxed);
            if (current == 0)
                break;

            active_tasks_.wait(current, std::memory_order_relaxed);
            if (active_tasks_.load(std::memory_order_relaxed) == 0)
                break;
        }
    }

private:
    static size_t numContexts() {
        return std::thread::hardware_concurrency();
    }

    std::atomic<unsigned> active_tasks_ = 0;
    Policy policy_;
    std::deque<Context> contexts_;
};

}; // namespace cocur
