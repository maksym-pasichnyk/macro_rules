//
// Created by Maksym Pasichnyk on 28.01.2023.
//

#pragma once

#include <tuple>
#include <variant>

#include "token_stream.hpp"

namespace meta::parse {
    template<auto Fn>
    struct wrapper {
        static constexpr auto __value = Fn();

        constexpr auto value() const {
            return __value;
        }
    };
}

namespace meta::parse {
    template<typename Arg0>
    struct Pair {
        Arg0 arg0;
        size_t arg1;

        constexpr auto has_value() const -> bool {
            return true;
        }
    };

    template<typename Arg0>
    Pair(Arg0, size_t) -> Pair<Arg0>;

    struct None {};

    struct Ident {
        size_t id;
    };

    struct Number {
        size_t num;
    };

    template<typename... T>
    using Group = std::tuple<T...>;

    consteval auto is_none(const None& r) -> bool {
        return true;
    }
    consteval auto is_none(const auto& r) -> bool {
        return false;
    }
    consteval auto is_some(const None& r) -> bool {
        return false;
    }
    consteval auto is_some(const auto& r) -> bool {
        return true;
    }

    template<typename T>
    consteval auto __compile(auto stream) {
        constexpr auto r = wrapper<[] { return T::compile(decltype(stream){}); }>{};
        if constexpr (is_none(r.value())) {
            return None{};
        } else {
            constexpr auto v = wrapper<[] { return decltype(r)::__value.arg0; }>{};
            if constexpr (requires{ T::transform(v); }) {
                return Pair{T::transform(v), r.value().arg1};
            } else {
                return r.value();
            }
        }
    }
}

namespace meta::parse {
    template<typename T>
    consteval auto __list(auto stream, auto&&... args) {
        constexpr auto r = wrapper<[] { return __compile<T>(decltype(stream){}); }>{};
        if constexpr (is_none(r.value())) {
            return Pair{std::tuple{std::forward<decltype(args)>(args)...}, stream.value().current()};
        } else {
            return __list<T>(
                wrapper<[] {
                    return decltype(stream)::__value.at(decltype(r)::__value.arg1);
                }>{},
                std::forward<decltype(args)>(args)..., r.value().arg0
            );
        }
    }

    template<typename Head, typename... Tail>
    consteval auto __group(auto stream, auto&&... args) {
        constexpr auto r = wrapper<[] { return __compile<Head>(decltype(stream){}); }>{};
        if constexpr (is_none(r.value())) {
            return None{};
        } else if constexpr (sizeof...(Tail) > 0) {
            return __group<Tail...>(
                wrapper<[] {
                    return decltype(stream)::__value.at(decltype(r)::__value.arg1);
                }>{},
                std::forward<decltype(args)>(args)..., r.value().arg0
            );
        } else {
            return Pair{std::tuple{std::forward<decltype(args)>(args)..., r.value().arg0}, r.value().arg1};
        }
    }

    consteval auto __first(auto stream) {
        return None{};
    }

    template<typename Head, typename... Tail>
    consteval auto __first(auto stream) {
        constexpr auto r = __compile<Head>(decltype(stream){});
        if constexpr (is_none(r)) {
            return __first<Tail...>(stream);
        } else {
            return r;
        }
    }
}

namespace meta::parse {
    template<TokenType type>
    struct token {
        consteval static auto compile(auto stream) {
            if constexpr (stream.value().token().is(type)) {
                return Pair{stream.value().token(), stream.value().current() + 1};
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
                return Pair{Ident{stream.value().token().id}, stream.value().current() + 1};
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
            constexpr auto r = __compile<T>(stream);
            if constexpr (is_none(r)) {
                return Pair{None{}, stream.value().current()};
            } else {
                return r;
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
            constexpr auto r = __list<T>(stream);
            if constexpr (is_some(r)) {
                if constexpr (std::tuple_size_v<decltype(r.arg0)> != 0) {
                    return r;
                } else {
                    return None{};
                }
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
        auto stream = wrapper<[] { return TokenStream::parse(chars.str()); }>{};
        auto r = wrapper<[] { return __compile<T>(decltype(stream){}); }>{};
        if constexpr (is_none(r.value())) {
            return None{};
        } else if constexpr (stream.value().at(r.value().arg1).token().is(TokenType::End)) {
            return r.value().arg0;
        } else {
            return None{};
        }
    }
}

#define apply_rules(rules, ...) meta::parse::compile<rules, const_string{#__VA_ARGS__}>()
