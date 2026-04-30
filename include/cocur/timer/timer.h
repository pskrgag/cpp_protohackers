/*
 * brief: Timer
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <chrono>
#include <cocur/linux/waiters.h>
#include <cocur/scheduler/task.h>
#include <sys/timerfd.h>

namespace cocur {

template <typename T, typename U>
cocur::Task<> sleepFor(std::chrono::duration<T, U> tm) {
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(tm);
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(tm);

    ns -= sec;

    co_await detail::Timeout{timespec{
        .tv_sec = sec.count(),
        .tv_nsec = ns.count(),
    }};
}
}; // namespace cocur
