#include <cocur/net/waiters.h>
#include <cocur/thread/pool.h>
#include <cocur/uring/engine.h>
#include <cocur/uring/task.h>

using namespace cocur;

void IOEngine::attachAccept(const Socket &socket, void *data) {
    ring_.attachAccept(socket.fd(), data);
}

void IOEngine::attachConnect(const Socket &socket, struct sockaddr *addr, size_t size, void *data) {
    ring_.attachConnect(socket.fd(), addr, size, data);
}

void IOEngine::attachRead(const Socket &socket, std::byte *buffer, size_t size, void *data) {
    ring_.attachRead(socket.fd(), buffer, size, data);
}

void IOEngine::attachWrite(const Socket &socket, const std::byte *buffer, size_t size, void *data) {
    ring_.attachWrite(socket.fd(), buffer, size, data);
}

void IOEngine::block_on(Task<> task) {
    ThreadPool pool;
    std::uint64_t val;

    ring_.attachRead(event_.fd(), &val, sizeof(val), nullptr);
    spawn(std::move(task));

    while (active_tasks_.load(std::memory_order_relaxed) > 0) {
        auto cqe = ring_.wait();
        auto syscall = reinterpret_cast<detail::AsyncSyscall<> *>(cqe.user_data);

        // eventfd was signaled
        if (!syscall) {
            break;
        }

        pool.schedule([syscall, res = cqe.res]() { syscall->resume(res); });
    }
}

void IOEngine::spawn(Task<> task) {
    // IOEngine should live longer than a task
    task.handle().promise().on_complete_ = [&]() {
        auto left = active_tasks_.fetch_sub(1, std::memory_order_relaxed);

        if (left == 1) {
            event_.signal();
        }
    };

    active_tasks_.fetch_add(1, std::memory_order_relaxed);
    task.handle().resume();
}
