#include "const_eval.h"
#include "core/panic.h"
#include "diag/diagnostics.h"
#include "ir/constants.h"
#include "parser/ast.h"
#include "parser/const_helpers.h"
#include "type/type.h"
#include "type/type_ref.h"
#include <memory>
#include <optional>

namespace z {

std::optional<ConstValue> ConstantEvaluator::eval(const ast::Expr& expr,
                                                  type::TypeRef expected) {
    if (const auto* int_expr = ast::dyn_cast<ast::IntExpr>(&expr)) {
        const auto* int_type = ty->get_as<type::IntegerType>(expected);
        if (!int_type) {
            diag->error(expr.get_span(), DiagnosticKind::ExpectedType,
                        ty->get(expected)->basic_name(ctxt));
            return std::nullopt;
        }

        const auto v = ir::ConstInt(int_expr->val, 64, false);
        if (!v.fits_in(int_type->get_width(), int_type->is_signed())) {
            diag->error(expr.get_span(), DiagnosticKind::NumericLiteralTooBig,
                        int_type->basic_name(ctxt));
            return std::nullopt;
        }
        return ConstValue(
            v.cast_to(int_type->get_width(), int_type->is_signed()));
    }

    if (const auto* pre_expr = ast::dyn_cast<ast::PrefixExpr>(&expr)) {
        auto val = std::optional<ConstValue>();

        const auto* type = ty->get(expected);
        const auto* lit = ast::dyn_cast<ast::IntExpr>(pre_expr->expr.get());
        if (pre_expr->op == ast::UnOp::Neg && lit) {
            const auto* int_type = ty->get_as<type::IntegerType>(expected);
            if (int_type->is_signed() &&
                lit->val <= (1U << (int_type->get_width() - 1U))) {
                return ConstValue(
                    ir::ConstInt(-lit->val, int_type->get_width(), true));
            }
        }
        val = eval(*pre_expr->expr, expected);
        if (!val)
            return std::nullopt;

        auto* int_val = val->holds_int();
        bool overflow = false;
        bool undefined = false;
        bool handled = false;

        switch (pre_expr->op) {
        case ast::UnOp::BitNot:
            if (int_val) {
                *int_val = int_val->bit_not();
                handled = true;
            }
            break;
        case ast::UnOp::Neg:
            if (type->is_integral() && int_val) {
                *int_val = int_val->neg(overflow, undefined);
                handled = true;
            }
            break;
        case ast::UnOp::LogicNot:
        case ast::UnOp::Inc:
        case ast::UnOp::Dec:
            diag->error(expr.get_span(), DiagnosticKind::NotCompileTimeConst);
            return std::nullopt;
        }

        if (overflow) {
            diag->error(expr.get_span(), DiagnosticKind::OperationOverflows,
                        type->basic_name(ctxt));
            return std::nullopt;
        }

        if (undefined) {
            diag->error(expr.get_span(), DiagnosticKind::OperationUndefined);
            return std::nullopt;
        }

        if (handled)
            return val;

        diag->error(expr.get_span(), DiagnosticKind::NotCompileTimeConst);
        return std::nullopt;
    }

    if (const auto* post_expr = ast::dyn_cast<ast::PostfixExpr>(&expr)) {
        auto val = eval(*post_expr->expr, expected);
        if (!val)
            return std::nullopt;

        switch (post_expr->op) {
        case ast::UnOp::Inc:
        case ast::UnOp::Dec:
            diag->error(expr.get_span(), DiagnosticKind::NotCompileTimeConst);
            return std::nullopt;
        case ast::UnOp::Neg:
        case ast::UnOp::BitNot:
        case ast::UnOp::LogicNot:
            panic("Invalid postfix operator");
        }

        return val;
    }

    if (const auto* bin_expr = ast::dyn_cast<ast::BinaryExpr>(&expr)) {
        auto lhs = eval(*bin_expr->lhs, expected);
        if (!lhs)
            return std::nullopt;
        auto rhs = eval(*bin_expr->rhs, expected);
        if (!rhs)
            return std::nullopt;

        bool overflow = false;
        bool undefined = false;

        const auto* lhs_int = lhs->holds_int();
        const auto* rhs_int = rhs->holds_int();

        auto val = std::optional<ConstValue>();

        switch (bin_expr->op) {
        case ast::BinOp::Add:
            if (lhs_int && rhs_int) {
                val.emplace<ir::ConstInt>(lhs_int->add(*rhs_int, overflow));
            }
            break;
        case ast::BinOp::Sub:
            if (lhs_int && rhs_int) {
                val.emplace<ir::ConstInt>(lhs_int->sub(*rhs_int, overflow));
            }
            break;
        case ast::BinOp::Mul:
            if (lhs_int && rhs_int) {
                val.emplace<ir::ConstInt>(lhs_int->mul(*rhs_int, overflow));
            }
            break;
        case ast::BinOp::Div:
            if (lhs_int && rhs_int) {
                if (lhs_int->is_signed()) {
                    val.emplace<ir::ConstInt>(
                        lhs_int->sdiv(*rhs_int, overflow, undefined));
                } else {
                    val.emplace<ir::ConstInt>(
                        lhs_int->udiv(*rhs_int, undefined));
                }
            }
            break;
        case ast::BinOp::Mod:
            if (lhs_int && rhs_int) {
                if (lhs_int->is_signed()) {
                    val.emplace<ir::ConstInt>(
                        lhs_int->srem(*rhs_int, undefined));
                } else {
                    val.emplace<ir::ConstInt>(
                        lhs_int->urem(*rhs_int, undefined));
                }
            }
            break;
        case ast::BinOp::BitXor:
            if (lhs_int && rhs_int) {
                val.emplace<ir::ConstInt>(lhs_int->bit_xor(*rhs_int));
            }
            break;
        case ast::BinOp::BitAnd:
            if (lhs_int && rhs_int) {
                val.emplace<ir::ConstInt>(lhs_int->bit_and(*rhs_int));
            }
            break;
        case ast::BinOp::BitOr:
            if (lhs_int && rhs_int) {
                val.emplace<ir::ConstInt>(lhs_int->bit_or(*rhs_int));
            }
            break;
        case ast::BinOp::Shl:
            if (lhs_int && rhs_int) {
                val.emplace<ir::ConstInt>(lhs_int->shl(*rhs_int, undefined));
            }
            break;
        case ast::BinOp::Shr:
            if (lhs_int && rhs_int) {
                if (lhs_int->is_signed()) {
                    val.emplace<ir::ConstInt>(
                        lhs_int->ashr(*rhs_int, undefined));
                } else {
                    val.emplace<ir::ConstInt>(
                        lhs_int->lshr(*rhs_int, undefined));
                }
            }
            break;
        case ast::BinOp::LogicAnd:
        case ast::BinOp::LogicOr:
        case ast::BinOp::EqEq:
        case ast::BinOp::Ne:
        case ast::BinOp::Gt:
        case ast::BinOp::Lt:
        case ast::BinOp::Ge:
        case ast::BinOp::Le:
        case ast::BinOp::Range:
        case ast::BinOp::RangeEq:
        case ast::BinOp::AddEq:
        case ast::BinOp::SubEq:
        case ast::BinOp::MulEq:
        case ast::BinOp::DivEq:
        case ast::BinOp::ModEq:
        case ast::BinOp::BitXorEq:
        case ast::BinOp::BitAndEq:
        case ast::BinOp::BitOrEq:
        case ast::BinOp::ShlEq:
        case ast::BinOp::ShrEq:
        case ast::BinOp::Eq:
        case ast::BinOp::ColonColon:
            diag->error(expr.get_span(), DiagnosticKind::NotCompileTimeConst);
            return std::nullopt;
        }

        if (overflow) {
            diag->error(expr.get_span(), DiagnosticKind::OperationOverflows,
                        ty->get(expected)->basic_name(ctxt));
            return std::nullopt;
        }

        if (undefined) {
            diag->error(expr.get_span(), DiagnosticKind::OperationUndefined);
            return std::nullopt;
        }

        if (!val)
            diag->error(expr.get_span(), DiagnosticKind::NotCompileTimeConst);

        return val;
    }

    if (const auto* ident = ast::dyn_cast<ast::Identifier>(&expr)) {
        const auto it = ctxt->const_decls.find(ident->get_id());
        if (it == ctxt->const_decls.end()) {
            diag->error(expr.get_span(), DiagnosticKind::NotCompileTimeConst);
            return std::nullopt;
        }

        auto v = evaluate(it->second);
        if (!v)
            return std::nullopt;

        if (it->second.decl->type != expected) {
            diag->error(expr.get_span(), DiagnosticKind::TypeMismatch,
                        ty->get(expected)->basic_name(ctxt),
                        ty->get(it->second.decl->type)->basic_name(ctxt));
            return std::nullopt;
        }

        return v;
    }

    diag->error(expr.get_span(), DiagnosticKind::NotCompileTimeConst);
    return std::nullopt;
}

std::optional<ConstValue> ConstantEvaluator::evaluate(ConstRecord& rec) {
    switch (rec.state) {
    case State::Done:
        return rec.value;
    case State::Error:
        return std::nullopt;
    case State::InProgress:
        diag->error(rec.decl->ident->get_span(),
                    DiagnosticKind::RecursiveConst);
        rec.state = State::Error;
        return std::nullopt;
    case State::Unevaluated:
        break;
    }

    rec.state = State::InProgress;
    ctxt->resolve_unk_type(rec.decl->type);
    rec.value = eval(*rec.decl->val, rec.decl->type);
    rec.state = rec.value ? State::Done : State::Error;
    return rec.value;
}
} // namespace z
