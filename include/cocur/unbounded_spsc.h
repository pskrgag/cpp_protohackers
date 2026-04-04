/*
 * brief:  Unbounded single producer single consumer queue.
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <atomic>
#include <cassert>
#include <optional>
#include <utility>

namespace cocur {

template <typename T>
class UnboundedSpsc {
    struct Node {
        std::atomic<Node *> next;
        T val;
    };

    alignas(64) std::atomic<Node *> head;
    alignas(64) std::atomic<Node *> tail;
    alignas(64) std::atomic<Node *> freelist;

    bool is_empty() const {
        return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed);
    }

    void push_to_freelist(Node *node) {
        auto expected = freelist.load(std::memory_order_acquire);

        while (true) {
            node->next.store(expected, std::memory_order_relaxed);

            if (freelist.compare_exchange_weak(expected, node, std::memory_order_release,
                                               std::memory_order_acquire))
                break;
        }
    }

    Node *pop_from_freelist(void) {
        auto expected = freelist.load(std::memory_order_acquire);

        while (expected) {
            auto next = expected->next.load(std::memory_order_relaxed);

            if (freelist.compare_exchange_weak(expected, next, std::memory_order_release,
                                               std::memory_order_acquire)) {
                return expected;
            }
        }

        return nullptr;
    }

public:
    UnboundedSpsc(const UnboundedSpsc &) = delete;
    UnboundedSpsc operator=(const UnboundedSpsc &) = delete;

    UnboundedSpsc(UnboundedSpsc &&other)
        : head(other.head), tail(other.tail), freelist(other.freelist) {
        other.head = nullptr;
        other.tail = nullptr;
        other.freelist = nullptr;
    }

    UnboundedSpsc operator=(UnboundedSpsc &&other) {
        return UnboundedSpsc(other);
    }

    UnboundedSpsc() {
        Node *new_node = new Node;

        freelist = nullptr;
        head = tail = new_node;
    }

    template <typename U>
    void push(U &&val) {
        auto free = pop_from_freelist();
        Node *new_node = free ?: new Node;
        auto head_copy = head.load(std::memory_order_relaxed);

        new_node->val = std::move(val);
        head_copy->next.store(new_node, std::memory_order_release);

        head.store(new_node, std::memory_order_relaxed);
    }

    std::optional<T> pop(void) {
        if (is_empty())
            return std::nullopt;

        auto tail_copy = tail.load(std::memory_order_relaxed);
        auto next = tail_copy->next.load(std::memory_order_acquire);
        assert(next);

        auto res = std::move(next->val);
        tail.store(next, std::memory_order_relaxed);

        push_to_freelist(tail_copy);
        return res;
    }

    ~UnboundedSpsc() {
        if (head.load(std::memory_order_relaxed)) {
            while (pop())
                ;

            while (auto node = pop_from_freelist()) {
                delete node;
            }

            delete head;
        }
    }
};

} // namespace cocur
