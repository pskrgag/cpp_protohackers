#include <cocur/scheduler/context.h>
#include <cocur/scheduler/task_handle.h>

using namespace cocur;

void TaskHandle::cancel() {
    auto state = state_;

    do {
        state->canceled_.store(true, std::memory_order_relaxed);

        Context *ctx = static_cast<Context *>(state->context_);
        if (!ctx)
            return;

        if (!state->completed_.load(std::memory_order_relaxed)) {
            ctx->engine().requestCancel(state);
        }

        state = state->child_.load(std::memory_order_relaxed);
    } while (state);
}
