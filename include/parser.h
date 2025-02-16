#pragma once
#include "ast.h"
#include "lexer.h"
#include "token.h"

class Parser {
    Lexer& lexer;
    Token tok;

    void next_token();
    void consume(TokenKind kind);
    bool kind(TokenKind kind);
    void assert(TokenKind kind);
    FuncDecl parse_func_decl();
    std::vector<Param> parse_func_params();
    Param parse_param_decl();
    Block parse_block();
    std::unique_ptr<Stmt> parse_stmt();
    std::unique_ptr<Expr> prime_parse_expr(int precedence=0);
    std::unique_ptr<Expr> parse_expr(int precedence=0);
    std::unique_ptr<Expr> parse_num();
    ForExpr parse_for_expr();
    IfExpr parse_if_expr();
    LoopExpr parse_loop_expr();
    WhileExpr parse_while_expr();

public:
    Parser(Lexer& lexer): lexer(lexer) {};
    FuncDecl parse();
};