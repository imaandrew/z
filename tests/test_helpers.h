#pragma once

#include "lexer.h"
#include "token.h"
#include <vector>

namespace z::test {
inline std::vector<Token> tokenize(SourceManager& sm) {
    auto lexer = Lexer(&sm);
    std::vector<Token> tokens;

    auto tok = lexer.lex_token();
    while (!tok.is(TokenKind::Eof)) {
        tokens.push_back(tok);
        tok = lexer.lex_token();
    }

    return tokens;
}
} // namespace z::test
