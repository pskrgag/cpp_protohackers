/*
 * brief:  io-uring wrapper
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/net/socket.h>
#include <liburing.h>
#include <memory>
#include <stdexcept>

namespace cocur {
class IOring {
public:
    explicit IOring(size_t ring_size) {
        auto res = io_uring_queue_init(ring_size, &ring_, IORING_SETUP_SQPOLL);
        if (res < 0) {
            throw std::runtime_error("Failed to initializing io_uring: " + std::to_string(res));
        }
    }

    IOring(const IOring &) = delete;
    IOring &operator=(const IOring &) = delete;
    IOring(IOring &&) = delete;
    IOring &operator=(IOring &&) = delete;

    ~IOring() {
        io_uring_queue_exit(&ring_);
    }

    void attachAccept(int fd, void *data) {
        auto sqe = allocate_sqe(data);

        io_uring_prep_accept(sqe, fd, nullptr, nullptr, 0);
        io_uring_submit(&ring_);
    }

    void attachWrite(int fd, const void *buffer, size_t size, void *data) {
        auto sqe = allocate_sqe(data);

        io_uring_prep_write(sqe, fd, buffer, size, 0);
        io_uring_submit(&ring_);
    }

    void attachRead(int fd, void *buffer, size_t size, void *data) {
        auto sqe = allocate_sqe(data);

        io_uring_prep_read(sqe, fd, buffer, size, 0);
        io_uring_submit(&ring_);
    }

    struct io_uring_cqe wait(void) {
        struct io_uring_cqe *cqes;
        int res = io_uring_wait_cqes(&ring_, &cqes, 1, nullptr, nullptr);
        if (res < 0) {
            throw std::runtime_error("Failed to wait" + std::to_string(res));
        }

        auto copy = *cqes;
        io_uring_cqe_seen(&ring_, cqes);
        return copy;
    }

private:
    template <typename T>
    struct io_uring_sqe *allocate_sqe(T *data) {
        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring_);

        if (!sqe)
            throw std::runtime_error("Out of entries");

        io_uring_sqe_set_data(sqe, (void *)data);
        return sqe;
    }

    struct io_uring ring_;
    std::unique_ptr<struct io_uring_cqe[]> cqes_;
};

} // namespace cocur
