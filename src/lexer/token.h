#pragma once

#include "diag/src_mgr.h"
#include <cstdint>

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
