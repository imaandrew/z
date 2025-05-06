#pragma once

#include "sourceman.h"
#include "token.h"

class Lexer {
    SourceManager* source;
    size_t start = 0;
    size_t cur = 0;
    size_t line = 1;
    size_t col = 1;
    size_t line_start = 0;
    char next();
    [[nodiscard]] char peek() const;
    void skip_whitespace();
    [[nodiscard]] Token make_token(TokenKind kind) const;

public:
    explicit Lexer(SourceManager* source) : source(source) {};
    Token lex_token();
};
