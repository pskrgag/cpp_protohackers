#include "chatroom.h"
#include <cctype>
#include <cocur/net/tcp.h>
#include <cocur/scheduler/scheduler.h>
#include <print>

cocur::Task<> handle_client(cocur::Scheduler<> &engine, Client client, ChatRoom &room) {
    auto validate_name = [](std::string_view name) {
        return std::all_of(name.begin(), name.end(), [](const char &c) { return std::isalnum(c); });
    };

    co_await client->send("Welcome to budgetchat! What shall I call you?\n");
    auto name = co_await client->getMessage();

    if (!name || name->size() == 0 || !validate_name(*name)) {
        co_await client->send("Invalid name\n");
        co_return;
    }

    auto handle = co_await room.add(*name, client);
    if (!handle) {
        co_await client->send("Duplicate name\n");
        co_return;
    }

    while (1) {
        auto msg = co_await client->getMessage();
        if (!msg) {
            break;
        }

        co_await room.sendMessage(*name, *msg);
    }
}

cocur::Task<> server(cocur::Scheduler<> &engine) {
    cocur::TcpListner sock("0.0.0.0:8081");
    ChatRoom room(engine);

    while (1) {
        auto client = co_await sock.accept();
        engine.spawn(handle_client(engine, std::make_shared<ClientSocket>(client), room));
    }
}

int main() {
    cocur::Scheduler<> engine;

    engine.spawn(server(engine));
    engine.runToTheEnd();
}
