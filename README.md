# Introduction

Header-only library for compile-time parsing. <br/>
Library provides `macro_rules` macro that allows to describe own DSL. <br/>
Syntax of `macro_rules` was inspired by Rust, but with some limitations. <br/>
DSL can be implemented directly using templates, for more flexibility.

This project is currently under development and not well tested and not documented.

# Code Example

```c++
#include "meta/meta_rules.hpp"

template<std::array params>
struct FunctionContext {
    template<meta::parse::Ident name>
    constexpr auto get(const auto& args) {
        return std::get<get_argument_index(name)>(args);
    }

    constexpr static auto get_argument_index(meta::parse::Ident name) -> size_t {
        for (size_t i = 0; i < params.size(); ++i) {
            if (params[i] == name.id) {
                return i;
            }
        }
        return -1;
    }
};

struct Function : macro_rules(
    ($($param:ident)*) -> $body:expr
) {
    consteval static auto transform(auto ctx) {
        constexpr auto params = std::apply(
            [](auto... args) {
                return std::array{
                    static_cast<size_t>(args.id)...
                };
            },
            meta::parse::get<"param">(ctx.value())
        );

        constexpr auto body = std::get<0>(meta::parse::get<"body">(ctx.value()));
        return wrap<FunctionContext<params>{}, body>();
    }

    template<FunctionContext ctx, auto body>
    consteval static auto wrap() {
        return [](auto&&... args) {
            return body(ctx, std::forward_as_tuple(std::forward<decltype(args)>(args)...));
        };
    }
};

#define $fn(...) apply_rules(Function, __VA_ARGS__)

auto main() -> int {
    constexpr auto fn1 = $fn((a b c d) -> ((a + b) * (c - d)) * 10);
    constexpr auto fn2 = [](int a, int b, int c, int d) {
        return (a + b) * (c - d) * 10;
    };
    static_assert(fn1(1, 2, 3, 4) == fn2(1, 2, 3, 4));
    return 0;
}
```