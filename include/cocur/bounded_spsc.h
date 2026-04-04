/*
 * brief:  Bounded single producer single consumer queue.
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <optional>
#include <utility>

namespace cocur {

template <typename T, std::size_t N>
class BoundedSpsc {
    T array[N + 1];

    alignas(64) std::atomic<std::size_t> consumer = 0;
    alignas(64) std::atomic<std::size_t> producer = 0;

    bool is_full() {
        return ((producer.load(std::memory_order_relaxed) + 1) % (N + 1)) ==
               (consumer.load(std::memory_order_relaxed) % (N + 1));
    }

    bool is_empty() {
        return (producer.load(std::memory_order_acquire) % (N + 1)) ==
               (consumer.load(std::memory_order_relaxed) % (N + 1));
    }

public:
    std::size_t capacity() const {
        return N;
    }

    template <typename U>
    bool push(U &&t) {
        if (is_full())
            return false;

        std::size_t push_idx = producer.load(std::memory_order_relaxed) % (N + 1);

        array[push_idx] = std::move(t);
        producer.fetch_add(1, std::memory_order_release);
        return true;
    }

    std::optional<T> pop(void) {
        if (is_empty())
            return std::nullopt;

        std::size_t pop_idx = consumer.fetch_add(1, std::memory_order_relaxed) % (N + 1);
        return std::move(array[pop_idx]);
    }
};

} // namespace cocur
