//
// Created by Maksym Pasichnyk on 28.01.2023.
//

#pragma once

#include <string_view>

template<size_t N>
struct const_string : std::array<char, N> {
    [[nodiscard]] constexpr auto str() const -> std::string_view {
        return std::string_view(std::array<char, N>::data(), N - 1);
    }
};

template<size_t N>
const_string(const char(&)[N]) -> const_string<N>;

static constexpr auto fnv1a(std::string_view s) -> uint32_t {
    uint32_t hash = 0;
    for (char i : s) {
        hash *= 0x811C9DC5;
        hash ^= static_cast<uint32_t>(i);
    }
    return hash;
}