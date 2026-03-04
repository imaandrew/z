#pragma once

#include "diag/src_mgr.h"
#include <cstdint>
#include <magic_enum/magic_enum_format.hpp> // NOLINT

namespace z {

/// Represents the different kinds of tokens that can be lexed.
enum class TokenKind : std::uint8_t {
    Eof,
    Unknown,

    // Keywords
    KwAs,
    KwBreak,
    KwConst,
    KwContinue,
    KwElse,
    KwEnum,
    KwFalse,
    KwFn,
    KwFor,
    KwIf,
    KwIn,
    KwLet,
    KwLoop,
    KwReturn,
    KwSelf,
    KwStatic,
    KwStruct,
    KwTrait,
    KwTrue,
    KwWhile,

    // Punctuator
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,

    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Caret,
    Not,
    LogicalNot,
    And,
    Or,
    AndAnd,
    OrOr,
    Shl,
    Shr,
    PlusPlus,
    MinusMinus,
    Range,
    RangeEq,
    PlusEq,
    MinusEq,
    StarEq,
    SlashEq,
    PercentEq,
    CaretEq,
    AndEq,
    OrEq,
    ShlEq,
    ShrEq,
    Eq,
    EqEq,
    Ne,
    Gt,
    Lt,
    Ge,
    Le,
    Comma,
    Semi,
    Colon,
    ColonColon,
    Dot,
    Arrow,

    Identifier,
    Number,
    String,
    Char,
};

constexpr std::string_view tok_kind_to_string(const TokenKind kind) {
    return magic_enum::enum_name(kind);
}

constexpr std::string_view operator_to_string(const TokenKind kind) {
    switch (kind) {
    case TokenKind::LParen:
        return "(";
    case TokenKind::RParen:
        return ")";
    case TokenKind::LBrace:
        return "{";
    case TokenKind::RBrace:
        return "}";
    case TokenKind::LBracket:
        return "[";
    case TokenKind::RBracket:
        return "]";
    case TokenKind::Plus:
        return "+";
    case TokenKind::Minus:
        return "-";
    case TokenKind::Star:
        return "*";
    case TokenKind::Slash:
        return "/";
    case TokenKind::Percent:
        return "%";
    case TokenKind::Caret:
        return "^";
    case TokenKind::Not:
        return "!";
    case TokenKind::LogicalNot:
        return "~";
    case TokenKind::And:
        return "&";
    case TokenKind::Or:
        return "|";
    case TokenKind::AndAnd:
        return "&&";
    case TokenKind::OrOr:
        return "||";
    case TokenKind::Shl:
        return "<<";
    case TokenKind::Shr:
        return ">>";
    case TokenKind::PlusPlus:
        return "++";
    case TokenKind::MinusMinus:
        return "--";
    case TokenKind::Range:
        return "..";
    case TokenKind::RangeEq:
        return "..=";
    case TokenKind::PlusEq:
        return "+=";
    case TokenKind::MinusEq:
        return "-=";
    case TokenKind::StarEq:
        return "*=";
    case TokenKind::SlashEq:
        return "/=";
    case TokenKind::PercentEq:
        return "%=";
    case TokenKind::CaretEq:
        return "^=";
    case TokenKind::AndEq:
        return "&=";
    case TokenKind::OrEq:
        return "|=";
    case TokenKind::ShlEq:
        return "<<=";
    case TokenKind::ShrEq:
        return ">>=";
    case TokenKind::Eq:
        return "=";
    case TokenKind::EqEq:
        return "==";
    case TokenKind::Ne:
        return "!=";
    case TokenKind::Gt:
        return ">";
    case TokenKind::Lt:
        return "<";
    case TokenKind::Ge:
        return ">=";
    case TokenKind::Le:
        return "<=";
    case TokenKind::Comma:
        return ",";
    case TokenKind::Semi:
        return ";";
    case TokenKind::Colon:
        return ":";
    case TokenKind::ColonColon:
        return "::";
    case TokenKind::Dot:
        return ".";
    case TokenKind::Arrow:
        return "->";
    default:
        panic("Not valid operator: {}", kind);
    }
}

class Token {
    TokenKind kind;
    Span span;

public:
    Token() = default;
    Token(const TokenKind kind, Span span) : kind(kind), span(span) {}

    [[nodiscard]] TokenKind get_kind() const { return kind; }
    [[nodiscard]] Span get_span() const { return span; }

    [[nodiscard]] bool is(const TokenKind kind) const {
        return kind == this->kind;
    }
};
} // namespace z
