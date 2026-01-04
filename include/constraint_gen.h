#pragma once

#include "ast.h"
#include "constraint.h"
#include "sym_table.h"
#include "type.h"
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace z::type {
class ConstraintGenerator : public ast::ASTVisitor {
    std::vector<std::shared_ptr<InferredType>>& type_vars;
    std::vector<Constraint> constraints;
    SymbolTable* syms;
    std::uint32_t type_id = 0;
    std::vector<std::shared_ptr<type::Type>> block_type_stack;

    std::shared_ptr<Type> new_type(InferType type) {
        auto v = std::make_shared<InferredType>(type_id++, type);
        type_vars.emplace_back(v);
        return v;
    }

    std::shared_ptr<Type> new_var() { return new_type(InferType::Var); }

    void emit(Constraint c) { constraints.push_back(std::move(c)); }

    void eq(std::shared_ptr<Type> lhs, std::shared_ptr<Type> rhs) {
        emit(EqualityConstraint{.lhs = std::move(lhs), .rhs = std::move(rhs)});
    }

    void push_block_type(std::shared_ptr<type::Type> type) {
        block_type_stack.push_back(std::move(type));
    }

    void pop_block_type() { block_type_stack.pop_back(); }

    std::shared_ptr<type::Type>& peek_block_type() {
        return block_type_stack.back();
    }

public:
    explicit ConstraintGenerator(
        std::vector<std::shared_ptr<InferredType>>& type_vars,
        SymbolTable* syms)
        : type_vars(type_vars), syms(syms) {}

    std::vector<Constraint> collect(ast::Decl* root) {
        root->accept(*this);
        return constraints;
    }

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
    void visit(ast::SourceFileDecl& file) override;
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

    void resolve_type_name(std::shared_ptr<type::Type>& type);
};
} // namespace z::type
