//
// Created by Maksym Pasichnyk on 28.01.2023.
//

#pragma once

#include "const_string.hpp"
#include "static_vector.hpp"

enum class TokenType {
    End,
    Unknown,

    Identifier,
    Comma,
    Arrow,
    FatArrow,
    Number,
    Colon,
    Semicolon,
    Dollar,
    Plus,
    Minus,
    Asterisk,
    Slash,
    Tilde,
    Not,
    Equal,
    LessThan,
    GreaterThan,
    LessEqual,
    GreaterEqual,

    LeftParen,
    RightParen,
    LeftCurly,
    RightCurly,
    LeftBrace,
    RightBrace,
};

struct Token {
    TokenType type;
    size_t id = 0;

    constexpr auto is(TokenType other) const -> bool {
        return type == other;
    }

    constexpr auto is_end() const {
        return type == TokenType::End;
    }
};

struct SourceStream {
    std::string_view s;
    size_t i;

    constexpr auto token() -> Token {
        while (i < s.size() && is_space_char(s[i])) {
            i++;
        }
        if (i < s.size()) {
            auto from = i;
            if (is_digit(s[i])) {
                size_t number = 0;
                while (i < s.size() && is_digit(s[i])) {
                    number *= 10;
                    number += s[i] - '0';
                    ++i;
                }
                return Token{TokenType::Number, number};
            }
            if (is_identifier_char(s[i])) {
                while (i < s.size() && is_identifier_char(s[i])) {
                    ++i;
                }
                return Token{TokenType::Identifier, fnv1a(s.substr(from, i - from))};
            }
            switch (s[i]) {
                case '+':
                    i += 1;
                    return Token{TokenType::Plus};
                case '-':
                    i += 1;
                    if (i < s.size() && s[i] == '>') {
                        i += 1;
                        return Token{TokenType::Arrow};
                    }
                    return Token{TokenType::Minus};
                case '*':
                    i += 1;
                    return Token{TokenType::Asterisk};
                case '/':
                    i += 1;
                    return Token{TokenType::Slash};
                case '>':
                    i += 1;
                    if (i < s.size() && s[i] == '=') {
                        i += 1;
                        return Token{TokenType::GreaterEqual};
                    }
                    return Token{TokenType::GreaterThan};
                case '<':
                    i += 1;
                    if (i < s.size() && s[i] == '=') {
                        i += 1;
                        return Token{TokenType::LessEqual};
                    }
                    return Token{TokenType::LessThan};
                case '!':
                    i += 1;
                    return Token{TokenType::Not};
                case '~':
                    i += 1;
                    return Token{TokenType::Tilde};
                case '(':
                    i += 1;
                    return Token{TokenType::LeftParen};
                case ')':
                    i += 1;
                    return Token{TokenType::RightParen};
                case '{':
                    i += 1;
                    return Token{TokenType::LeftCurly};
                case '}':
                    i += 1;
                    return Token{TokenType::RightCurly};
                case '[':
                    i += 1;
                    return Token{TokenType::LeftBrace};
                case ']':
                    i += 1;
                    return Token{TokenType::RightBrace};
                case ',':
                    i += 1;
                    return Token{TokenType::Comma};
                case ':':
                    i += 1;
                    return Token{TokenType::Colon};
                case ';':
                    i += 1;
                    return Token{TokenType::Semicolon};
                case '$':
                    i += 1;
                    return Token{TokenType::Dollar};
                case '=':
                    i += 1;
                    if (i < s.size() && s[i] == '>') {
                        i += 1;
                        return Token{TokenType::FatArrow};
                    }
                    return Token{TokenType::Equal};
                default:
                    i += 1;
                    return Token{TokenType::Unknown};
            }
        }
        return Token{TokenType::End};
    }

    static constexpr auto is_space_char(char c) -> bool {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }
    static constexpr auto is_identifier_char(char c) -> bool {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    }
    static constexpr auto is_digit(char c) -> bool {
        return (c >= '0' && c <= '9');
    }
};

struct TokenStream {
    // todo: dynamic size?
    static_vector<Token, 1024> tokens;
    size_t pos{};

    constexpr auto token() const -> Token {
        return tokens[pos];
    }

    constexpr auto at(size_t where) const -> TokenStream {
        return {tokens, where};
    }

    constexpr auto current() const -> size_t {
        return pos;
    }

    static constexpr auto parse(std::string_view s) -> TokenStream {
        SourceStream source_stream{s, 0};
        static_vector<Token, 1024> tokens{};
        auto tk = source_stream.token();
        while (!tk.is_end()) {
            tokens.emplace_back(tk);
            tk = source_stream.token();
        }
        return TokenStream{tokens, 0};
    }
};
