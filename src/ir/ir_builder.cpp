#include "ir_builder.h"
#include "core/panic.h"
#include "ir/ir.h"
#include "parser/ast.h"
#include "type/type.h"
#include "type/type_arena.h"
#include <cassert>
#include <magic_enum/magic_enum_format.hpp>

namespace z::ir {

using ast::BinOp;
using ast::UnOp;

void IRBuilder::visit(ast::Identifier& ident) {
    last_result = Operand::reg(get_var(ident.get_id(), ident.node_type));
}

void IRBuilder::visit(ast::IntExpr& expr) {
    const auto* ty = ctxt->ty->get_as<type::IntegerType>(expr.node_type);
    last_result = Operand::imm(
        ConstInt(expr.val, ty->get_width(), ty->is_signed()), expr.node_type);
}

void IRBuilder::visit(ast::FloatExpr& expr) {
    const auto* ty = ctxt->ty->get_as<type::FloatType>(expr.node_type);
    last_result =
        Operand::imm(ConstFloat(expr.val, ty->get_width()), expr.node_type);
}

void IRBuilder::visit(ast::BoolExpr& expr) {
    last_result = Operand::imm(expr.val, type::TypeArena::BOOL);
}

void IRBuilder::visit(ast::PrefixExpr& expr) {
    const auto var = emit_op(expr.expr.get());
    const auto reg = var.as_reg();
    const auto dest = copy_reg(reg);

    const auto expr_op = expr.op;

    switch (expr_op) {
    case UnOp::Inc:
    case UnOp::Dec: {
        const auto* int_type = ctxt->ty->get_as<type::IntegerType>(reg.type);

        emit_inst(get_int_ir_op(expr_op), dest,
                  {var, Operand::imm(ConstInt(1, int_type->get_width(),
                                              int_type->is_signed()),
                                     reg.type)},
                  reg.type);

        last_result = Operand::reg(dest);
        break;
    }
    case UnOp::LogicNot:
    case UnOp::BitNot: {
        emit_inst(get_ir_op(expr_op), dest, {var}, reg.type);

        last_result = Operand::reg(dest);
        break;
    }
    case UnOp::Neg: {
        const auto* type = ctxt->ty->get(reg.type);
        if (type->is_integral())
            emit_inst(IROp::INeg, dest, {var}, reg.type);
        else if (type->is_float())
            emit_inst(IROp::FNeg, dest, {var}, reg.type);
        else
            panic("Invalid type for negation operator");

        last_result = Operand::reg(dest);
        break;
    }
    default:
        std::unreachable();
    }
}

void IRBuilder::visit(ast::PostfixExpr& expr) {
    const auto var = emit_op(expr.expr.get());
    const auto reg = var.as_reg();
    const auto dest = copy_reg(reg);

    const auto* int_type = ctxt->ty->get_as<type::IntegerType>(reg.type);

    emit_inst(get_int_ir_op(expr.op), dest,
              {var, Operand::imm(ConstInt(1, int_type->get_width(),
                                          int_type->is_signed()),
                                 reg.type)},
              reg.type);

    last_result = var;
}

void IRBuilder::visit(ast::BinaryExpr& expr) {
    const auto lhs = emit_op(expr.lhs.get());
    const auto rhs = emit_op(expr.rhs.get());
    const auto expr_op = expr.op;

    auto op = [&] {
        const auto* type = ctxt->ty->get(expr.node_type);
        if (type->is_integral()) {
            const auto* int_type = type::cast<type::IntegerType>(type);
            switch (expr_op) {
            case BinOp::DivEq:
                return int_type->is_signed() ? IROp::SDiv : IROp::UDiv;
            case BinOp::ShrEq:
                return int_type->is_signed() ? IROp::Asr : IROp::Lsr;
            default:
                return get_int_ir_op(expr_op);
            }
        }

        if (type->is_float())
            return get_float_ir_op(expr_op);

        return get_ir_op(expr_op);
    }();

    if (lhs.is_imm() && rhs.is_imm()) {
    }

    switch (expr_op) {
    case BinOp::AddEq:
    case BinOp::SubEq:
    case BinOp::MulEq:
    case BinOp::DivEq:
    case BinOp::ModEq:
    case BinOp::BitXorEq:
    case BinOp::BitAndEq:
    case BinOp::BitOrEq:
    case BinOp::ShlEq:
    case BinOp::ShrEq: {
        const auto dest = copy_reg(lhs.as_reg());

        emit_inst(op, dest, {lhs, rhs}, expr.lhs->node_type);
        last_result = std::nullopt;
        break;
    }

    case BinOp::LogicOr:
    case BinOp::LogicAnd:
    case BinOp::EqEq:
    case BinOp::Ne:
    case BinOp::BitOr:
    case BinOp::BitXor:
    case BinOp::BitAnd:
    case BinOp::Shl:
    case BinOp::Shr:
    case BinOp::Mod:
    case BinOp::Add:
    case BinOp::Sub:
    case BinOp::Mul:
    case BinOp::Div:
    case BinOp::Gt:
    case BinOp::Lt:
    case BinOp::Ge:
    case BinOp::Le: {
        if (lhs.is_imm() && rhs.is_imm()) {
        }
        const auto dest = emit_reg(expr.node_type);
        emit_inst(get_ir_op(expr_op), dest, {lhs, rhs}, expr.node_type);
        last_result = Operand::reg(dest);
        break;
    }

    case BinOp::Range:
    case BinOp::RangeEq:
    case BinOp::ColonColon:
        assert(false && "TODO");
    }
}

void IRBuilder::visit(ast::CallExpr& expr) {}

void IRBuilder::visit(ast::ArrayExpr& expr) {}

void IRBuilder::visit(ast::FieldExpr& expr) {}

void IRBuilder::visit(ast::ArrayInitExpr& expr) {}

void IRBuilder::visit(ast::StructExprField& expr) {}

void IRBuilder::visit(ast::StructInitExpr& expr) {}

void IRBuilder::visit(ast::TupleExpr& expr) {
    const auto first = emit_op(expr.first.get());
    const auto second = emit_op(expr.second.get());
    const auto dest = emit_reg(expr.node_type);
    emit_inst(IROp::TupleInit, dest, {first, second}, expr.node_type);
    last_result = Operand::reg(dest);
}

void IRBuilder::visit(ast::Block& block) {
    new_block();
    for (const auto& stmts : block.stmts) {
        stmts->accept(*this);
    }
}

void IRBuilder::visit(ast::Param& param) {
    last_result = emit_op(param.name.get());
}

void IRBuilder::visit(ast::SourceFileDecl& file) {
    for (const auto& decl : file.decls) {
        decl->accept(*this);
    }
}

void IRBuilder::visit(ast::FuncDecl& func) {
    auto params = std::vector<VReg>();

    for (const auto& param : func.params) {
        params.push_back(emit_reg(param->name->get_id(), param->type));
    }

    func.body->accept(*this);
}

void IRBuilder::visit(ast::BreakStmt& stmt) {}

void IRBuilder::visit(ast::ContinueStmt& stmt) {}

void IRBuilder::visit(ast::ForExpr& expr) {}

void IRBuilder::visit(ast::LetStmt& stmt) {}

void IRBuilder::visit(ast::ReturnStmt& stmt) {}

void IRBuilder::visit(ast::IfExpr& expr) {}

void IRBuilder::visit(ast::ElseExpr& expr) {}

void IRBuilder::visit(ast::LoopExpr& expr) {}

void IRBuilder::visit(ast::WhileExpr& expr) {}

void IRBuilder::visit(ast::StringExpr& expr) {}

void IRBuilder::visit(ast::CharExpr& expr) {}

void IRBuilder::visit(ast::StructField& field) {}

void IRBuilder::visit(ast::StructDecl& decl) {}

void IRBuilder::visit(ast::EnumField& field) {}

void IRBuilder::visit(ast::EnumDecl& decl) {}

void IRBuilder::visit(ast::ConstDecl& decl) {}

void IRBuilder::visit(ast::StaticDecl& decl) {}

void IRBuilder::visit(ast::TraitDecl& decl) {}

void IRBuilder::visit(ast::TypeAliasDecl& decl) {}

void IRBuilder::visit(ast::TraitFuncDecl& decl) {}

std::optional<ConstInt> fold_int_op(IROp op, const ConstInt& lhs,
                                    const ConstInt& rhs) {
    switch (op) {
        case IROp::IAdd: {
            bool overflow = false;
            auto res = lhs.add(rhs, overflow);
            if (overflow) {
                return std::nullopt;
            }
            return res;
        }
    }
}

} // namespace z::ir
