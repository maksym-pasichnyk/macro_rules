//
// Created by Maksym Pasichnyk on 30.01.2023.
//

#pragma once

#include <cstddef>

namespace meta::parse {
    template<auto Fn>
    struct Wrapper {
        static constexpr auto stored = Fn();

        constexpr auto value() const {
            return stored;
        }
    };

    struct None {
        constexpr explicit operator bool() const {
            return false;
        }
    };

    template<typename V>
    struct Result {
        V value;
        size_t current;

        constexpr explicit operator bool() const {
            return true;
        }
    };

    template<typename V>
    Result(V, size_t) -> Result<V>;

    template<size_t id, typename T>
    struct WithName : T {};
}