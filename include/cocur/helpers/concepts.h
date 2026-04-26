/*
 * brief: Helper concepts
 *
 * Copyright (c) 2026 Pavel Skripkin
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <ranges>

template <typename T>
concept ByteRange = std::ranges::contiguous_range<T> && std::ranges::sized_range<T> &&
                    sizeof(std::ranges::range_value_t<T>) == 1;
template <typename T>
concept Pod = std::is_trivial_v<T> && std::is_standard_layout_v<T> && !std::is_array_v<T>;
