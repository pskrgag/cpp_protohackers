#include <cmath>
#include <cocur/net/tcp.h>
#include <cocur/uring/engine.h>
#include <cocur/uring/task.h>
#include <format>
#include <ranges>
#include <rapidjson/document.h>
#include <string>

static bool isPrime(size_t number) {
    if (number < 2)
        return false;

    if (number == 2)
        return true;

    for (unsigned int i = 2; i < sqrt(number) + 1; ++i) {
        if ((number % i) == 0)
            return false;
    }

    return true;
}

static bool isPrime(double number) {
    // Check if the double is exactly an integer
    if (std::fabs(number - std::round(number)) > 1e-9)
        return false; // not a whole number

    if (number < 0)
        return false;

    int n = static_cast<int>(std::round(number));
    return isPrime((size_t)n);
}

// {"method":"isPrime","number":123}
cocur::Task<> handle_client(cocur::IOEngine &engine, std::shared_ptr<cocur::TcpClient> client) {
    auto disconnect = false;
    auto is_valid = [](const rapidjson::Document &d) {
        auto is_ok = d.HasMember("method") && d.HasMember("number");

        return is_ok && d["method"].IsString() && d["method"] == "isPrime" &&
               d["number"].IsNumber();
    };
    constexpr auto malformed = std::string_view("{\n");

    while (true) {
        rapidjson::Document d;
        auto json = co_await client->recv();
        if (json.size() == 0)
            break;

        constexpr std::byte delim = static_cast<std::byte>('\n');
        auto lines = json | std::views::split(delim);

        for (const auto &line : lines) {
            d.Parse(reinterpret_cast<char *>(line.data()), line.size());

            if (line.size() == 0)
                continue;

            if (d.HasParseError() || !d.IsObject() || !is_valid(d)) {
                co_await client->send(malformed);
                break;
            } else {
                auto number = d["number"].Get<double>();
                auto res = std::format("{{\"method\":\"isPrime\",\"prime\":{}}}\n",
                                       isPrime(number) ? "true" : "false");

                co_await client->send(res);
            }
        }
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
