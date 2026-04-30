/*
 * brief:  io-uring wrapper
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cocur/linux/fd.h>
#include <functional>
#include <liburing.h>
#include <memory>
#include <print>
#include <stdexcept>

namespace cocur {

class IOring {
public:
    explicit IOring(size_t ring_size) : io_count_(0) {
        auto res = io_uring_queue_init(ring_size, &ring_, 0);
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

    void attachAccept(const Fd &socket, void *data) {
        auto sqe = allocate_sqe(data);

        io_uring_prep_accept(sqe, socket.fd(), nullptr, nullptr, 0);
    }

    void attachConnect(const Fd &socket, const struct sockaddr *addr, size_t size, void *data) {
        auto sqe = allocate_sqe(data);

        io_uring_prep_connect(sqe, socket.fd(), addr, size);
    }

    void attachTimout(struct timespec ts, void *data) {
        auto sqe = allocate_sqe(data);
        auto kern_ts = (__kernel_timespec){.tv_sec = ts.tv_sec, .tv_nsec = ts.tv_nsec};

        io_uring_prep_timeout(sqe, &kern_ts, 1, 0);
    }

    void attachWrite(const Fd &socket, const void *buffer, size_t size, void *data) {
        auto sqe = allocate_sqe(data);

        io_uring_prep_write(sqe, socket.fd(), buffer, size, 0);
    }

    void attachRead(const Fd &socket, void *buffer, size_t size, void *data) {
        auto sqe = allocate_sqe(data);

        io_uring_prep_read(sqe, socket.fd(), buffer, size, 0);
    }

    void attachRecv(const Fd &socket, struct msghdr *hdr, void *data) {
        auto sqe = allocate_sqe(data);

        io_uring_prep_recvmsg(sqe, socket.fd(), hdr, 0);
    }

    void attachSend(const Fd &socket, const void *buffer, size_t size, struct sockaddr *addr,
                    size_t addrSize, void *data) {
        auto sqe = allocate_sqe(data);

        io_uring_prep_sendto(sqe, socket.fd(), buffer, size, 0, addr, addrSize);
    }

    void cancel(void *data) {
        auto sqe = allocate_sqe(nullptr);

        io_uring_prep_cancel(sqe, data, 0);
        io_uring_submit(&ring_);
    }

    void wait(std::function<void(struct io_uring_cqe *)> cb) {
        struct io_uring_cqe *cqes[32] = {}, *cqe;
        unsigned head;
        int count;

        if (io_count_ == 0)
            return;

        do {
            int res = io_uring_submit_and_wait(&ring_, 1);
            if (res < 0 && !(res == -EINTR || res == -ETIME)) {
                throw std::runtime_error("Failed to wait " + std::to_string(count));
            }

            count = io_uring_peek_batch_cqe(&ring_, cqes, 32);
            if (count > 0)
                break;
        } while (true);

        for (auto i = 0; i < count; ++i) {
            cb(cqes[i]);
            io_count_--;
        }

        io_uring_cq_advance(&ring_, count);
    }

private:
    struct io_uring_sqe *allocate_sqe(void *data) {
        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring_);

        if (!sqe)
            throw std::runtime_error("Out of entries");

        io_count_++;
        io_uring_sqe_set_data(sqe, (void *)data);
        return sqe;
    }

    struct io_uring ring_;
    unsigned io_count_;
    std::unique_ptr<struct io_uring_cqe[]> cqes_;
};

} // namespace cocur
