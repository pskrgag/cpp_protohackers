/*
 * brief: Scheduler policy
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#include <atomic>
#include <stddef.h>

namespace cocur::detail {

class Policy {
public:
    virtual void init(size_t numCtx) noexcept = 0;
    virtual size_t pickContext() noexcept = 0;
};

class RoundRobin : public Policy {
public:
    virtual void init(size_t numCtx) noexcept override {
        numCtx_ = numCtx;
    }

    virtual size_t pickContext() noexcept override {
        return counter_.fetch_add(1, std::memory_order_relaxed) % numCtx_;
    }

private:
    size_t numCtx_ = 0;
    std::atomic<size_t> counter_ = 0;
};

}; // namespace cocur::detail
