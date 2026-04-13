/*
 * brief:  Eventfd wrapper
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#include <cstdint>
#include <stdexcept>
#include <sys/eventfd.h>
#include <unistd.h>

namespace cocur {
namespace detail {

class EventFd {
public:
    EventFd() {
        fd_ = eventfd(0, 0);
        if (fd_ < 0)
            throw std::runtime_error{"Failed to create an eventfd"};
    }

    void signal() {
        std::uint64_t val = 0xdeadbeef;
        ssize_t res;

        res = write(fd_, &val, sizeof(val));
        if (res != sizeof(val))
            throw std::runtime_error{"Failed signal eventfd"};
    }

    int fd() const {
        return fd_;
    }

    ~EventFd() {
        close(fd_);
    }

private:
    int fd_;
};

} // namespace detail
}; // namespace cocur
