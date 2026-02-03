#pragma once

#include "core/string_pool.h"
#include "core/zctxt.h"
#include "ir.h"
#include "parser/ast.h"
#include "type/type_ref.h"
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <unordered_map>
#include <vector>

namespace z::ir {

class IRBuilder final : ast::ASTVisitor {
    ZContext* ctxt;

    std::vector<Instruction> insts;
    std::vector<BasicBlock> blocks;
    std::vector<IRFunction> funcs;

    std::unordered_map<StringID, VReg> reg_map;
    std::unordered_map<std::uint32_t, StringID> str_map;

    std::uint32_t reg_id = 0;

    std::optional<Operand> last_result;

    void visit(ast::Identifier& ident) override;
    void visit(ast::IntExpr& expr) override;
    void visit(ast::FloatExpr& expr) override;
    void visit(ast::BoolExpr& expr) override;
    void visit(ast::PrefixExpr& expr) override;
    void visit(ast::PostfixExpr& expr) override;
    void visit(ast::BinaryExpr& expr) override;
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

    VReg emit_reg(type::TypeRef type) {
        return VReg{.id = reg_id++, .type = type};
    }

    VReg emit_reg(StringID name, type::TypeRef type) {
        auto reg = VReg{.id = reg_id++, .type = type};
        reg_map.insert_or_assign(name, reg);
        str_map.insert_or_assign(reg.id, name);
        return reg;
    }

    VReg get_var(StringID name, type::TypeRef type) {
        if (reg_map.contains(name))
            return reg_map.at(name);

        return emit_reg(name, type);
    }

    StringID get_str(VReg reg) { return str_map.at(reg.id); }

    VReg copy_reg(VReg reg) {
        auto name = get_str(reg);
        return emit_reg(name, reg.type);
    }

    void new_block() { blocks.emplace_back(); }

    Operand emit_op(ast::Expr* e) {
        e->accept(*this);
        return last_result.value();
    }

    void emit_inst(IROp op, VReg dest, std::initializer_list<Operand> ops,
                   type::TypeRef type) {
        insts.emplace_back(op, dest, ops, type);
    }

    std::optional<ConstInt> fold_int_op(ast::BinOp op, const ConstInt& lhs, const ConstInt& rhs);
};
} // namespace z::ir
