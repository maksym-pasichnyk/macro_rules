//
// Created by Maksym Pasichnyk on 28.01.2023.
//

#pragma once

#include "meta.hpp"
#include "expression.hpp"

template<typename T>
struct MacroList : meta::parse::list<T> {
    consteval static auto transform(auto ctx) {
        return unwrap(ctx.value());
    }

    template<typename Arg0, typename... U>
    consteval static auto unwrap(const std::tuple<Arg0, U...>&) {
        if constexpr (sizeof...(U) == 0) {
            return Arg0{};
        } else {
            return meta::parse::group<Arg0, U...>{};
        }
    }
};

struct MacroIdent : meta::parse::token<TokenType::Identifier> {
    consteval static auto transform(auto ctx) {
        return meta::parse::ident<ctx.value().id>{};
    }
};

template<TokenType type>
struct MacroToken : meta::parse::token<type> {
    consteval static auto transform(auto ctx) {
        return meta::parse::token<type>{};
    }
};

using MacroFragSpec = meta::parse::one_of<
    meta::parse::ident<fnv1a("number")>,
    meta::parse::ident<fnv1a("ident")>,
    meta::parse::ident<fnv1a("expr")>
>;

struct MacroFragment : meta::parse::group<
    meta::parse::token<TokenType::Dollar>,
    meta::parse::token<TokenType::Identifier>,
    meta::parse::token<TokenType::Colon>,
    MacroFragSpec
> {
    consteval static auto transform(auto ctx) {
        constexpr auto name = std::get<1>(ctx.value()).id;
        constexpr auto spec = std::get<3>(ctx.value()).id;

        if constexpr (spec == fnv1a("number")) {
            return meta::parse::with_name<name, meta::parse::token<TokenType::Number>>{};
        }
        if constexpr (spec == fnv1a("ident")) {
            return meta::parse::with_name<name, meta::parse::token<TokenType::Identifier>>{};
        }
        if constexpr (spec == fnv1a("expr")) {
            return meta::parse::with_name<name, Expression>{};
        }
    }
};

using MacroRepSep = meta::parse::one_of<
    meta::parse::token<TokenType::Comma>,
    meta::parse::token<TokenType::Colon>,
    meta::parse::token<TokenType::Semicolon>
>;

using MacroRepOp = meta::parse::one_of<
    meta::parse::token<TokenType::Plus>,
    meta::parse::token<TokenType::Asterisk>
>;

struct MacroMatch;

struct MacroRepDef : meta::parse::group<
    meta::parse::token<TokenType::Dollar>,
    meta::parse::token<TokenType::LeftParen>,
    MacroList<MacroMatch>,
    meta::parse::token<TokenType::RightParen>,
    meta::parse::opt<MacroRepSep>,
    MacroRepOp
> {
    consteval static auto transform(auto ctx) {
        constexpr auto arg2 = std::get<2>(ctx.value());
        constexpr auto arg4 = std::get<4>(ctx.value());
        constexpr auto arg5 = std::get<5>(ctx.value());
        if constexpr (!meta::parse::is_none(arg5)) {
            using Item = std::decay_t<decltype(arg2)>;
            if constexpr (arg5.is(TokenType::Asterisk)) {
                return meta::parse::list<Item>{};
            } else {
                return meta::parse::list_non_empty<Item>{};
            }
        } else {
            return arg2;
        }
    }
};

struct MacroMatcher : meta::parse::one_of<
    meta::parse::group<
        MacroToken<TokenType::LeftCurly>,
        MacroList<MacroMatch>,
        MacroToken<TokenType::RightCurly>
    >,
    meta::parse::group<
        MacroToken<TokenType::LeftParen>,
        MacroList<MacroMatch>,
        MacroToken<TokenType::RightParen>
    >,
    meta::parse::group<
        MacroToken<TokenType::LeftBrace>,
        MacroList<MacroMatch>,
        MacroToken<TokenType::RightBrace>
    >
> {
    template<typename... U>
    consteval static auto unwrap(const std::tuple<U...>&) {
        return meta::parse::group<U...>{};
    }

    consteval static auto transform(auto ctx) {
        return unwrap(ctx.value());
    }
};

struct MacroMatch : meta::parse::one_of<
    MacroRepDef,
    MacroFragment,
    MacroIdent,
    MacroMatcher,
    // todo: any_token ?
    meta::parse::one_of<
        MacroToken<TokenType::Identifier>,
        MacroToken<TokenType::Comma>,
        MacroToken<TokenType::Arrow>,
        MacroToken<TokenType::FatArrow>,
        MacroToken<TokenType::Number>,
        MacroToken<TokenType::Colon>,
        MacroToken<TokenType::Semicolon>,
        MacroToken<TokenType::Plus>,
        MacroToken<TokenType::Minus>,
        MacroToken<TokenType::Asterisk>,
        MacroToken<TokenType::Slash>,
        MacroToken<TokenType::Tilde>,
        MacroToken<TokenType::Not>,
        MacroToken<TokenType::Equal>,
        MacroToken<TokenType::LessThan>,
        MacroToken<TokenType::GreaterThan>,
        MacroToken<TokenType::LessEqual>,
        MacroToken<TokenType::GreaterEqual>
    >
> {};

struct MacroRules : MacroList<MacroMatch> {
};

#define macro_rules(...) decltype(apply_rules(MacroRules, __VA_ARGS__))
