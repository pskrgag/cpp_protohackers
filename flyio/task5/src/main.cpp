#include <cocur/net/tcp.h>
#include <cocur/scheduler/scheduler.h>
#include <string>

class ClientSocket {
public:
    ClientSocket(std::shared_ptr<cocur::TcpClient> client) : client_(client), messages_{} {
    }

    cocur::Task<std::optional<std::string>> getMessage() {
        std::byte buffer[2000];
        bool to_do_last = false;

        if (messages_.size() == 0) {
            do {
                auto res = co_await client_->recv(buffer);
                if (res == 0) {
                    co_return {};
                }

                auto received_data = std::span(buffer, buffer + res);

                if (to_do_last) {
                    auto elem = std::ranges::find(received_data, static_cast<std::byte>('\n'));
                    auto idx = elem != std::end(received_data)
                                   ? std::distance(received_data.begin(), elem)
                                   : received_data.size();

                    messages_.back().append(std::string_view((char *)buffer, idx));
                    received_data = received_data.subspan(idx);
                }

                for (auto part : received_data | std::views::split(static_cast<std::byte>('\n'))) {
                    if (part.size() != 0)
                        messages_.emplace_back(part.begin(), part.end());
                }

                to_do_last = buffer[res - 1] != static_cast<std::byte>('\n');
            } while (to_do_last);
        }

        auto first = messages_.front();

        messages_.pop_front();
        co_return first;
    }

    cocur::Task<ssize_t> send(std::string_view data) {
        co_return co_await client_->send(data);
    }

private:
    std::shared_ptr<cocur::TcpClient> client_;
    std::deque<std::string> messages_;
};

static std::string rewrite_msg(std::string msg) {
    static constexpr std::string_view toni_address = "7YWHMfk9JZe0LM0g1ZauHuiSxhI";

    auto validate_addr = [](std::string_view name) {
        return name.size() >= 26 && name.size() <= 35 && name.starts_with('7') &&
               std::all_of(name.begin(), name.end(), [](const char &c) {
                   return std::isalnum(static_cast<unsigned char>(c));
               });
    };

    size_t start = 0;
    while (start < msg.size()) {
        auto end = msg.find(' ', start);
        if (end == std::string::npos)
            end = msg.size();

        if (validate_addr(std::string_view(msg).substr(start, end - start))) {
            msg.replace(start, end - start, toni_address);
            end = start + toni_address.size();
        }

        start = end + 1;
    }

    return msg;
}

cocur::Task<> handle_upstream(ClientSocket sock, ClientSocket &upstream) {
    while (1) {
        auto opt_msg = co_await upstream.getMessage();
        if (!opt_msg)
            break;

        auto msg = *opt_msg;

        msg = rewrite_msg(std::move(msg));
        msg.push_back('\n');
        co_await sock.send(msg);
    }

    co_return;
}

cocur::Task<> handle_client(cocur::Scheduler<> &engine, ClientSocket sock) {
    auto upstream = ClientSocket(std::make_shared<cocur::TcpClient>(
        co_await cocur::TcpClient::connect("chat.protohackers.com:16963")));

    auto upstream_task = engine.spawn(handle_upstream(sock, upstream));

    while (1) {
        auto opt_msg = co_await sock.getMessage();
        if (!opt_msg)
            break;

        auto msg = *opt_msg;

        msg = rewrite_msg(std::move(msg));
        msg.push_back('\n');
        co_await upstream.send(msg);
    }

    upstream_task.cancel();
    co_return;
}

cocur::Task<> server(cocur::Scheduler<> &engine) {
    cocur::TcpListner sock("0.0.0.0:8080");

    while (1) {
        auto client = co_await sock.accept();

        engine.spawn(handle_client(engine, std::make_shared<cocur::TcpClient>(std::move(client))));
    }
}

int main() {
    cocur::Scheduler<> engine;

    engine.spawn(server(engine));
    engine.runToTheEnd();
}
