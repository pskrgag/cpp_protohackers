#include <cocur/fs/file.h>
#include <cocur/net/tcp.h>
#include <cocur/scheduler/scheduler.h>
#include <cocur/timer/timer.h>
#include <gtest/gtest.h>

using namespace std::chrono_literals;

cocur::Task<> handle_client(cocur::Scheduler<> &engine, cocur::TcpClient client) {
    std::byte buffer[1000] = {};
    auto span = std::span(buffer, sizeof(buffer));

    while (true) {
        auto read = co_await client.recv(span);
        if (read == 0)
            break;

        co_await client.send(std::span(buffer, read));
    }
}

cocur::Task<> server(cocur::Scheduler<> &engine) {
    cocur::TcpListner sock("0.0.0.0:9998");

    auto client = co_await sock.accept();
    co_await handle_client(engine, std::move(client));
}

cocur::Task<> client(cocur::Scheduler<> &engine) {
    cocur::TcpClient sock = co_await cocur::TcpClient::connect("0.0.0.0:9998");

    std::byte buffer[1000];
    auto span = std::span(buffer, sizeof(buffer));

    co_await sock.send("hello");
    auto read = co_await sock.recv(span);

    EXPECT_EQ(read, 6);
    EXPECT_EQ("hello", std::string((char *)buffer, read - 1));
}

TEST(Engine, SimpleSocket) {
    cocur::Scheduler engine;

    engine.spawn(server(engine));
    engine.spawn(client(engine));
    engine.runToTheEnd();
}

cocur::Task<> block() {
    cocur::TcpListner sock("0.0.0.0:9998");

    auto client = co_await sock.accept();
}

cocur::Task<> block1() {
    co_await block();
}

TEST(Engine, Cancel) {
    {
        cocur::Scheduler engine;

        auto handle = engine.spawn(block());
        handle.cancel();
        engine.runToTheEnd();
    }

    {
        cocur::Scheduler engine;

        auto handle = engine.spawn(block1());

        // ... =)
        sleep(1);

        handle.cancel();
        engine.runToTheEnd();
    }
}

cocur::Task<> sleep(std::atomic<int> &flag) {
    co_await cocur::sleepFor(1s);
    flag.store(1, std::memory_order_relaxed);
}

cocur::Task<> wait(std::atomic<int> &flagParent) {
    std::atomic<int> flag = 0;
    co_await sleep(flag);

    EXPECT_EQ(flag.load(std::memory_order_relaxed), 1);
    flagParent.store(1, std::memory_order_relaxed);
}

TEST(Engine, Join) {
    {
        cocur::Scheduler engine;
        std::atomic<int> flag = 0;

        auto handle = engine.spawn(wait(flag));
        handle.join();
        EXPECT_EQ(flag.load(std::memory_order_relaxed), 1);
        engine.runToTheEnd();
    }
}
