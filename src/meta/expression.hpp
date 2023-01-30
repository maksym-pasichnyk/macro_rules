//
// Created by Maksym Pasichnyk on 28.01.2023.
//

#pragma once

#include "meta.hpp"

struct Variable {
    size_t id;
};

struct VariableExpression : meta::parse::token<TokenType::Identifier> {
    consteval static auto transform(auto ctx) {
        return [](auto ctx_, const auto& args) {
            constexpr auto name = Variable{decltype(ctx){}.value().id};
            return ctx_.template get<name>(args);
        };
    }
};

struct NumberLiteralExpression : meta::parse::token<TokenType::Number> {
    consteval static auto transform(auto ctx) {
        return [](auto ctx_, const auto& args) {
            return static_cast<int>(decltype(ctx){}.value().id);
        };
    }
};

template<typename T>
struct WithParens : meta::parse::group<
    meta::parse::token<TokenType::LeftParen>,
    T,
    meta::parse::token<TokenType::RightParen>
> {
    consteval static auto transform(auto ctx) {
        return std::get<1>(ctx.value());
    }
};

struct Expression;

struct PrimitiveExpression : meta::parse::one_of<VariableExpression, NumberLiteralExpression, WithParens<Expression>> {

};

struct UnaryExpression {
    struct prefix : meta::parse::group<
        meta::parse::one_of<
            meta::parse::token<TokenType::Plus>,
            meta::parse::token<TokenType::Minus>,
            meta::parse::token<TokenType::Not>,
            meta::parse::token<TokenType::Tilde>
        >,
        UnaryExpression
    > {
        consteval static auto transform(auto ctx) {
            return [](auto ctx_, const auto& args) {
                constexpr auto op = std::get<0>(decltype(ctx){}.value());
                auto arg = std::get<1>(decltype(ctx){}.value())(ctx_, args);

                if constexpr (op.is(TokenType::Plus)) {
                    return +arg;
                }
                if constexpr (op.is(TokenType::Minus)) {
                    return -arg;
                }
                if constexpr (op.is(TokenType::Not)) {
                    return !arg;
                }
                if constexpr (op.is(TokenType::Tilde)) {
                    return ~arg;
                }
            };
        }
    };

    consteval static auto compile(auto stream) {
        return meta::parse::one_of<prefix, PrimitiveExpression>::compile(stream);
    }
};

struct MultiplicationExpression {
    struct infix : meta::parse::group<
        UnaryExpression,
        meta::parse::one_of<
            meta::parse::token<TokenType::Asterisk>,
            meta::parse::token<TokenType::Slash>
        >,
        MultiplicationExpression
    > {
        consteval static auto transform(auto ctx) {
            return [] (auto ctx_, const auto& args) {
                constexpr auto op = std::get<1>(decltype(ctx){}.value());
                auto lhs = std::get<0>(decltype(ctx){}.value())(ctx_, args);
                auto rhs = std::get<2>(decltype(ctx){}.value())(ctx_, args);

                if constexpr (op.is(TokenType::Asterisk)) {
                    return lhs * rhs;
                }
                if constexpr (op.is(TokenType::Slash)) {
                    return lhs / rhs;
                }
            };
        }
    };

    consteval static auto compile(auto stream) {
        return meta::parse::one_of<infix, UnaryExpression>::compile(stream);
    }
};

struct AdditionExpression {
    struct infix : meta::parse::group<
        MultiplicationExpression,
        meta::parse::one_of<
            meta::parse::token<TokenType::Plus>,
            meta::parse::token<TokenType::Minus>
        >,
        AdditionExpression
    > {
        consteval static auto transform(auto ctx) {
            return [] (auto ctx_, const auto& args) {
                constexpr auto op = std::get<1>(decltype(ctx){}.value());
                auto lhs = std::get<0>(decltype(ctx){}.value())(ctx_, args);
                auto rhs = std::get<2>(decltype(ctx){}.value())(ctx_, args);

                if constexpr (op.is(TokenType::Plus)) {
                    return lhs + rhs;
                }
                if constexpr (op.is(TokenType::Minus)) {
                    return lhs - rhs;
                }
            };
        }
    };

    consteval static auto compile(auto stream) {
        return meta::parse::one_of<infix, MultiplicationExpression>::compile(stream);
    }
};

struct ComparisonExpression {
    struct infix : WithParens<meta::parse::group<
        AdditionExpression,
        meta::parse::one_of<
            meta::parse::token<TokenType::LessThan>,
            meta::parse::token<TokenType::LessEqual>,
            meta::parse::token<TokenType::GreaterThan>,
            meta::parse::token<TokenType::GreaterEqual>
        >,
        AdditionExpression
    >> {
        consteval static auto transform(auto ctx) {
            return [] (auto ctx_, const auto& args) {
                constexpr auto op = std::get<1>(decltype(ctx){}.value());
                auto lhs = std::get<0>(decltype(ctx){}.value())(ctx_, args);
                auto rhs = std::get<2>(decltype(ctx){}.value())(ctx_, args);

                if constexpr (op.is(TokenType::LessThan)) {
                    return lhs < rhs;
                }
                if constexpr (op.is(TokenType::LessEqual)) {
                    return lhs <= rhs;
                }
                if constexpr (op.is(TokenType::GreaterThan)) {
                    return lhs > rhs;
                }
                if constexpr (op.is(TokenType::GreaterEqual)) {
                    return lhs >= rhs;
                }
            };
        }
    };

    consteval static auto compile(auto stream) {
        return meta::parse::one_of<infix, AdditionExpression>::compile(stream);
    }
};

struct Expression : ComparisonExpression {};