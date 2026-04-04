#include <cocur/net/helpers.h>
#include <gtest/gtest.h>

TEST(Helpers, Address) {
    {
        auto address = cocur::string_to_address("1.1.1.1:10");
        struct in_addr addr;

        ASSERT_EQ(inet_aton("1.1.1.1", &addr), 1);
        ASSERT_EQ(address.address.s_addr, addr.s_addr);
        ASSERT_EQ(address.port, htons(10));
    }
    {
        auto address = cocur::string_to_address("100.103.104.110:10");
        struct in_addr addr;

        ASSERT_EQ(inet_aton("100.103.104.110", &addr), 1);
        ASSERT_EQ(address.address.s_addr, addr.s_addr);
        ASSERT_EQ(address.port, htons(10));
    }
}
