#include "ir_builder.h"
#include "core/panic.h"
#include "diag/diagnostics.h"
#include "ir/ir.h"
#include "parser/ast.h"
#include "type/type.h"
#include "type/type_arena.h"
#include <cassert>
#include <magic_enum/magic_enum_format.hpp>

namespace z::ir {

using ast::BinOp;
using ast::UnOp;

namespace {
std::optional<ConstInt> fold_int_op(IROp op, const ConstInt& lhs,
                                    const ConstInt& rhs, bool& overflow) {
    auto with_overflow = [&overflow](auto fn) -> std::optional<ConstInt> {
        auto res = fn();
        return overflow ? std::nullopt : std::optional(res);
    };

    switch (op) {
    case IROp::IAdd:
        return with_overflow([&]() { return lhs.add(rhs, overflow); });
    case IROp::ISub:
        return with_overflow([&]() { return lhs.sub(rhs, overflow); });
    case IROp::IMul:
        return with_overflow([&]() { return lhs.mul(rhs, overflow); });
    case IROp::SDiv:
        return with_overflow([&]() { return lhs.sdiv(rhs, overflow); });
    case IROp::UDiv:
        return lhs.udiv(rhs);
    case IROp::Shl:
        return lhs.shl(rhs);
    case IROp::Lsr:
        return lhs.lshr(rhs);
    case IROp::Asr:
        return lhs.ashr(rhs);
    default:
        return std::nullopt;
    }
}

std::optional<ConstFloat> fold_float_op(IROp op, const ConstFloat& lhs,
                                        const ConstFloat& rhs) {
    switch (op) {
    case IROp::FAdd:
        return lhs.add(rhs);
    case IROp::FSub:
        return lhs.sub(rhs);
    case IROp::FMul:
        return lhs.mul(rhs);
    case IROp::FDiv:
        return lhs.div(rhs);
    default:
        return std::nullopt;
    }
}

std::optional<bool> fold_bool_op(BinOp op, bool lhs, bool rhs) {
    switch (op) {
    case BinOp::Eq:
        return lhs == rhs;
    case BinOp::Ne:
        return lhs != rhs;
    case BinOp::BitAnd:
        return lhs && rhs;
    case BinOp::BitOr:
        return lhs || rhs;
    case BinOp::BitXor:
        return lhs ^ rhs;
    default:
        return std::nullopt;
    }
}
} // namespace

void IRBuilder::visit(ast::Identifier& ident) {
    last_result = Operand::reg(read_var(ident.get_id()));
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

    const auto expr_op = expr.op;

    switch (expr_op) {
    case UnOp::Inc:
    case UnOp::Dec: {
        const auto* int_type = ctxt->ty->get_as<type::IntegerType>(reg.type);

        auto dest =
            emit_inst(get_int_ir_op(expr_op),
                      {var, Operand::imm(ConstInt(1, int_type->get_width(),
                                                  int_type->is_signed()),
                                         reg.type)},
                      reg.type);

        write_var(ast::cast<ast::Identifier>(expr.expr.get())->get_id(), dest);
        last_result = Operand::reg(dest);
        break;
    }
    case UnOp::LogicNot:
    case UnOp::BitNot: {
        auto dest = emit_inst(get_ir_op(expr_op), {var}, reg.type);
        last_result = Operand::reg(dest);
        break;
    }
    case UnOp::Neg: {
        const auto* type = ctxt->ty->get(reg.type);
        if (type->is_integral()) {
            auto dest = emit_inst(IROp::INeg, {var}, reg.type);
            last_result = Operand::reg(dest);
        } else if (type->is_float()) {
            auto dest = emit_inst(IROp::FNeg, {var}, reg.type);
            last_result = Operand::reg(dest);
        } else
            panic("Invalid type for negation operator");

        break;
    }
    default:
        std::unreachable();
    }
}

void IRBuilder::visit(ast::PostfixExpr& expr) {
    const auto var = emit_op(expr.expr.get());
    const auto reg = var.as_reg();

    const auto* int_type = ctxt->ty->get_as<type::IntegerType>(reg.type);

    auto dest = emit_inst(get_int_ir_op(expr.op),
                          {var, Operand::imm(ConstInt(1, int_type->get_width(),
                                                      int_type->is_signed()),
                                             reg.type)},
                          reg.type);
    write_var(ast::cast<ast::Identifier>(expr.expr.get())->get_id(), dest);
    last_result = var;
}

void IRBuilder::visit(ast::BinaryExpr& expr) {
    const auto lhs = emit_op(expr.lhs.get());
    const auto rhs = emit_op(expr.rhs.get());
    const auto expr_op = expr.op;

    std::optional<IntCC> int_cc{};
    std::optional<FloatCC> float_cc{};

    auto op = [&] {
        const auto* type = ctxt->ty->get(expr.node_type);
        if (type->is_integral()) {
            const auto* int_type = type::cast<type::IntegerType>(type);
            auto op = get_int_ir_op(expr_op, int_type->is_signed());
            if (op == IROp::ICmp)
                int_cc = get_int_cc(expr_op, int_type->is_signed());

            return op;
        }

        if (type->is_float()) {
            auto op = get_float_ir_op(expr_op);
            if (op == IROp::FCmp)
                float_cc = get_float_cc(expr_op);
            return op;
        }

        return get_ir_op(expr_op);
    }();

    if (lhs.is_imm() && rhs.is_imm()) {
        auto lhs_imm = lhs.as_imm();
        auto rhs_imm = rhs.as_imm();

        if (lhs_imm.is_int() && rhs_imm.is_int()) {
            if (op == IROp::ICmp) {
                auto res = lhs_imm.as_int().cmp(rhs_imm.as_int(), *int_cc);
                last_result = Operand::imm(res, type::TypeArena::BOOL);
                return;
            }

            bool overflow = false;
            auto res =
                fold_int_op(op, lhs_imm.as_int(), rhs_imm.as_int(), overflow);

            if (overflow) {
                ctxt->diag.emit(
                    expr.get_span(), DiagnosticKind::OperationOverflows,
                    ctxt->ty->get(expr.node_type)->basic_name(ctxt));
                // TODO: handle error
                return;
            }

            if (res) {
                last_result = Operand::imm(*res, expr.node_type);
                return;
            }
        } else if (lhs_imm.is_float() && rhs_imm.is_float()) {
            if (op == IROp::FCmp) {
                auto res =
                    lhs_imm.as_float().cmp(rhs_imm.as_float(), *float_cc);
                last_result = Operand::imm(res, type::TypeArena::BOOL);
                return;
            }

            auto res =
                fold_float_op(op, lhs_imm.as_float(), rhs_imm.as_float());
            if (res) {
                last_result = Operand::imm(*res, expr.node_type);
                return;
            }
        } else if (lhs_imm.is_bool() && rhs_imm.is_bool()) {
            auto res =
                fold_bool_op(expr_op, lhs_imm.as_bool(), rhs_imm.as_bool());
            if (res) {
                last_result = Operand::imm(*res, type::TypeArena::BOOL);
                return;
            }
        }
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
        auto dest = emit_inst(op, {lhs, rhs}, expr.lhs->node_type);
        write_var(ast::cast<ast::Identifier>(expr.lhs.get())->get_id(), dest);
        last_result = std::nullopt;
        break;
    }

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
        auto dest = emit_inst(op, {lhs, rhs}, expr.node_type);
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
    auto dest = emit_inst(IROp::TupleInit, {first, second}, expr.node_type);
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
        // params.push_back(emit_reg(param->name->get_id(), param->type));
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

} // namespace z::ir
