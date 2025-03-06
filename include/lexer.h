#pragma once

#include "token.h"
#include <vector>

class Lexer {
    std::vector<char>& input;
    size_t start = 0;
    size_t cur = 0;
    size_t line = 1;
    size_t col = 1;
    size_t line_start = 0;
    char next();
    char peek() const;
    void skip_whitespace();
    Token make_token(TokenKind kind) const;

public:
    Lexer(std::vector<char>& input) : input(input) {};
    Token lex_token();
    bool at_end();
};
