#include <cocur/net/tcp.h>
#include <cocur/net/waiters.h>
#include <cocur/uring/uring.h>
#include <format>
#include <limits>

using namespace cocur;

int TcpListner::create_and_listen(const std::string &server_addr) {
    auto addr = cocur::string_to_address(server_addr);

    int fd_ = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd_ < 0)
        throw std::runtime_error("Failed to create a socket");

    struct sockaddr_in in;
    in.sin_family = AF_INET;
    in.sin_port = addr.port;
    in.sin_addr = addr.address;

    int enable = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");

    int res = bind(fd_, (struct sockaddr *)&in, sizeof(in));
    if (res < 0)
        throw std::runtime_error(std::format("Failed to bind socket {}", errno));

    res = listen(fd_, std::numeric_limits<int>::max());
    if (res < 0)
        throw std::runtime_error("Failed to listen to socket");

    return fd_;
}

Task<std::shared_ptr<TcpClient>> TcpListner::accept() {
    int fd = co_await detail::Accept{*this};
    co_return std::shared_ptr<TcpClient>(new TcpClient(fd, engine_));
}

Task<ssize_t> Socket::recv(std::span<std::byte> &span) {
    ssize_t read = co_await detail::Read{*this, span};
    co_return read;
}

Task<ssize_t> Socket::send(const std::span<std::byte> &span) {
    ssize_t written = co_await detail::Write{*this, span};
    co_return written;
}

int TcpClient::connect(const std::string &server_addr) {
    auto addr = cocur::string_to_address(server_addr);

    int fd_ = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd_ < 0)
        throw std::runtime_error("Failed to create a socket");

    struct sockaddr_in in;
    in.sin_family = AF_INET;
    in.sin_port = addr.port;
    in.sin_addr = addr.address;

    int res = ::connect(fd_, (struct sockaddr *)&in, sizeof(in));
    if (res < 0)
        throw std::runtime_error("Failed to connect");

    return fd_;
}
