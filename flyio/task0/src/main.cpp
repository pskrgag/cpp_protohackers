#include <cocur/net/tcp.h>
#include <cocur/scheduler/scheduler.h>
#include <string>

cocur::Task<> handle_client(cocur::Scheduler<> &engine, std::shared_ptr<cocur::TcpClient> client) {
    std::byte buffer[1000] = {};
    auto span = std::span(buffer, sizeof(buffer));

    while (true) {
        auto read = co_await client->recv(span);
        if (read == 0)
            break;

        co_await client->send(std::span(buffer, read));
    }
}

cocur::Task<> server(cocur::Scheduler<> &scheduler) {
    cocur::TcpListner sock("0.0.0.0:8080");

    while (1) {
        auto client = co_await sock.accept();
        scheduler.spawn(handle_client(scheduler, client));
    }
}

int main() {
    cocur::Scheduler<> scheduler;

    scheduler.spawn(server(scheduler));
    scheduler.runToTheEnd();
}
