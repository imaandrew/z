#pragma once
#include "ast.h"
#include "core/result.h"
#include "core/string_pool.h"
#include "core/zctxt.h"
#include "diag/diagnostics.h"
#include "diag/src_mgr.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "sema/sym_table.h"
#include "type/type.h"
#include "type/type_arena.h"
#include "type/type_ref.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace z {

using StmtResult = Result<std::unique_ptr<ast::Stmt>>;
using ExprResult = Result<std::unique_ptr<ast::Expr>>;
using DeclResult = Result<std::unique_ptr<ast::Decl>>;
using TypeResult = Result<type::TypeRef>;

inline StmtResult StmtError() { return StmtResult(); }
inline ExprResult ExprError() { return ExprResult(); }
inline DeclResult DeclError() { return DeclResult(); }
inline TypeResult TypeError() { return TypeResult(); }

enum class SyncFlags : std::uint8_t {
    None = 0,
    StopAtSemi = 1U << 0U,
    BreakBefore = 1U << 1U,
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
    Lexer lexer;
    DiagnosticsEngine* diag;
    SourceManager* source;
    SymbolTable* syms;
    StringPool* strings;
    type::TypeArena* ty;
    Token tok{};
    Token prev_tok{};
    bool required_semi = true;

    void next_token();
    bool consume(TokenKind kind);
    bool kind(TokenKind kind);
    bool tok_assert(TokenKind kind);
    bool sync(TokenKind kind, SyncFlags flags = static_cast<SyncFlags>(0));
    bool sync(std::vector<TokenKind> const& kinds,
              SyncFlags flags = static_cast<SyncFlags>(0));
    void recover_decl();
    void recover_stmt();
    [[nodiscard]] bool can_be_expr() const;
    DeclResult parse_struct_decl();
    Result<std::unique_ptr<ast::StructField>> parse_struct_field();
    DeclResult parse_enum_decl();
    Result<std::unique_ptr<ast::EnumField>> parse_enum_field();
    DeclResult parse_const_decl();
    DeclResult parse_static_decl();
    DeclResult parse_trait_decl();
    DeclResult parse_type_alias_decl();
    DeclResult parse_trait_func_decl();
    DeclResult parse_func_decl();
    Result<std::vector<std::unique_ptr<ast::Param>>> parse_func_params();
    Result<std::unique_ptr<ast::Param>> parse_param_decl();
    Result<std::unique_ptr<ast::Block>>
    parse_block(bool implicit_return = true);
    StmtResult parse_stmt();
    StmtResult parse_let_stmt();
    ExprResult prime_parse_expr(int precedence = 0,
                                std::optional<TokenKind> ignore = std::nullopt);
    ExprResult parse_expr(int precedence = 0,
                          std::optional<TokenKind> ignore = std::nullopt);
    Result<std::unique_ptr<ast::StructExprField>> parse_struct_expr_field();
    std::unique_ptr<ast::Identifier> parse_ident_unchecked();
    [[nodiscard]] std::unique_ptr<ast::Expr> parse_num() const;
    ExprResult parse_for_expr();
    ExprResult parse_if_expr();
    ExprResult parse_loop_expr();
    ExprResult parse_while_expr();
    TypeResult prime_parse_type();
    TypeResult parse_type();

public:
    Parser(const Lexer& lexer, ZContext& ctxt)
        : lexer(lexer), diag(&ctxt.diag), source(ctxt.src.get()),
          syms(ctxt.syms.get()), strings(ctxt.strings.get()),
          ty(ctxt.ty.get()) {};
    std::unique_ptr<ast::SourceFileDecl> parse();
};
} // namespace z
