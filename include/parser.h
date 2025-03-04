#pragma once
#include "ast.h"
#include "lexer.h"
#include "token.h"

class Parser {
    Lexer& lexer;
    Token tok;
    bool required_semi = true;

    void next_token();
    void consume(TokenKind kind);
    bool kind(TokenKind kind);
    void assert(TokenKind kind);
    StructDecl parse_struct_decl();
    StructField parse_struct_field();
    EnumDecl parse_enum_decl();
    EnumField parse_enum_field();
    ConstDecl parse_const_decl();
    StaticDecl parse_static_decl();
    FuncDecl parse_func_decl();
    std::vector<Param> parse_func_params();
    Param parse_param_decl();
    Block parse_block(bool implicit_return=true);
    std::unique_ptr<Stmt> parse_stmt();
    std::unique_ptr<Expr> prime_parse_expr(int precedence=0);
    std::unique_ptr<Expr> parse_expr(int precedence=0);
    std::unique_ptr<Expr> parse_num();
    ForExpr parse_for_expr();
    IfExpr parse_if_expr();
    LoopExpr parse_loop_expr();
    WhileExpr parse_while_expr();
    Type parse_type();

public:
    Parser(Lexer& lexer): lexer(lexer) {};
    std::vector<std::unique_ptr<Decl>> parse();
};