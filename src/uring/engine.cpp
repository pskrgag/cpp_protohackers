#include <cocur/net/waiters.h>
#include <cocur/thread/pool.h>
#include <cocur/uring/engine.h>
#include <cocur/uring/task.h>

using namespace cocur;

void IOEngine::attachAccept(const Socket &socket, void *data) {
    ring_.attachAccept(socket.fd(), data);
}

void IOEngine::attachRead(const Socket &socket, std::byte *buffer, size_t size, void *data) {
    ring_.attachRead(socket.fd(), buffer, size, data);
}

void IOEngine::attachWrite(const Socket &socket, const std::byte *buffer, size_t size, void *data) {
    ring_.attachWrite(socket.fd(), buffer, size, data);
}

void IOEngine::block_on(Task<> task) {
    ThreadPool pool;

    task.resume();

    while (1) {
        auto cqe = ring_.wait();
        auto syscall = reinterpret_cast<detail::AsyncSyscall<> *>(cqe.user_data);

        pool.schedule([syscall, res = cqe.res]() { syscall->resume(res); });
    }
}

void IOEngine::spawn(Task<> task) {
    task.handle().resume();
}
