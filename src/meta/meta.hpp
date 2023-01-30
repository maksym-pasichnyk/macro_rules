//
// Created by Maksym Pasichnyk on 28.01.2023.
//

#pragma once

#include <tuple>
#include <variant>

#include "types.hpp"
#include "token_stream.hpp"

namespace meta::parse {
    namespace detail {
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

        template<size_t id>
        consteval auto __get(const auto& leaf);

        template<size_t id, typename T>
        consteval auto __get(const WithName<id, T>& leaf);

        template<size_t id, typename... T, size_t... I>
        consteval auto __get(const std::tuple<T...>& tree, std::index_sequence<I...>);

        template<size_t id, typename... T>
        consteval auto __get(const std::tuple<T...>& tree);

        template<size_t id>
        consteval auto __get(const auto& leaf) {
            return std::tuple<>();
        }

        template<size_t id, typename T>
        consteval auto __get(const WithName<id, T>& leaf) {
            return std::tuple(static_cast<T>(leaf));
        }

        template<size_t id, typename... T, size_t... I>
        consteval auto __get(const std::tuple<T...>& tree, std::index_sequence<I...>) {
            return std::tuple_cat(__get<id>(std::get<I>(tree))...);
        }

        template<size_t id, typename... T>
        consteval auto __get(const std::tuple<T...>& tree) {
            return __get<id>(tree, std::index_sequence_for<T...>{});
        }
    }

    // todo: replace with a better implementation, maybe delete this
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

    // todo: replace with a better implementation, maybe delete this
    template<size_t id>
    struct ident {
        consteval static auto compile(auto stream) {
            if constexpr (stream.value().token().is(TokenType::Identifier) && stream.value().token().id == id) {
                return Result{stream.value().token(), stream.value().current() + 1};
            } else {
                return None{};
            }
        }
    };

    /** rule0 rule1... **/
    template<typename... T>
    struct group {
        consteval static auto compile(auto stream) {
            return detail::__group<T...>(stream);
        }
    };

    /** rule? **/
    template<typename T>
    struct opt {
        consteval static auto compile(auto stream) {
            if constexpr (auto r = detail::__compile<T>(stream)) {
                return r;
            } else {
                return Result{None{}, stream.value().current()};
            }
        }
    };

    /** rule* **/
    template<typename T>
    struct list {
        consteval static auto compile(auto stream) {
            return detail::__list<T>(stream);
        }
    };

    /** rule+ **/
    template<typename T>
    struct list_non_empty {
        consteval static auto compile(auto stream) {
            auto r = detail::__list<T>(stream);
            if constexpr (std::tuple_size_v<decltype(r.value)> != 0) {
                return r;
            } else {
                return None{};
            }
        }
    };

    /** rule0 | rule1 | ... **/
    template<typename... T>
    struct one_of {
        consteval static auto compile(auto stream) {
            return detail::__first<T...>(stream);
        }
    };

    template<size_t id, typename T>
    struct with_name : T {
        consteval static auto transform(auto ctx) {
            using U = std::decay_t<decltype(ctx.value())>;
            return WithName<id, U>{ctx.value()};
        }
    };

    template<typename T, const_string chars>
    consteval auto compile() {
        auto stream = Wrapper<[] { return TokenStream::parse(chars.str()); }>{};
        auto r = Wrapper<[] { return detail::__compile<T>(decltype(stream){}); }>{};
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

    /** function to retrieve all fragments with a given name **/
    template<const_string name>
    consteval auto get(const auto& tree) {
        return detail::__get<fnv1a(name.str())>(tree);
    }

    consteval auto is_none(const auto& v) -> bool {
        return std::same_as<std::decay_t<decltype(v)>, None>;
    }
}

#define apply_rules(rules, ...) meta::parse::compile<rules, #__VA_ARGS__>()
