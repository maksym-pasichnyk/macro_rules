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
        // todo: ctx.get<"param">()
        constexpr auto params = std::apply(
            [](auto... args) { return std::array{args.id...}; },
            std::get<1>(std::get<0>(ctx.value()))
        );
        // todo: ctx.get<"body">()
        constexpr auto body = std::get<2>(ctx.value());
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
    fn1(1, 2, 3, 4);
    static_assert(fn1(1, 2, 3, 4) == fn2(1, 2, 3, 4));
    return 0;
}