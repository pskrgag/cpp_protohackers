#include <cocur/net/tcp.h>
#include <cocur/scheduler/scheduler.h>
#include <gtest/gtest.h>

cocur::Task<> handle_client(cocur::Scheduler<> &engine, std::shared_ptr<cocur::TcpClient> client) {
    std::byte buffer[1000] = {};
    auto span = std::span(buffer, sizeof(buffer));

    while (true) {
	    std::println("1recv");
        auto read = co_await client->recv(span);
        if (read == 0)
            break;

        co_await client->send(std::span(buffer, read));
    }
}

cocur::Task<> server(cocur::Scheduler<> &engine) {
    cocur::TcpListner sock("0.0.0.0:9998");

    auto client = co_await sock.accept();
    co_await handle_client(engine, client);
}

cocur::Task<> client(cocur::Scheduler<> &engine) {
    cocur::TcpClient sock = co_await cocur::TcpClient::connect("0.0.0.0:9998");

    std::byte buffer[1000];
    auto span = std::span(buffer, sizeof(buffer));

    co_await sock.send("hello");
    std::println("recv");
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
