#pragma once

#include "src_mgr.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>

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
    KwStatic,
    KwStruct,
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
    Question,
    Dot,
    Arrow,

    Identifier,
    Number,
    String,
    Char,
};

inline const std::string& tok_kind_to_string(const TokenKind kind) {
    static std::unordered_map<TokenKind, const std::string> token_strs = {
        {TokenKind::Eof, "TOK_EOF"},
        {TokenKind::Unknown, "TOK_UNK"},
        {TokenKind::KwAs, "TOK_AS"},
        {TokenKind::KwBreak, "TOK_BREAK"},
        {TokenKind::KwConst, "TOK_CONST"},
        {TokenKind::KwContinue, "TOK_CONTINUE"},
        {TokenKind::KwElse, "TOK_ELSE"},
        {TokenKind::KwEnum, "TOK_ENUM"},
        {TokenKind::KwFalse, "TOK_FALSE"},
        {TokenKind::KwFn, "TOK_FN"},
        {TokenKind::KwFor, "TOK_FOR"},
        {TokenKind::KwIf, "TOK_IF"},
        {TokenKind::KwIn, "TOK_IN"},
        {TokenKind::KwLet, "TOK_LET"},
        {TokenKind::KwLoop, "TOK_LOOP"},
        {TokenKind::KwReturn, "TOK_RETURN"},
        {TokenKind::KwStatic, "TOK_STATIC"},
        {TokenKind::KwStruct, "TOK_STRUCT"},
        {TokenKind::KwTrue, "TOK_TRUE"},
        {TokenKind::KwWhile, "TOK_WHILE"},
        {TokenKind::LParen, "TOK_LPAREN"},
        {TokenKind::RParen, "TOK_RPAREN"},
        {TokenKind::LBrace, "TOK_LBRACE"},
        {TokenKind::RBrace, "TOK_RBRACE"},
        {TokenKind::LBracket, "TOK_LBRACKET"},
        {TokenKind::RBracket, "TOK_RBRACKET"},
        {TokenKind::Plus, "TOK_PLUS"},
        {TokenKind::Minus, "TOK_MINUS"},
        {TokenKind::Star, "TOK_STAR"},
        {TokenKind::Slash, "TOK_SLASH"},
        {TokenKind::Percent, "TOK_PERCENT"},
        {TokenKind::Caret, "TOK_CARET"},
        {TokenKind::Not, "TOK_NOT"},
        {TokenKind::LogicalNot, "TOK_LOGICALNOT"},
        {TokenKind::And, "TOK_AND"},
        {TokenKind::Or, "TOK_OR"},
        {TokenKind::AndAnd, "TOK_ANDAND"},
        {TokenKind::OrOr, "TOK_OROR"},
        {TokenKind::Shl, "TOK_SHL"},
        {TokenKind::Shr, "TOK_SHR"},
        {TokenKind::PlusPlus, "TOK_PLUSPLUS"},
        {TokenKind::MinusMinus, "TOK_MINUSMINUS"},
        {TokenKind::Range, "TOK_RANGE"},
        {TokenKind::RangeEq, "TOK_RANGEEQ"},
        {TokenKind::PlusEq, "TOK_PLUSEQ"},
        {TokenKind::MinusEq, "TOK_MINUSEQ"},
        {TokenKind::StarEq, "TOK_STAREQ"},
        {TokenKind::SlashEq, "TOK_SLASHEQ"},
        {TokenKind::PercentEq, "TOK_PERCENTEQ"},
        {TokenKind::CaretEq, "TOK_CARETEQ"},
        {TokenKind::AndEq, "TOK_ANDEQ"},
        {TokenKind::OrEq, "TOK_OREQ"},
        {TokenKind::ShlEq, "TOK_SHLEQ"},
        {TokenKind::ShrEq, "TOK_SHREQ"},
        {TokenKind::Eq, "TOK_EQ"},
        {TokenKind::EqEq, "TOK_EQEQ"},
        {TokenKind::Ne, "TOK_NE"},
        {TokenKind::Gt, "TOK_GT"},
        {TokenKind::Lt, "TOK_LT"},
        {TokenKind::Ge, "TOK_GE"},
        {TokenKind::Le, "TOK_LE"},
        {TokenKind::Comma, "TOK_COMMA"},
        {TokenKind::Semi, "TOK_SEMI"},
        {TokenKind::Colon, "TOK_COLON"},
        {TokenKind::ColonColon, "TOK_COLONCOLON"},
        {TokenKind::Question, "TOK_QUESTION"},
        {TokenKind::Dot, "TOK_DOT"},
        {TokenKind::Arrow, "TOK_ARROW"},
        {TokenKind::Identifier, "TOK_IDENT"},
        {TokenKind::Number, "TOK_NUM"},
        {TokenKind::String, "TOK_STRING"},
        {TokenKind::Char, "TOK_CHAR"},
    };

    if (const auto str = token_strs.find(kind); str != token_strs.end()) {
        return str->second;
    }

    throw std::runtime_error("Cannot convert tokenkind to string");
}

class Token {
    TokenKind kind;
    Span span;

public:
    Token() = default;
    Token(const TokenKind kind, Span span) : kind(kind), span(span) {}

    [[nodiscard]] TokenKind get_kind() const { return kind; }
    [[nodiscard]] const Span& get_span() const { return span; }

    [[nodiscard]] bool is(const TokenKind kind) const {
        return kind == this->kind;
    }
};
