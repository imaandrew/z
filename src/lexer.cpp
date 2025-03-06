#include "lexer.h"
#include "token.h"
#include <cctype>
#include <climits>
#include <stdexcept>
#include <string>
#include <unordered_map>

const static std::unordered_map<std::string, TokenKind> KEYWORDS = {
    {"as", TokenKind::KwAs},
    {"break", TokenKind::KwBreak},
    {"const", TokenKind::KwConst},
    {"continue", TokenKind::KwContinue},
    {"else", TokenKind::KwElse},
    {"enum", TokenKind::KwEnum},
    {"false", TokenKind::KwFalse},
    {"fn", TokenKind::KwFn},
    {"for", TokenKind::KwFor},
    {"if", TokenKind::KwIf},
    {"in", TokenKind::KwIf},
    {"let", TokenKind::KwLet},
    {"loop", TokenKind::KwLoop},
    {"return", TokenKind::KwReturn},
    {"static", TokenKind::KwStatic},
    {"struct", TokenKind::KwStruct},
    {"true", TokenKind::KwTrue},
    {"while", TokenKind::KwWhile}
};

char Lexer::next() {
    try {
        return input.at(cur++);
    } catch (std::out_of_range) {
        return '\0';
    }
}

char Lexer::peek() const {
    try {
        return input.at(cur);
    } catch (std::out_of_range) {
        return '\0';
    }
}

void Lexer::skip_whitespace() {
    auto c = peek();
    while (true) {
        if (c == '\n') {
            col = 1;
            line++;
            line_start = cur + 1;
        }
        if (!std::isspace(static_cast<unsigned char>(c)))
            break;
        next();
        c = peek();
    }
}

Token Lexer::make_token(TokenKind kind) const {
    return Token(kind, input.data() + start, start, line, col, cur - start);
}

bool Lexer::at_end() {
    return cur >= input.size();
}

Token Lexer::lex_token() {
    skip_whitespace();
    col = cur - line_start + 1;
    start = cur;

    auto c = next();
    switch (c) {
        case '(': return make_token(TokenKind::LParen);
        case ')': return make_token(TokenKind::RParen);
        case '{': return make_token(TokenKind::LBrace);
        case '}': return make_token(TokenKind::RBrace);
        case '[': return make_token(TokenKind::LBracket);
        case ']': return make_token(TokenKind::RBracket);
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
                while (peek() != '\n') next();
                return lex_token();
            }
            if (peek() == '*') {
                next();
                int i = 1;
                while (i > 0) {
                    auto c = next();
                    if (c == '*' && peek() == '/') {
                        i--;
                        next();
                    } else if (c == '/' && peek() == '*') {
                        i++;
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
        case '~': return make_token(TokenKind::LogicalNot);
        case '!': {
            if (peek() == '=') {
                next();
                return make_token(TokenKind::Ne);
            }
            return make_token(TokenKind::Not);
        }
        case '&': {
            switch (peek()) {
                case '&': next(); return make_token(TokenKind::AndAnd);
                case '=': next(); return make_token(TokenKind::AndEq);
                default: return make_token(TokenKind::And);
            }
        }
        case '|': {
            switch (peek()) {
                case '|': next(); return make_token(TokenKind::Or);
                case '=': next(); return make_token(TokenKind::OrEq);
                default: return make_token(TokenKind::Or);
            }
        }
        case '.' : {
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
            c = peek();
            if (c == '<') {
                next(); c = peek();
                if (c == '=') {
                    next();
                    return make_token(TokenKind::ShlEq);
                }
                return make_token(TokenKind::Shl);
            }
            if (c == '=') {
                next();
                return make_token(TokenKind::Le);
            }
            return make_token(TokenKind::Lt);

        }
        case '>': {
            c = peek();
            if (c == '>') {
                next(); c = peek();
                if (c == '=') {
                    next();
                    return make_token(TokenKind::ShrEq);
                }
                return make_token(TokenKind::Shr);
            }
            if (c == '=') {
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
        case ',': return make_token(TokenKind::Comma);
        case ';': return make_token(TokenKind::Semi);
        case ':': {
            if (peek() == ':') {
                next();
                return make_token(TokenKind::ColonColon);
            }
            return make_token(TokenKind::Colon);
        }
        case '\0': return make_token(TokenKind::Eof);
        case '"': {
            while (peek() != '"') {
                next();

                if (peek() == '\\') {
                    next();
                    if (peek() == '"')
                        next();
                }
            }
            auto t = Token(TokenKind::String, input.data() + start + 1, start, line, col, cur - start - 1);
            next();
            return t;
        }
        default: {
            if (std::isalpha(static_cast<unsigned char>(c))) {
                c = peek();
                while (std::isalpha(static_cast<unsigned char>(c)) || std::isdigit(static_cast<unsigned char>(c)) || c == '_') {
                    next();
                    c = peek();
                }

                auto len = cur - start;
                auto literal = std::string(input.data() + start, len);

                if (auto kind = KEYWORDS.find(literal); kind != KEYWORDS.end()) {
                    return make_token(kind->second);
                }

                return make_token(TokenKind::Identifier);
            }

            if (std::isdigit(static_cast<unsigned char>(c))) {
                c = peek();
                while (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
                    next();
                    c = peek();
                }

                return make_token(TokenKind::Number);
            }
        }
    }

    return make_token(TokenKind::Unknown);
}
