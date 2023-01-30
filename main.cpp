#include "meta/meta_rules.hpp"

struct Example : meta::parse::group<
    meta::parse::ident<fnv1a("sum")>,
    meta::parse::list<
        meta::parse::with_name<
            fnv1a("args"),
            meta::parse::token<TokenType::Number>
        >
    >
> {
    consteval static auto transform(auto ctx) {
        return std::apply(
            [](auto... args) {
                return (static_cast<int>(args.id) + ...);
            },
            meta::parse::get<"args">(ctx.value())
        );
    }
};

static_assert(apply_rules(Example, sum 1 2 3 4) == 10);

template<std::array params>
struct FunctionContext {
    template<Variable name>
    constexpr auto get(const auto& args) {
        return std::get<get_argument_index(name)>(args);
    }

    constexpr static auto get_argument_index(Variable name) -> size_t {
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