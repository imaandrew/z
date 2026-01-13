#pragma once

#include "ast.h"
#include "constraint.h"
#include "sym_table.h"
#include "type.h"
#include "type_arena.h"
#include "type_ref.h"
#include "zctxt.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace z::type {
class ConstraintGenerator : public ast::ASTVisitor {
    TypeArena* ty;
    std::vector<TypeRef>& inferred_types;
    std::vector<Constraint> constraints;
    SymbolTable* syms;
    std::uint32_t type_id = 0;
    std::vector<TypeRef> block_type_stack;
    std::vector<TypeRef> expected_type_stack;

    TypeRef new_type(InferType type) {
        auto v = ty->make<InferredType>(type_id++, type);
        inferred_types.emplace_back(v);
        return v;
    }

    TypeRef new_var() { return new_type(InferType::Var); }

    void emit(Constraint c) { constraints.push_back(c); }

    void eq(TypeRef lhs, TypeRef rhs) {
        emit(EqualityConstraint{.lhs = lhs, .rhs = rhs});
    }

    void push_block_type(TypeRef type) { block_type_stack.push_back(type); }

    void pop_block_type() { block_type_stack.pop_back(); }

    TypeRef peek_block_type() { return block_type_stack.back(); }

    void push_expected(TypeRef type) { expected_type_stack.push_back(type); }

    void pop_expected() { expected_type_stack.pop_back(); }

    std::optional<TypeRef> peek_expected() {
        return expected_type_stack.empty()
                   ? std::nullopt
                   : std::optional(expected_type_stack.back());
    }

public:
    explicit ConstraintGenerator(std::vector<TypeRef>& inferred_types,
                                 ZContext& ctxt)
        : ty(ctxt.ty.get()), inferred_types(inferred_types),
          syms(ctxt.syms.get()) {}

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
    void visit(ast::TraitDecl& decl) override;
    void visit(ast::TypeAliasDecl& decl) override;
    void visit(ast::TraitFuncDecl& decl) override;

    void resolve_type_name(TypeRef& type);
};
} // namespace z::type
