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
    void resolve(std::shared_ptr<Type>& type);
    void resolve(ASTNode* node);
    void visit_method_call(BinaryExpr& expr);
};

class ResolutionVisitor : public ASTVisitor {
    TypeResolver& resolver;

    void resolve_node(ASTNode& node) {
        if (node.has_type())
            resolver.resolve(node.node_type);
    }

public:
    explicit ResolutionVisitor(TypeResolver& r) : resolver(r) {};

    void visit(IntExpr& expr) override { resolve_node(expr); }
    void visit(FloatExpr& expr) override { resolve_node(expr); }
    void visit(BoolExpr& expr) override { resolve_node(expr); }
    void visit(StringExpr& expr) override { resolve_node(expr); }
    void visit(Identifier& expr) override { resolve_node(expr); }

    void visit(PrefixExpr& expr) override {
        resolve_node(expr);
        if (expr.expr)
            expr.expr->accept(*this);
    }

    void visit(PostfixExpr& expr) override {
        resolve_node(expr);
        if (expr.expr)
            expr.expr->accept(*this);
    }

    void visit(BinaryExpr& expr) override {
        resolve_node(expr);
        if (expr.lhs)
            expr.lhs->accept(*this);
        if (expr.rhs)
            expr.rhs->accept(*this);
    }

    void visit(TernaryExpr& expr) override {
        resolve_node(expr);
        if (expr.lhs)
            expr.lhs->accept(*this);
        if (expr.mhs)
            expr.mhs->accept(*this);
        if (expr.rhs)
            expr.rhs->accept(*this);
    }

    void visit(CallExpr& expr) override {
        resolve_node(expr);
        if (expr.ident)
            expr.ident->accept(*this);
        for (auto& arg : expr.args) {
            if (arg)
                arg->accept(*this);
        }
    }

    void visit(ArrayExpr& expr) override {
        resolve_node(expr);
        if (expr.ident)
            expr.ident->accept(*this);
        if (expr.val)
            expr.val->accept(*this);
    }

    void visit(FieldExpr& expr) override {
        resolve_node(expr);
        if (expr.container)
            expr.container->accept(*this);
        if (expr.field)
            expr.field->accept(*this);
    }

    void visit(ArrayInitExpr& expr) override {
        resolve_node(expr);
        for (auto& val : expr.vals) {
            if (val)
                val->accept(*this);
        }
    }

    void visit(StructExprField& expr) override {
        resolve_node(expr);
        if (expr.ident)
            expr.ident->accept(*this);
        if (expr.val)
            expr.val->accept(*this);
    }

    void visit(StructInitExpr& expr) override {
        resolve_node(expr);
        if (expr.ident)
            expr.ident->accept(*this);
        for (auto& field : expr.fields) {
            if (field)
                field->accept(*this);
        }
    }

    void visit(Block& block) override {
        for (auto& stmt : block.stmts) {
            if (stmt)
                stmt->accept(*this);
        }
        resolve_node(block);
    }

    void visit(Param& param) override { resolve_node(param); }

    void visit(FuncDecl& func) override {
        resolve_node(func);
        if (func.name)
            func.name->accept(*this);
        if (func.impl_type && *func.impl_type)
            (*func.impl_type)->accept(*this);
        for (auto& param : func.params) {
            if (param)
                param->accept(*this);
        }
        if (func.body)
            func.body->accept(*this);
    }

    void visit(BreakStmt& stmt) override {
        resolve_node(stmt);
        if (stmt.expr)
            stmt.expr->accept(*this);
    }

    void visit(ContinueStmt& stmt) override { resolve_node(stmt); }

    void visit(ForExpr& expr) override {
        resolve_node(expr);
        if (expr.ident)
            expr.ident->accept(*this);
        if (expr.expr)
            expr.expr->accept(*this);
        if (expr.block)
            expr.block->accept(*this);
    }

    void visit(LetStmt& stmt) override {
        resolve_node(stmt);
        if (stmt.ident)
            stmt.ident->accept(*this);
        if (stmt.val)
            stmt.val->accept(*this);
    }

    void visit(ReturnStmt& stmt) override {
        resolve_node(stmt);
        if (stmt.expr)
            stmt.expr->accept(*this);
    }

    void visit(IfExpr& expr) override {
        resolve_node(expr);
        if (expr.expr)
            expr.expr->accept(*this);
        if (expr.block)
            expr.block->accept(*this);
        if (expr.else_expr)
            expr.else_expr->accept(*this);
    }

    void visit(ElseExpr& expr) override {
        resolve_node(expr);
        if (expr.block)
            expr.block->accept(*this);
    }

    void visit(LoopExpr& expr) override {
        resolve_node(expr);
        if (expr.expr)
            expr.expr->accept(*this);
        if (expr.block)
            expr.block->accept(*this);
    }

    void visit(WhileExpr& expr) override {
        resolve_node(expr);
        if (expr.expr)
            expr.expr->accept(*this);
        if (expr.block)
            expr.block->accept(*this);
    }

    void visit(StructField& field) override {
        resolve_node(field);
        if (field.ident)
            field.ident->accept(*this);
    }

    void visit(StructDecl& /*decl*/) override {}

    void visit(EnumField& /*field*/) override {}

    void visit(EnumDecl& /*decl*/) override {}

    void visit(ConstDecl& decl) override {
        if (decl.ident)
            decl.ident->accept(*this);
        if (decl.val)
            decl.val->accept(*this);
        resolve_node(decl);
    }

    void visit(StaticDecl& decl) override {
        if (decl.ident)
            decl.ident->accept(*this);
        if (decl.val)
            decl.val->accept(*this);
        resolve_node(decl);
    }
};
