#include "lexer.h"
#include "token.h"
#include <cassert>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_map>

namespace z {

namespace {
TokenKind get_keyword(const std::string& keyword) {
    static std::unordered_map<std::string_view, TokenKind> keywords = {
        {"as", TokenKind::KwAs},         {"break", TokenKind::KwBreak},
        {"const", TokenKind::KwConst},   {"continue", TokenKind::KwContinue},
        {"else", TokenKind::KwElse},     {"enum", TokenKind::KwEnum},
        {"false", TokenKind::KwFalse},   {"fn", TokenKind::KwFn},
        {"for", TokenKind::KwFor},       {"if", TokenKind::KwIf},
        {"in", TokenKind::KwIn},         {"let", TokenKind::KwLet},
        {"loop", TokenKind::KwLoop},     {"return", TokenKind::KwReturn},
        {"self", TokenKind::KwSelf},     {"static", TokenKind::KwStatic},
        {"struct", TokenKind::KwStruct}, {"trait", TokenKind::KwTrait},
        {"true", TokenKind::KwTrue},     {"while", TokenKind::KwWhile}};

    if (const auto kind = keywords.find(keyword); kind != keywords.end()) {
        return kind->second;
    }

    return TokenKind::Unknown;
}
} // namespace

char Lexer::next() {
    const auto next_char = peek();
    cur++;

    return next_char;
}

char Lexer::peek() const {
    const auto cur_char = source->get_char(cur);

    if (!cur_char)
        return '\0';

    return cur_char.value();
}

void Lexer::skip_whitespace() {
    char cur_char = peek();

    while (true) {
        if (cur_char == '\n') {
            col = 1;
            line++;
            line_start = cur + 1;
        }
        if (std::isspace(static_cast<unsigned char>(cur_char)) == 0)
            break;
        next();
        cur_char = peek();
    }
}

Token Lexer::make_token(const TokenKind kind) const {
    return Token(kind, Span(start, cur - start));
}

Token Lexer::lex_token() {
    skip_whitespace();

    col = cur - line_start + 1;
    start = cur;

    switch (auto cur_char = next()) {
    case '(':
        return make_token(TokenKind::LParen);
    case ')':
        return make_token(TokenKind::RParen);
    case '{':
        return make_token(TokenKind::LBrace);
    case '}':
        return make_token(TokenKind::RBrace);
    case '[':
        return make_token(TokenKind::LBracket);
    case ']':
        return make_token(TokenKind::RBracket);
    case '+': {
        if (peek() == '=') {
            next();
            return make_token(TokenKind::PlusEq);
        }
        if (peek() == '+') {
            next();
            return make_token(TokenKind::PlusPlus);
        }
        return make_token(TokenKind::Plus);
    }
    case '-': {
        if (peek() == '=') {
            next();
            return make_token(TokenKind::MinusEq);
        }
        if (peek() == '-') {
            next();
            return make_token(TokenKind::MinusMinus);
        }
        if (peek() == '>') {
            next();
            return make_token(TokenKind::Arrow);
        }
        return make_token(TokenKind::Minus);
    }
    case '*': {
        if (peek() == '=') {
            next();
            return make_token(TokenKind::StarEq);
        }
        return make_token(TokenKind::Star);
    }
    case '/': {
        if (peek() == '=') {
            next();
            return make_token(TokenKind::SlashEq);
        }
        if (peek() == '/') {
            next();
            while (peek() != '\n' && peek() != '\0')
                next();
            return lex_token();
        }
        if (peek() == '*') {
            next();
            int comment_level = 1;
            while (comment_level > 0 && peek() != '\0') {
                if (const auto next_char = next();
                    next_char == '*' && peek() == '/') {
                    comment_level--;
                    next();
                } else if (next_char == '/' && peek() == '*') {
                    comment_level++;
                    next();
                }
            }
            return lex_token();
        }
        return make_token(TokenKind::Slash);
    }
    case '%': {
        if (peek() == '=') {
            next();
            return make_token(TokenKind::PercentEq);
        }
        return make_token(TokenKind::Percent);
    }
    case '^': {
        if (peek() == '=') {
            next();
            return make_token(TokenKind::CaretEq);
        }
        return make_token(TokenKind::Caret);
    }
    case '~':
        return make_token(TokenKind::Not);
    case '!': {
        if (peek() == '=') {
            next();
            return make_token(TokenKind::Ne);
        }
        return make_token(TokenKind::LogicalNot);
    }
    case '&': {
        switch (peek()) {
        case '&':
            next();
            return make_token(TokenKind::AndAnd);
        case '=':
            next();
            return make_token(TokenKind::AndEq);
        default:
            return make_token(TokenKind::And);
        }
    }
    case '|': {
        switch (peek()) {
        case '|':
            next();
            return make_token(TokenKind::Or);
        case '=':
            next();
            return make_token(TokenKind::OrEq);
        default:
            return make_token(TokenKind::Or);
        }
    }
    case '.': {
        if (peek() == '.') {
            next();
            if (peek() == '=') {
                next();
                return make_token(TokenKind::RangeEq);
            }
            return make_token(TokenKind::Range);
        }
        return make_token(TokenKind::Dot);
    }
    case '<': {
        if (peek() == '<') {
            next();
            if (peek() == '=') {
                next();
                return make_token(TokenKind::ShlEq);
            }
            return make_token(TokenKind::Shl);
        }
        if (peek() == '=') {
            next();
            return make_token(TokenKind::Le);
        }
        return make_token(TokenKind::Lt);
    }
    case '>': {
        if (peek() == '>') {
            next();
            if (peek() == '=') {
                next();
                return make_token(TokenKind::ShrEq);
            }
            return make_token(TokenKind::Shr);
        }
        if (peek() == '=') {
            next();
            return make_token(TokenKind::Ge);
        }
        return make_token(TokenKind::Gt);
    }
    case '=': {
        if (peek() == '=') {
            next();
            return make_token(TokenKind::EqEq);
        }
        return make_token(TokenKind::Eq);
    }
    case ',':
        return make_token(TokenKind::Comma);
    case ';':
        return make_token(TokenKind::Semi);
    case '?':
        return make_token(TokenKind::Question);
    case ':': {
        if (peek() == ':') {
            next();
            return make_token(TokenKind::ColonColon);
        }
        return make_token(TokenKind::Colon);
    }
    case '\0':
        return make_token(TokenKind::Eof);
    case '"': {
        while (peek() != '"' && peek() != '\0') {
            next();

            if (peek() == '\\') {
                next();
                if (peek() == '"')
                    next();
            }
        }
        auto tok = Token(TokenKind::String, Span(start + 1, cur - start - 1));
        next();
        return tok;
    }
    case '\'': {
        while (peek() != '\'' && peek() != '\0') {
            next();
        }
        auto tok = Token(TokenKind::Char, Span(start + 1, cur - start - 1));
        next();
        return tok;
    }
    default: {
        // NOLINTBEGIN(readability-implicit-bool-conversion)
        if (std::isalpha(static_cast<unsigned char>(cur_char)) ||
            cur_char == '_') {
            cur_char = peek();
            while (std::isalpha(static_cast<unsigned char>(cur_char)) ||
                   std::isdigit(static_cast<unsigned char>(cur_char)) ||
                   cur_char == '_') {
                next();
                cur_char = peek();
            }

            const auto len = cur - start;
            const auto literal = std::string(source->get_char_ptr(start), len);

            if (const auto keyword = get_keyword(literal);
                keyword != TokenKind::Unknown) {
                return make_token(keyword);
            }

            return make_token(TokenKind::Identifier);
        }

        if (std::isdigit(static_cast<unsigned char>(cur_char))) {
            cur_char = peek();
            while (std::isxdigit(static_cast<unsigned char>(cur_char)) ||
                   cur_char == '.' || cur_char == '_' || cur_char == 'x' ||
                   cur_char == 'X' || cur_char == 'o' || cur_char == 'O') {
                next();
                cur_char = peek();
            }

            return make_token(TokenKind::Number);
        }
        // NOLINTEND(readability-implicit-bool-conversion)
    }
    }

    return make_token(TokenKind::Unknown);
}
} // namespace z
