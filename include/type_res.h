#pragma once

#include "ast.h"
#include "diagnostics.h"
#include "inf_ctxt.h"
#include "src_mgr.h"
#include "sym_table.h"
#include "type.h"
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace z {

class TypeResolver : public ast::ASTVisitor {
    SymbolTable* syms;
    DiagnosticsEngine diag;
    std::optional<InferenceContext> infctxt;

public:
    TypeResolver(SymbolTable* syms, SourceManager* src)
        : syms(syms), diag(src), infctxt(std::in_place) {};
    ~TypeResolver() override = default;
    TypeResolver(const TypeResolver& other) = delete;
    TypeResolver(TypeResolver&& other) = delete;
    TypeResolver& operator=(const TypeResolver& other) = delete;
    TypeResolver& operator=(TypeResolver&& other) = delete;

    void fill_top_level_syms(
        const std::vector<std::unique_ptr<ast::Decl>>& decls) const;
    void visit(ast::Identifier& ident) override;
    void visit(ast::IntExpr& expr) override;
    void visit(ast::FloatExpr& expr) override;
    void visit(ast::BoolExpr& expr) override;
    void visit(ast::PrefixExpr& expr) override;
    void visit(ast::PostfixExpr& expr) override;
    void visit(ast::BinaryExpr& expr) override;
    void visit(ast::TernaryExpr& expr) override;
    void visit(ast::CallExpr& expr) override;
    void visit(ast::ArrayExpr& expr) override;
    void visit(ast::FieldExpr& expr) override;
    void visit(ast::ArrayInitExpr& expr) override;
    void visit(ast::StructExprField& expr) override;
    void visit(ast::StructInitExpr& expr) override;
    void visit(ast::TupleExpr& expr) override;
    void visit(ast::Block& block) override;
    void visit(ast::Param& param) override;
    void visit(ast::FuncDecl& func) override;
    void visit(ast::BreakStmt& stmt) override;
    void visit(ast::ContinueStmt& stmt) override;
    void visit(ast::ForExpr& expr) override;
    void visit(ast::LetStmt& stmt) override;
    void visit(ast::ReturnStmt& stmt) override;
    void visit(ast::IfExpr& expr) override;
    void visit(ast::ElseExpr& expr) override;
    void visit(ast::LoopExpr& expr) override;
    void visit(ast::WhileExpr& expr) override;
    void visit(ast::StringExpr& expr) override;
    void visit(ast::CharExpr& expr) override;
    void visit(ast::StructField& field) override;
    void visit(ast::StructDecl& decl) override;
    void visit(ast::EnumField& field) override;
    void visit(ast::EnumDecl& decl) override;
    void visit(ast::ConstDecl& decl) override;
    void visit(ast::StaticDecl& decl) override;
    void resolve(std::shared_ptr<type::Type>& type);
    void resolve(ast::ASTNode* node);
    void resolve_subtree(ast::ASTNode* node);
    void visit_method_call(ast::BinaryExpr& expr);
};

} // namespace z
