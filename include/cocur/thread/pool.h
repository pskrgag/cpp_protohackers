/*
 * brief:  Thread pool
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/uring/engine.h>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <unistd.h>

namespace cocur {

namespace detail {
class Job {
    std::function<void(void)> ptr_;

public:
    template <typename T>
    Job(T ptr) : ptr_(ptr) {};

    void run() {
        ptr_();
    }
};
} // namespace detail

class ThreadPool {
    std::vector<std::jthread> threads_;

    std::mutex pending_mutex_;
    std::deque<detail::Job> pending_jobs_;
    std::condition_variable wait_jobs_;
    std::atomic<bool> stop_;

    bool should_stop() {
        return stop_.load(std::memory_order_relaxed);
    }

    void work_thread() {
        while (!should_stop()) {
            std::unique_lock guard(pending_mutex_);

            wait_jobs_.wait(guard, [&]() { return pending_jobs_.size() != 0 || should_stop(); });

            if (should_stop())
                break;

            auto job = pending_jobs_.front();
            pending_jobs_.pop_front();
            guard.~unique_lock();

            job.run();
        }
    }

public:
    void schedule(detail::Job j) {
        std::unique_lock guard(pending_mutex_);

        pending_jobs_.push_back(j);
        wait_jobs_.notify_one();
    }

    ThreadPool() : pending_mutex_(), pending_jobs_(), wait_jobs_() {
        const auto processor_count = std::thread::hardware_concurrency();

        for (auto i = 0; i < processor_count; ++i) {
            auto t = std::jthread([id = i, this]() {
                pid_t pid = getpid();
                cpu_set_t cpu_set;

                CPU_ZERO(&cpu_set);
                CPU_SET(id, &cpu_set);

                int res = sched_setaffinity(pid, sizeof(cpu_set), &cpu_set);
                if (res != 0)
                    throw std::runtime_error{"Failed to set thread affinity"};

                work_thread();
            });

            threads_.push_back(std::move(t));
        }
    }

    ~ThreadPool() {
        stop_.store(true, std::memory_order_relaxed);
        wait_jobs_.notify_all();
    }
};
} // namespace cocur
