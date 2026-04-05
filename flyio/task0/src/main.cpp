#include <cocur/net/tcp.h>
#include <cocur/uring/engine.h>
#include <cocur/uring/task.h>
#include <string>

cocur::Task<> handle_client(cocur::IOEngine &engine, std::shared_ptr<cocur::TcpClient> client) {
    std::byte buffer[1000] = {};
    auto span = std::span(buffer, sizeof(buffer));

    while (true) {
        auto read = co_await client->recv(span);
        if (read == 0)
            break;

        co_await client->send(std::span(buffer, read));
    }
}

cocur::Task<> server(cocur::IOEngine &engine) {
    cocur::TcpListner sock("0.0.0.0:8080", engine);

    while (1) {
        auto client = co_await sock.accept();
        engine.spawn(handle_client(engine, client));
    }
}

int main() {
    cocur::IOEngine engine;

    engine.block_on(server(engine));
}
