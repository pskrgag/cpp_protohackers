/*
 * brief:  Network helpers
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <arpa/inet.h>
#include <cocur/net/endpoint.h>
#include <format>
#include <limits>

namespace cocur {

static int create_listner(std::string_view address, int proto) {
    auto addr = Endpoint(address);

    int fd_ = socket(PF_INET, proto | SOCK_NONBLOCK, 0);
    if (fd_ < 0)
        throw std::runtime_error("Failed to create a socket");

    int enable = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");

    int res = bind(fd_, (struct sockaddr *)addr.addr(), addr.addrlen());
    if (res < 0)
        throw std::runtime_error(std::format("Failed to bind socket {}", errno));

    if (proto == SOCK_STREAM) {
        res = listen(fd_, std::numeric_limits<int>::max());
        if (res < 0)
            throw std::runtime_error(std::format("Failed to listen to socket {}", errno));
    }

    return fd_;
}

} // namespace cocur
