#include <cocur/containers/unbounded_spsc.h>
#include <gtest/gtest.h>
#include <ranges>
#include <thread>

TEST(UbSpsc, SingleThread) {
    cocur::UnboundedSpsc<int> q;

    for (const auto _ : std::views::iota(0, 1)) {
        for (auto i : std::views::iota(0, 1)) {
            q.push(i);
        }

        for (auto i : std::views::iota(0, 1)) {
            ASSERT_EQ(q.pop(), i);
        }

        ASSERT_EQ(q.pop(), std::nullopt);
    }
}

TEST(UbSpsc, Threads) {
    auto q = std::make_unique<cocur::UnboundedSpsc<std::size_t>>();
#ifdef WITH_SANITIZERS
    std::size_t repeats = 100;
#else
    std::size_t repeats = 10'000'000;
#endif
    auto expected = (repeats - 1) * repeats / 2;

    auto jt = std::jthread([&]() {
        std::size_t sum = 0;
        std::size_t count = 0;

        while (true) {
            auto res = q->pop();

            if (res.has_value()) {
                sum += *res;

                if (++count == repeats) {
                    break;
                }
            }
        }

        ASSERT_EQ(sum, expected);
    });

    for (const auto i : std::views::iota(0uz, repeats)) {
        q->push(i);
    }
}
