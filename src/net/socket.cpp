#include <cocur/linux/waiters.h>
#include <cocur/net/tcp.h>
#include <cocur/uring/uring.h>

using namespace cocur;

Task<ssize_t> ClientSocket::connect(struct sockaddr *addr, size_t size) {
    ssize_t fd = co_await detail::Connect{*this, addr, size};
    co_return fd;
}

Task<ssize_t> ClientSocket::recvImpl(std::span<std::byte> span) {
    ssize_t read = co_await detail::Read{*this, span};
    co_return read;
}

Task<ssize_t> ClientSocket::sendImpl(std::span<const std::byte> span) {
    ssize_t written = co_await detail::Write{*this, span};
    co_return written;
}

Task<ssize_t> ClientSocket::recvExactImpl(std::span<std::byte> span) {
    ssize_t read = co_await detail::ReadExact{*this, span};
    co_return read;
}

Task<std::vector<std::byte>> Socket::readToTheEnd(void) {
    std::vector<std::byte> res{};
    ssize_t read = 0;
    std::byte buffer[8 << 10];

    do {
        auto span = std::span(buffer, sizeof(buffer));

        read = co_await detail::Read{*this, span};
        res.insert(res.end(), span.begin(), span.begin() + read);
    } while (read == sizeof(buffer));

    co_return res;
}
