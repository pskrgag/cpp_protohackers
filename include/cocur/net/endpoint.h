/*
 * brief:  Network endpoint
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <arpa/inet.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <format>
#include <stdexcept>
#include <sys/socket.h>

namespace cocur {

class Endpoint {
public:
    Endpoint() : addr_() {};

    /// a.b.c.d:n
    Endpoint(std::string_view address) {
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

        addr_.sin_port = ::htons((uint16_t)port);
        addr_.sin_family = AF_INET;

        static_assert(sizeof(addr_.sin_addr) == sizeof(addr));
        ::memcpy(&addr_.sin_addr, &addr, sizeof(addr));
    }

    struct sockaddr_in *addr() {
        return &addr_;
    }

    const struct sockaddr_in *addr() const {
        return &addr_;
    }

    socklen_t addrlen() const {
        return sizeof(addr_);
    }

    uint16_t port() const {
        return __builtin_bswap32(addr_.sin_port);
    }

private:
    struct sockaddr_in addr_;
};

} // namespace cocur

template <>
struct std::formatter<cocur::Endpoint> : std::formatter<std::string> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin(); // no custom format specifiers
    }

    auto format(const cocur::Endpoint &addr, std::format_context &ctx) const {
        char ip[INET_ADDRSTRLEN]{};
        const char *ok = ::inet_ntop(AF_INET, &addr.addr()->sin_addr, ip, sizeof(ip));

        assert(ok);

        std::string s = std::format("{}:{}", ip, addr.port());
        return std::formatter<std::string>::format(s, ctx);
    }
};
