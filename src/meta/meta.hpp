//
// Created by Maksym Pasichnyk on 28.01.2023.
//

#pragma once

#include <tuple>
#include <variant>

#include "token_stream.hpp"

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

    constexpr auto is_none(const auto& v) -> bool {
        return std::same_as<std::decay_t<decltype(v)>, None>;
    }
}

namespace meta::parse {
    template<typename T>
    consteval auto __list(auto stream, auto&&... args) {
        auto r = Wrapper<[] { return __compile<T>(decltype(stream){}); }>{};
        if constexpr (r.value()) {
            return __list<T>(
                Wrapper<[] {
                    return decltype(stream){}.value().at(decltype(r){}.value().current);
                }>{},
                std::forward<decltype(args)>(args)..., r.value().value
            );
        } else {
            return Result{std::tuple{std::forward<decltype(args)>(args)...}, stream.value().current()};
        }
    }

    template<typename Head, typename... Tail>
    consteval auto __group(auto stream, auto&&... args) {
        auto r = Wrapper<[] { return __compile<Head>(decltype(stream){}); }>{};
        if constexpr (r.value()) {
            if constexpr (sizeof...(Tail) > 0) {
                return __group<Tail...>(
                    Wrapper<[] {
                        return decltype(stream){}.value().at(decltype(r){}.value().current);
                    }>{},
                    std::forward<decltype(args)>(args)..., r.value().value
                );
            } else {
                return Result{std::tuple{std::forward<decltype(args)>(args)..., r.value().value}, r.value().current};
            }
        } else {
            return None{};
        }
    }

    consteval auto __first(auto stream) {
        return None{};
    }

    template<typename Head, typename... Tail>
    consteval auto __first(auto stream) {
        auto r = __compile<Head>(decltype(stream){});
        if constexpr (r) {
            return r;
        } else {
            return __first<Tail...>(stream);
        }
    }

    template<typename T>
    consteval auto __compile(auto stream) {
        auto r = Wrapper<[] { return T::compile(decltype(stream){}); }>{};
        if constexpr (r.value()) {
            auto v = Wrapper<[] { return decltype(r){}.value().value; }>{};
            if constexpr (requires{ T::transform(v); }) {
                return Result{T::transform(v), r.value().current};
            } else {
                return r.value();
            }
        } else {
            return None{};
        }
    }
}

namespace meta::parse {
    template<TokenType type>
    struct token {
        consteval static auto compile(auto stream) {
            if constexpr (stream.value().token().is(type)) {
                return Result{stream.value().token(), stream.value().current() + 1};
            } else {
                return None{};
            }
        }

        constexpr auto is(TokenType other) const {
            return type == other;
        }
    };

    template<uint32_t id>
    struct ident {
        consteval static auto compile(auto stream) {
            if constexpr (stream.value().token().is(TokenType::Identifier) && stream.value().token().id == id) {
                return Result{stream.value().token(), stream.value().current() + 1};
            } else {
                return None{};
            }
        }
    };

    template<typename... T>
    struct group {
        consteval static auto compile(auto stream) {
            return __group<T...>(stream);
        }
    };

    template<typename T>
    struct opt {
        consteval static auto compile(auto stream) {
            if constexpr (auto r = __compile<T>(stream)) {
                return r;
            } else {
                return Result{None{}, stream.value().current()};
            }
        }
    };

    template<typename T>
    struct list {
        consteval static auto compile(auto stream) {
            return __list<T>(stream);
        }
    };

    template<typename T>
    struct list_non_empty {
        consteval static auto compile(auto stream) {
            auto r = __list<T>(stream);
            if constexpr (std::tuple_size_v<decltype(r.value)> != 0) {
                return r;
            } else {
                return None{};
            }
        }
    };

    template<typename... T>
    struct one_of {
        consteval static auto compile(auto stream) {
            return __first<T...>(stream);
        }
    };

    template<typename T, const_string chars>
    consteval static auto compile() {
        auto stream = Wrapper<[] { return TokenStream::parse(chars.str()); }>{};
        auto r = Wrapper<[] { return __compile<T>(decltype(stream){}); }>{};
        if constexpr (r.value()) {
            if constexpr (stream.value().at(r.value().current).token().is(TokenType::End)) {
                return r.value().value;
            } else {
                return None{};
            }
        } else {
            return None{};
        }
    }
}

#define apply_rules(rules, ...) meta::parse::compile<rules, const_string{#__VA_ARGS__}>()
