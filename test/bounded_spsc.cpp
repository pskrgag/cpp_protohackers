#include <cocur/bounded_spsc.h>
#include <gtest/gtest.h>
#include <ranges>
#include <thread>

TEST(Spsc, SingleThread) {
    cocur::BoundedSpsc<int, 10> q;

    for (const auto _ : std::views::iota(0, 100)) {
        for (auto i : std::views::iota(0, 10)) {
            ASSERT_EQ(q.push(i), true);
        }

        ASSERT_EQ(q.push(1), false);
        ASSERT_EQ(q.push(1), false);

        for (auto i : std::views::iota(0, 10)) {
            ASSERT_EQ(q.pop(), i);
        }

        ASSERT_EQ(q.pop(), std::nullopt);
    }
}

class TestClass {
    int a;

public:
    TestClass() = default;
    TestClass(TestClass &&) = default;
    TestClass &operator=(TestClass &&) = default;

    TestClass(const TestClass &) = delete;
    TestClass operator=(const TestClass &) = delete;
};

class ResourceClass {
    int *ptr;

public:
    ResourceClass() : ptr(new int) {
    }

    ResourceClass(const ResourceClass &other) : ptr(new int) {
    }

    ResourceClass operator=(const ResourceClass &other) {
        return ResourceClass();
    }

    ~ResourceClass() {
        delete ptr;
    }
};

TEST(Spsc, Constructor) {
    cocur::BoundedSpsc<TestClass, 10> q;

    q.push(TestClass{});
    auto _ = q.pop();
}

TEST(Spsc, Leak) {
    cocur::BoundedSpsc<ResourceClass, 10> q;

    q.push(ResourceClass{});
    q.push(ResourceClass{});
    q.push(ResourceClass{});
}

#ifdef WITH_SANITIZERS
static constexpr std::size_t QSize = 100;
#else
static constexpr std::size_t QSize = 10'000'000;
#endif

TEST(Spsc, Threads) {
    auto q = std::make_unique<cocur::BoundedSpsc<std::size_t, QSize>>();
    auto expected = (q->capacity() - 1) * q->capacity() / 2;

    auto jt = std::jthread([&]() {
        std::size_t sum = 0;
        std::size_t count = 0;

        while (true) {
            auto res = q->pop();

            if (res.has_value()) {
                sum += *res;

                if (++count == q->capacity()) {
                    break;
                }
            }
        }

        ASSERT_EQ(sum, expected);
    });

    for (const auto i : std::views::iota(0uz, q->capacity())) {
        q->push(i);
    }
}
