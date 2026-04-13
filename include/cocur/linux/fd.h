/*
 * brief:  Fd interface
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */

#include <unistd.h>
#pragma once

namespace cocur {
class Fd {
public:
    virtual int fd() const = 0;
};

}; // namespace cocur
