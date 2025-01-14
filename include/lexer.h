#include "token.h"
#include <vector>

class Lexer {
    std::vector<char>& input;
    size_t start = 0;
    size_t cur = 0;
    size_t line = 0;
    size_t col = 0;
    char next();
    char peek();
    void skip_whitespace();
    Token make_token(TokenKind kind);

public:
    Lexer(std::vector<char>& input) : input(input) {};
    Token lex_token();
};
