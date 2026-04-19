/*
 * brief:  Per-thread context
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <cocur/scheduler/thread_info.h>
#include <cocur/uring/engine.h>
#include <thread>
#include <unistd.h>

namespace cocur {

class Context {
public:
    Context(std::uint16_t cpu) : engine_() {
        thread_ = std::jthread([cpu = cpu, this]() {
            pid_t pid = getpid();
            cpu_set_t cpu_set;

            CPU_ZERO(&cpu_set);
            CPU_SET(cpu, &cpu_set);

            int res = sched_setaffinity(pid, sizeof(cpu_set), &cpu_set);
            if (res != 0)
                throw std::runtime_error{"Failed to set thread affinity"};

            work_thread();
        });
    }

    void spawn(std::coroutine_handle<> task) {
        engine_.spawn(task);
        engine_.signal();
    }

    void stop() {
        stop_.store(true, std::memory_order_relaxed);
        engine_.signal();
    }

    ~Context() {
        stop();
        thread_.join();
    }

    IOEngine &engine() {
        return engine_;
    }

private:
    bool should_stop() {
        return stop_.load(std::memory_order_relaxed);
    }

    void work_thread() {
        detail::tinfo.context_ = this;

        while (!should_stop()) {
            while (engine_.hasJobs()) {
                engine_.executeOne();
            }

            engine_.waitIo();
        }

        detail::tinfo.context_ = nullptr;
    }

    std::atomic<bool> stop_ = false;
    IOEngine engine_;
    std::jthread thread_;
};

inline Context *current_context(void) {
    return detail::tinfo.context_;
}

}; // namespace cocur
