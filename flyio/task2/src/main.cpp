#include <cocur/net/tcp.h>
#include <cocur/scheduler/scheduler.h>
#include <map>

struct Message {
    char type;
    int32_t payload1;
    int32_t payload2;
} __attribute__((packed));

static_assert(std::endian::native == std::endian::little,
              "This code requires a little-endian architecture");
static_assert(sizeof(Message) == 9, "Check size");

cocur::Task<> handle_client(cocur::Scheduler<> &engine, std::shared_ptr<cocur::TcpClient> client) {
    std::map<int32_t, int32_t> map;

    while (true) {
        Message msg;
        auto res = co_await client->recv(msg);
        if (res <= 0)
            break;

        assert(res == sizeof(Message));

        int32_t payload1 = __builtin_bswap32(msg.payload1);
        int32_t payload2 = __builtin_bswap32(msg.payload2);

        switch (msg.type) {
        case 'I':
            map[payload1] = payload2;
            break;
        case 'Q': {
            auto lb = map.lower_bound(payload1);
            auto ub = map.upper_bound(payload2);
            int64_t sum = 0;

            if (payload1 <= payload2) {
                int32_t count = 0;

                for (auto i = lb; i != ub; ++i) {
                    sum += i->second;
                    count += 1;
                }

                sum /= count;
            }

            co_await client->send(__builtin_bswap32((int32_t)sum));
            break;
        }
        default:
            co_return;
            break;
        }
    }
}

cocur::Task<> server(cocur::Scheduler<> &engine) {
    cocur::TcpListner sock("0.0.0.0:8080");

    while (1) {
        auto client = co_await sock.accept();
        engine.spawn(handle_client(engine, client));
    }
}

int main() {
    cocur::Scheduler<> engine;

    engine.spawn(server(engine));
    engine.runToTheEnd();
}
