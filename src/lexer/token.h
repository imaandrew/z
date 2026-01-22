#pragma once

#include "diag/src_mgr.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace z {

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
        {TokenKind::KwSelf, "TOK_SELF"},
        {TokenKind::KwStatic, "TOK_STATIC"},
        {TokenKind::KwStruct, "TOK_STRUCT"},
        {TokenKind::KwTrait, "TOK_TRAIT"},
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

    std::unreachable();
}

inline const std::string& operator_to_string(const TokenKind kind) {
    static std::unordered_map<TokenKind, const std::string> token_strs = {
        {TokenKind::LParen, "("},      {TokenKind::RParen, ")"},
        {TokenKind::LBrace, "{"},      {TokenKind::RBrace, "}"},
        {TokenKind::LBracket, "["},    {TokenKind::RBracket, "]"},
        {TokenKind::Plus, "+"},        {TokenKind::Minus, "-"},
        {TokenKind::Star, "*"},        {TokenKind::Slash, "/"},
        {TokenKind::Percent, "%"},     {TokenKind::Caret, "^"},
        {TokenKind::Not, "!"},         {TokenKind::LogicalNot, "~"},
        {TokenKind::And, "&"},         {TokenKind::Or, "|"},
        {TokenKind::AndAnd, "&&"},     {TokenKind::OrOr, "||"},
        {TokenKind::Shl, "<<"},        {TokenKind::Shr, ">>"},
        {TokenKind::PlusPlus, "++"},   {TokenKind::MinusMinus, "--"},
        {TokenKind::Range, ".."},      {TokenKind::RangeEq, "..="},
        {TokenKind::PlusEq, "+="},     {TokenKind::MinusEq, "-="},
        {TokenKind::StarEq, "*="},     {TokenKind::SlashEq, "/="},
        {TokenKind::PercentEq, "%="},  {TokenKind::CaretEq, "^="},
        {TokenKind::AndEq, "&="},      {TokenKind::OrEq, "|="},
        {TokenKind::ShlEq, "<<="},     {TokenKind::ShrEq, ">>="},
        {TokenKind::Eq, "="},          {TokenKind::EqEq, "=="},
        {TokenKind::Ne, "!="},         {TokenKind::Gt, ">"},
        {TokenKind::Lt, "<"},          {TokenKind::Ge, ">="},
        {TokenKind::Le, "<="},         {TokenKind::Comma, ","},
        {TokenKind::Semi, ";"},        {TokenKind::Colon, ":"},
        {TokenKind::ColonColon, "::"}, {TokenKind::Question, "?"},
        {TokenKind::Dot, "."},         {TokenKind::Arrow, "->"},
    };

    if (const auto str = token_strs.find(kind); str != token_strs.end()) {
        return str->second;
    }

    throw std::runtime_error("Not valid operator");
}

constexpr bool is_assignment_op(TokenKind kind) {
    switch (kind) {
    case TokenKind::PlusPlus:
    case TokenKind::MinusMinus:
    case TokenKind::PlusEq:
    case TokenKind::MinusEq:
    case TokenKind::StarEq:
    case TokenKind::SlashEq:
    case TokenKind::PercentEq:
    case TokenKind::CaretEq:
    case TokenKind::AndEq:
    case TokenKind::OrEq:
    case TokenKind::ShlEq:
    case TokenKind::ShrEq:
    case TokenKind::Eq:
        return true;
    default:
        return false;
    }
}

constexpr bool is_division_op(TokenKind kind) {
    switch (kind) {
    case TokenKind::Slash:
    case TokenKind::SlashEq:
    case TokenKind::Percent:
    case TokenKind::PercentEq:
        return true;
    default:
        return false;
    }
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
} // namespace z
