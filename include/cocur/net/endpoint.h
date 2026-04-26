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
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>

namespace cocur {

class Endpoint {
public:
    Endpoint() : addr_() {};

    /// host:n
    Endpoint(std::string_view address) {
        auto colon = address.rfind(':');
        if (colon == std::string_view::npos || colon == address.size() - 1)
            throw std::runtime_error(std::format("wrong endpoint string: {}", address));

        std::string host(address.substr(0, colon));
        std::string port(address.substr(colon + 1));

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_flags = host.empty() ? AI_PASSIVE : 0;

        addrinfo *result = nullptr;
        int ret = ::getaddrinfo(host.empty() ? nullptr : host.c_str(), port.c_str(), &hints,
                                &result);
        if (ret != 0)
            throw std::runtime_error(std::format("failed to resolve endpoint {}: {}", address,
                                                 ::gai_strerror(ret)));

        if (result == nullptr || result->ai_addrlen != sizeof(addr_)) {
            if (result != nullptr)
                ::freeaddrinfo(result);
            throw std::runtime_error(std::format("failed to resolve IPv4 endpoint {}", address));
        }

        std::memcpy(&addr_, result->ai_addr, sizeof(addr_));
        ::freeaddrinfo(result);
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
        return ::ntohs(addr_.sin_port);
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
