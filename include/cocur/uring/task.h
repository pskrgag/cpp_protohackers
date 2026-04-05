/*
 * brief:  Co-routine task
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <cassert>
#include <coroutine>
#include <functional>
#include <optional>

namespace cocur {

class IOEngine;

template <typename T>
class Task;

template <typename T>
class Promise;

namespace detail {
template <typename T = void>
struct PromiseBase {
    std::coroutine_handle<> caller_;
    std::function<void(void)> on_complete_;

    Task<T> get_return_object();

    std::suspend_always initial_suspend() {
        return std::suspend_always{};
    }

    void unhandled_exception() {
        std::terminate();
    }
};

template <typename T = void>
class Promise final : public PromiseBase<T> {
    std::optional<T> result_;

    friend class Task<T>;

public:
    Task<T> get_return_object();

    auto final_suspend() noexcept {
        struct Awaiter {
            bool await_ready() noexcept {
                return false;
            }

            void await_suspend(std::coroutine_handle<Promise<T>> h) noexcept {
                if (h.promise().on_complete_) {
                    h.promise().on_complete_();
                }

                if (h.promise().caller_)
                    h.promise().caller_.resume();
            }

            void await_resume() noexcept {
            }
        };

        return Awaiter{};
    }

    void return_value(T result) {
        result_ = std::move(result);
    }
};

template <>
class Promise<void> final : public PromiseBase<void> {
public:
    Task<void> get_return_object();

    auto final_suspend() noexcept {
        struct Awaiter {
            bool await_ready() noexcept {
                return false;
            }

            void await_suspend(std::coroutine_handle<Promise<void>> h) noexcept {
                if (h.promise().on_complete_) {
                    h.promise().on_complete_();
                }

                if (h.promise().caller_)
                    h.promise().caller_.resume();
            }

            void await_resume() noexcept {
            }
        };

        return Awaiter{};
    }

    void return_void() {
    }
};
} // namespace detail

template <typename T = void>
class Task {
    std::coroutine_handle<detail::Promise<T>> handle_ = std::noop_coroutine;

    friend class detail::Promise<T>;
    friend class IOEngine;

    Task(std::coroutine_handle<detail::Promise<T>> handle) : handle_(handle) {
    }

public:
    using promise_type = detail::Promise<T>;

    bool await_ready() {
        // Pass to the waiter
        return false;
    }

    T await_resume() {
        if constexpr (!std::same_as<T, void>) {
            return std::move(*handle_.promise().result_);
        }
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        handle_.promise().caller_ = caller;
        return handle_;
    }

    void resume() {
        handle_.resume();
    }

    std::coroutine_handle<detail::Promise<T>> &handle() {
        return handle_;
    }
};

namespace detail {

template <typename T>
inline Task<T> Promise<T>::get_return_object() {
    return Task<T>(std::coroutine_handle<Promise<T>>::from_promise(*this));
}

inline Task<> Promise<void>::get_return_object() {
    return Task<>(std::coroutine_handle<Promise<>>::from_promise(*this));
}

} // namespace detail
} // namespace cocur
