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
    void visit(ast::StructField& field) override;
    void visit(ast::StructDecl& decl) override;
    void visit(ast::EnumField& field) override;
    void visit(ast::EnumDecl& decl) override;
    void visit(ast::ConstDecl& decl) override;
    void visit(ast::StaticDecl& decl) override;
    void resolve(std::shared_ptr<type::Type>& type);
    void resolve(ast::ASTNode* node);
    void visit_method_call(ast::BinaryExpr& expr);
};

class ResolutionVisitor : public ast::ASTVisitor {
    TypeResolver& resolver;

    void resolve_node(ast::ASTNode& node) {
        if (node.has_type())
            resolver.resolve(node.node_type);
    }

public:
    explicit ResolutionVisitor(TypeResolver& r) : resolver(r) {};

    void visit(ast::IntExpr& expr) override { resolve_node(expr); }
    void visit(ast::FloatExpr& expr) override { resolve_node(expr); }
    void visit(ast::BoolExpr& expr) override { resolve_node(expr); }
    void visit(ast::StringExpr& expr) override { resolve_node(expr); }
    void visit(ast::Identifier& expr) override { resolve_node(expr); }

    void visit(ast::PrefixExpr& expr) override {
        resolve_node(expr);
        if (expr.expr)
            expr.expr->accept(*this);
    }

    void visit(ast::PostfixExpr& expr) override {
        resolve_node(expr);
        if (expr.expr)
            expr.expr->accept(*this);
    }

    void visit(ast::BinaryExpr& expr) override {
        resolve_node(expr);
        if (expr.lhs)
            expr.lhs->accept(*this);
        if (expr.rhs)
            expr.rhs->accept(*this);
    }

    void visit(ast::TernaryExpr& expr) override {
        resolve_node(expr);
        if (expr.lhs)
            expr.lhs->accept(*this);
        if (expr.mhs)
            expr.mhs->accept(*this);
        if (expr.rhs)
            expr.rhs->accept(*this);
    }

    void visit(ast::CallExpr& expr) override {
        resolve_node(expr);
        if (expr.ident)
            expr.ident->accept(*this);
        for (auto& arg : expr.args) {
            if (arg)
                arg->accept(*this);
        }
    }

    void visit(ast::ArrayExpr& expr) override {
        resolve_node(expr);
        if (expr.ident)
            expr.ident->accept(*this);
        if (expr.val)
            expr.val->accept(*this);
    }

    void visit(ast::FieldExpr& expr) override {
        resolve_node(expr);
        if (expr.container)
            expr.container->accept(*this);
        if (expr.field)
            expr.field->accept(*this);
    }

    void visit(ast::ArrayInitExpr& expr) override {
        resolve_node(expr);
        for (auto& val : expr.vals) {
            if (val)
                val->accept(*this);
        }
    }

    void visit(ast::StructExprField& expr) override {
        resolve_node(expr);
        if (expr.ident)
            expr.ident->accept(*this);
        if (expr.val)
            expr.val->accept(*this);
    }

    void visit(ast::StructInitExpr& expr) override {
        resolve_node(expr);
        if (expr.ident)
            expr.ident->accept(*this);
        for (auto& field : expr.fields) {
            if (field)
                field->accept(*this);
        }
    }

    void visit(ast::Block& block) override {
        for (auto& stmt : block.stmts) {
            if (stmt)
                stmt->accept(*this);
        }
        resolve_node(block);
    }

    void visit(ast::Param& param) override { resolve_node(param); }

    void visit(ast::FuncDecl& func) override {
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

    void visit(ast::BreakStmt& stmt) override {
        resolve_node(stmt);
        if (stmt.expr)
            stmt.expr->accept(*this);
    }

    void visit(ast::ContinueStmt& stmt) override { resolve_node(stmt); }

    void visit(ast::ForExpr& expr) override {
        resolve_node(expr);
        if (expr.ident)
            expr.ident->accept(*this);
        if (expr.expr)
            expr.expr->accept(*this);
        if (expr.block)
            expr.block->accept(*this);
    }

    void visit(ast::LetStmt& stmt) override {
        resolve_node(stmt);
        if (stmt.ident)
            stmt.ident->accept(*this);
        if (stmt.val)
            stmt.val->accept(*this);
    }

    void visit(ast::ReturnStmt& stmt) override {
        resolve_node(stmt);
        if (stmt.expr)
            stmt.expr->accept(*this);
    }

    void visit(ast::IfExpr& expr) override {
        resolve_node(expr);
        if (expr.expr)
            expr.expr->accept(*this);
        if (expr.block)
            expr.block->accept(*this);
        if (expr.else_expr)
            expr.else_expr->accept(*this);
    }

    void visit(ast::ElseExpr& expr) override {
        resolve_node(expr);
        if (expr.block)
            expr.block->accept(*this);
    }

    void visit(ast::LoopExpr& expr) override {
        resolve_node(expr);
        if (expr.expr)
            expr.expr->accept(*this);
        if (expr.block)
            expr.block->accept(*this);
    }

    void visit(ast::WhileExpr& expr) override {
        resolve_node(expr);
        if (expr.expr)
            expr.expr->accept(*this);
        if (expr.block)
            expr.block->accept(*this);
    }

    void visit(ast::StructField& field) override {
        resolve_node(field);
        if (field.ident)
            field.ident->accept(*this);
    }

    void visit(ast::StructDecl& /*decl*/) override {}

    void visit(ast::EnumField& /*field*/) override {}

    void visit(ast::EnumDecl& /*decl*/) override {}

    void visit(ast::ConstDecl& decl) override {
        if (decl.ident)
            decl.ident->accept(*this);
        if (decl.val)
            decl.val->accept(*this);
        resolve_node(decl);
    }

    void visit(ast::StaticDecl& decl) override {
        if (decl.ident)
            decl.ident->accept(*this);
        if (decl.val)
            decl.val->accept(*this);
        resolve_node(decl);
    }
};
} // namespace z
