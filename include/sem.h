#pragma once

#include "ast.h"
#include "diagnostics.h"
#include "src_mgr.h"
#include "sym_table.h"
#include "type.h"

class SemChecker : public ASTVisitor {
    SymbolTable* syms;
    DiagnosticsEngine diag;

public:
    SemChecker(SymbolTable* syms, SourceManager* src)
        : syms(syms), diag(src) {};
    ~SemChecker() override = default;
    SemChecker(const SemChecker& other) = delete;
    SemChecker(SemChecker&& other) = delete;
    SemChecker& operator=(const SemChecker& other) = delete;
    SemChecker& operator=(SemChecker&& other) = delete;

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
    void visit(StructExprField& expr) override;
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

    void check_expr_assignable(Expr& expr);
};
