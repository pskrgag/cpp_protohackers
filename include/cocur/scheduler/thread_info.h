/*
 * brief:  Per-thread info
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <unistd.h>

namespace cocur {
class Context;
};

namespace cocur::detail {

struct ThreadInfo {
    Context *context_;
};

inline thread_local ThreadInfo tinfo;

}; // namespace cocur::detail
