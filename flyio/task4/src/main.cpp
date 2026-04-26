#include <cocur/net/udp.h>
#include <cocur/scheduler/scheduler.h>
#include <mutex>
#include <optional>
#include <print>
#include <span>
#include <string_view>
#include <unordered_map>

class Db {
public:
    Db() = default;

    void add(std::string key, std::string value) {
        std::lock_guard _guard(mutex_);
        map_[key] = value;
    }

    std::optional<std::string> get(std::string key) {
        std::lock_guard _guard(mutex_);

        if (map_.contains(key))
            return map_[key];

        return std::nullopt;
    }

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::string> map_;
};

static Db Db;

cocur::Task<> handle_client(std::vector<std::byte> data, cocur::UdpListner &server,
                            cocur::Endpoint ep) {
    auto eq = std::ranges::find(data, std::byte{'='});

    if (eq != data.end()) {
        auto key_span = std::span(data.begin(), eq);
        auto value_span = std::span(eq + 1, data.end());
        auto key = std::string((char *)key_span.data(), key_span.size());

        if (key != "version")
            Db.add(key, std::string((char *)value_span.data(), value_span.size()));
    } else {
        auto key = std::string((char *)data.data(), data.size());

        if (key == "version") {
            static constexpr std::string_view version = "version=Ken's Key-Value Store 1.0";
            co_await server.sendTo(version, ep);
            co_return;
        }

        auto value = Db.get(key);

        if (value) {
            co_await server.sendTo(std::format("{}={}", key, *value), ep);
        } else {
            co_await server.sendTo(std::format("{}=", key), ep);
        }
    }

    co_return;
}

cocur::Task<> server(cocur::Scheduler<> &engine) {
    cocur::UdpListner sock("0.0.0.0:8080");

    while (1) {
        std::byte buffer[1000];
        cocur::Endpoint from;

        auto size = co_await sock.recvFrom(buffer, from);
        engine.spawn(handle_client(std::vector(buffer, buffer + size), sock, from));
    }
}

int main() {
    cocur::Scheduler<> engine;

    engine.spawn(server(engine));
    engine.runToTheEnd();
}
