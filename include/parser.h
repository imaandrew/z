#pragma once
#include "ast.h"
#include "error.h"
#include "lexer.h"
#include "sourceman.h"
#include "token.h"
#include <utility>

using StmtResult = Result<std::unique_ptr<Stmt>>;
using ExprResult = Result<std::unique_ptr<Expr>>;
using DeclResult = Result<std::unique_ptr<Decl>>;
using TypeResult = Result<Type>;

inline StmtResult StmtError() { return StmtResult(false); }
inline ExprResult ExprError() { return ExprResult(false); }
inline DeclResult DeclError() { return DeclResult(false); }
inline TypeResult TypeError() { return TypeResult(false); }

enum class SyncFlags : unsigned char {
    None = 0,
    StopAtSemi = 1 << 0,
    BreakBefore = 1 << 1,
};

inline SyncFlags operator|(const SyncFlags& lhs, const SyncFlags& rhs) {
    return static_cast<SyncFlags>(std::to_underlying(lhs) |
                                  std::to_underlying(rhs));
}

inline SyncFlags operator&(const SyncFlags& lhs, const SyncFlags& rhs) {
    return static_cast<SyncFlags>(std::to_underlying(lhs) &
                                  std::to_underlying(rhs));
}

class Parser {
    Lexer& lexer;
    DiagnosticEmitter diag;
    Token tok;
    Token prev_tok;
    bool required_semi = true;

    void next_token();
    bool consume(TokenKind kind);
    bool kind(TokenKind kind);
    bool assert(TokenKind kind);
    bool sync(TokenKind kind, SyncFlags = static_cast<SyncFlags>(0));
    bool sync(std::vector<TokenKind> kinds,
              SyncFlags = static_cast<SyncFlags>(0));
    void recover_decl();
    void recover_stmt();
    bool can_be_expr();
    DeclResult parse_struct_decl();
    Result<StructField> parse_struct_field();
    DeclResult parse_enum_decl();
    Result<EnumField> parse_enum_field();
    DeclResult parse_const_decl();
    DeclResult parse_static_decl();
    DeclResult parse_func_decl();
    Result<std::vector<Param>> parse_func_params();
    Result<Param> parse_param_decl();
    Result<Block> parse_block(bool implicit_return = true);
    StmtResult parse_stmt();
    ExprResult prime_parse_expr(int precedence = 0);
    ExprResult parse_expr(int precedence = 0);
    std::unique_ptr<Expr> parse_num() const;
    ExprResult parse_for_expr();
    ExprResult parse_if_expr();
    ExprResult parse_loop_expr();
    ExprResult parse_while_expr();
    TypeResult prime_parse_type();
    TypeResult parse_type();

public:
    Parser(Lexer& lexer, SourceManager& sm) : lexer(lexer), diag(sm) {};
    std::vector<std::unique_ptr<Decl>> parse();
};