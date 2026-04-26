#include <cocur/net/helpers.h>
#include <cerrno>
#include <format>
#include <gtest/gtest.h>
#include <unistd.h>

TEST(Helpers, Address) {
    {
        auto address = cocur::Endpoint("1.1.1.1:10");
        struct in_addr addr;

        ASSERT_EQ(inet_aton("1.1.1.1", &addr), 1);
        ASSERT_EQ(address.addr()->sin_family, AF_INET);
        ASSERT_EQ(address.addr()->sin_addr.s_addr, addr.s_addr);
        ASSERT_EQ(address.addr()->sin_port, htons(10));
        ASSERT_EQ(address.port(), 10);
        ASSERT_EQ(std::format("{}", address), "1.1.1.1:10");
    }
    {
        auto address = cocur::Endpoint("100.103.104.110:65535");
        struct in_addr addr;

        ASSERT_EQ(inet_aton("100.103.104.110", &addr), 1);
        ASSERT_EQ(address.addr()->sin_family, AF_INET);
        ASSERT_EQ(address.addr()->sin_addr.s_addr, addr.s_addr);
        ASSERT_EQ(address.addr()->sin_port, htons(65535));
        ASSERT_EQ(address.port(), 65535);
        ASSERT_EQ(std::format("{}", address), "100.103.104.110:65535");
    }
}

TEST(Helpers, PassiveAddress) {
    auto address = cocur::Endpoint(":8080");

    ASSERT_EQ(address.addr()->sin_family, AF_INET);
    ASSERT_EQ(address.addr()->sin_addr.s_addr, htonl(INADDR_ANY));
    ASSERT_EQ(address.addr()->sin_port, htons(8080));
    ASSERT_EQ(address.port(), 8080);
}

TEST(Helpers, BadAddress) {
    ASSERT_THROW(cocur::Endpoint("1.2.3.4"), std::runtime_error);
    ASSERT_THROW(cocur::Endpoint("1.2.3.4:"), std::runtime_error);
    ASSERT_THROW(cocur::Endpoint("1.2.3.4:not-a-port"), std::runtime_error);
}

TEST(Helpers, CreateUdpListenerOnEphemeralPort) {
    int fd = cocur::create_listner("127.0.0.1:0", SOCK_DGRAM);
    ASSERT_GE(fd, 0);

    sockaddr_in bound{};
    socklen_t bound_len = sizeof(bound);
    ASSERT_EQ(getsockname(fd, reinterpret_cast<sockaddr *>(&bound), &bound_len), 0);
    ASSERT_EQ(bound.sin_family, AF_INET);
    ASSERT_NE(bound.sin_port, 0);

    close(fd);
}
