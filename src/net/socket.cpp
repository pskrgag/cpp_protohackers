#include <cocur/net/tcp.h>
#include <cocur/linux/waiters.h>
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
    co_return std::make_shared<TcpClient>(fd);
}

Task<ssize_t> Socket::recvImpl(std::span<std::byte> span) {
    ssize_t read = co_await detail::Read{*this, span};
    co_return read;
}

Task<ssize_t> Socket::recvExactImpl(std::span<std::byte> span) {
    ssize_t read = co_await detail::ReadExact{*this, span};
    co_return read;
}

Task<std::vector<std::byte>> Socket::recv(void) {
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

Task<ssize_t> Socket::connect(struct sockaddr *addr, size_t size) {
    ssize_t read = co_await detail::Connect{*this, addr, size};
    co_return read;
}

Task<ssize_t> Socket::sendImpl(std::span<const std::byte> span) {
    ssize_t written = co_await detail::Write{*this, span};
    co_return written;
}

Task<TcpClient> TcpClient::connect(const std::string &server_addr) {
    auto addr = cocur::string_to_address(server_addr);

    Socket sock;

    struct sockaddr_in in;
    in.sin_family = AF_INET;
    in.sin_port = addr.port;
    in.sin_addr = addr.address;

    int res = co_await sock.connect((struct sockaddr *)&in, sizeof(in));
    if (res < 0)
        throw std::runtime_error("Failed to connect " + std::to_string(errno));

    co_return TcpClient(std::move(sock));
}
