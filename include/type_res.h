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

class TypeResolver : public ASTVisitor {
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

    void
    fill_top_level_syms(const std::vector<std::unique_ptr<Decl>>& decls) const;
    void visit(Identifier& ident) override;
    void visit(IntExpr& expr) override;
    void visit(FloatExpr& expr) override;
    void visit(BoolExpr& expr) override;
    void visit(PrefixExpr& expr) override;
    void visit(PostfixExpr& expr) override;
    void visit(BinaryExpr& expr) override;
    void visit(TernaryExpr& expr) override;
    void visit(CallExpr& expr) override;
    void visit(ArrayExpr& expr) override;
    void visit(FieldExpr& expr) override;
    void visit(ArrayInitExpr& expr) override;
    void visit(StructInitExpr& expr) override;
    void visit(Block& block) override;
    void visit(Param& param) override;
    void visit(FuncDecl& func) override;
    void visit(BreakStmt& stmt) override;
    void visit(ContinueStmt& stmt) override;
    void visit(ForExpr& expr) override;
    void visit(LetStmt& stmt) override;
    void visit(ReturnStmt& stmt) override;
    void visit(IfExpr& expr) override;
    void visit(ElseExpr& expr) override;
    void visit(LoopExpr& expr) override;
    void visit(WhileExpr& expr) override;
    void visit(StringExpr& expr) override;
    void visit(StructField& field) override;
    void visit(StructDecl& decl) override;
    void visit(EnumField& field) override;
    void visit(EnumDecl& decl) override;
    void visit(ConstDecl& decl) override;
    void visit(StaticDecl& decl) override;
    void resolve(std::shared_ptr<Type>& type);
    void resolve(ASTNode* node);
    void visit_method_call(BinaryExpr& expr);
};
