/*
 * brief:  Network helpers
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace cocur {

struct Address {
    struct in_addr address;
    std::uint16_t port; // in BE
};

/// a.b.c.d:n
static inline Address string_to_address(const std::string &address) {
    char *endp;
    const char *start = address.data();
    std::uint32_t addr = 0;

    for (int i = 0; i < 4; ++i) {
        if (start >= address.data() + address.size())
            throw std::runtime_error("wrong string");

        unsigned long part = strtoul(start, &endp, 10);

        if (part == std::numeric_limits<unsigned long>::max() || endp == start)
            throw std::runtime_error("wrong string");

        if (part > 255)
            throw std::runtime_error("wrong string address part");

        addr |= (part << (i * 8));
        start = endp + 1;
    }

    unsigned long port = strtoul(start, &endp, 10);
    if (port == std::numeric_limits<unsigned long>::max() || endp == start)
        throw std::runtime_error("wrong string");

    if (port > (unsigned long)std::numeric_limits<std::uint16_t>::max())
        throw std::runtime_error("wrong port");

    return {{addr}, htons((std::uint16_t)port)};
}

} // namespace cocur
