//
// Created by Maksym Pasichnyk on 28.01.2023.
//

#pragma once

#include <array>

template<typename T, size_t Capacity>
struct static_vector {
    [[nodiscard]] constexpr auto operator[](size_t i) -> T& {
        return raw[i];
    }

    [[nodiscard]] constexpr auto operator[](size_t i) const -> T const& {
        return raw[i];
    }

    [[nodiscard]] constexpr auto size() const -> size_t {
        return count;
    }

    [[nodiscard]] constexpr auto data() const -> const T* {
        return raw.data();
    }

    template<typename U>
    constexpr void emplace_back(U&& u) {
        raw[count] = static_cast<U>(u);
        count += 1;
    }

    std::array<T, Capacity> raw{};
    size_t count = 0;
};