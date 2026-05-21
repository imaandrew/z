#include "sem.h"
#include "core/panic.h"
#include "core/string_pool.h"
#include "core/types.h"
#include "diag/diagnostics.h"
#include "parser/ast.h"
#include "type/type.h"
#include "type/type_arena.h"
#include "type/type_ref.h"
#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_set>
#include <utility>

namespace z {

namespace {
constexpr bool is_assignment_op(ast::BinOp op) {
    switch (op) {
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
        return true;
    default:
        return false;
    }
}

constexpr bool is_division_op(ast::BinOp op) {
    switch (op) {
    case ast::BinOp::Div:
    case ast::BinOp::DivEq:
    case ast::BinOp::Mod:
    case ast::BinOp::ModEq:
        return true;
    default:
        return false;
    }
}
} // namespace

void SemChecker::visit(ast::Identifier& ident) {
    if (!ident.has_type()) {
        ctxt->diag.error(ident.get_span(), DiagnosticKind::UndefinedIdentifier,
                         ctxt->strings->get_string(ident.get_id()));

        return;
    }

    if (!ctxt->syms->is_var_initialized(ident.get_id())) {
        ctxt->diag.error(ident.get_span(), DiagnosticKind::UninitializedVar,
                         ctxt->strings->get_string(ident.get_id()));
    }
}

void SemChecker::visit(ast::IntExpr& expr) {
    const auto* type = ctxt->ty->get_as<type::IntegerType>(expr.get_type());
    expect(type != nullptr, "IntExpr should have IntegerType");

    u64 max = 0;
    switch (auto width = type->get_width()) {
    case 8:
        max = type->is_signed() ? INT8_MAX : UINT8_MAX;
        break;
    case 16:
        max = type->is_signed() ? INT16_MAX : UINT16_MAX;
        break;
    case 32:
        max = type->is_signed() ? INT32_MAX : UINT32_MAX;
        break;
    case 64:
        max = type->is_signed() ? INT64_MAX : UINT64_MAX;
        break;
    default:
        panic("Unexpected IntExpr width: {}", width);
    }

    if (expr.val > max) {
        ctxt->diag.error(expr.get_span(), DiagnosticKind::NumericLiteralTooBig,
                         type->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::FloatExpr& expr) {
    const auto* type = ctxt->ty->get_as<type::FloatType>(expr.get_type());
    expect(type != nullptr, "FloatExpr should have FloatType");

    double max = 0;
    switch (auto width = type->get_width()) {
    case 32:
        max = FLT_MAX;
        break;
    case 64:
        max = DBL_MAX;
        break;
    default:
        panic("Unexpected FloatExpr width: {}", width);
    }

    if (expr.val > max) {
        ctxt->diag.error(expr.get_span(), DiagnosticKind::NumericLiteralTooBig,
                         type->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::BoolExpr& /*expr*/) {}

void SemChecker::visit(ast::PrefixExpr& expr) {
    expr.expr->accept(*this);
    if (!expr.expr->has_type())
        return;

    const auto* type = ctxt->ty->get(expr.expr->get_type());
    using ast::UnOp;
    switch (expr.op) {
    case UnOp::Inc:
    case UnOp::Dec:
        check_expr_assignable(*expr.expr);

        if (!type->is_integral()) {
            ctxt->diag
                .error(expr.get_span(), DiagnosticKind::InvalidUnaryOperand,
                       expr.op, type->basic_name(ctxt))
                .add_primary_note("must have an integral type");
        }
        return;
    case UnOp::LogicNot:
        if (!type->is_logical()) {
            ctxt->diag
                .error(expr.get_span(), DiagnosticKind::InvalidUnaryOperand,
                       expr.op, type->basic_name(ctxt))
                .add_primary_note("must have a boolean type");
        }
        return;
    case UnOp::BitNot:
        if (!type->is_integral()) {
            ctxt->diag
                .error(expr.get_span(), DiagnosticKind::InvalidUnaryOperand,
                       expr.op, type->basic_name(ctxt))
                .add_primary_note("must have an integral type");
        }
        return;
    case UnOp::Neg:
        if (!type->is_numeric()) {
            ctxt->diag
                .error(expr.get_span(), DiagnosticKind::InvalidUnaryOperand,
                       expr.op, type->basic_name(ctxt))
                .add_primary_note("must have a numeric type");
        }
        return;
    }

    std::unreachable();
}

void SemChecker::visit(ast::PostfixExpr& expr) {
    expr.expr->accept(*this);
    if (!expr.expr->has_type())
        return;

    using ast::UnOp;
    switch (expr.op) {
    case UnOp::Inc:
    case UnOp::Dec:
        check_expr_assignable(*expr.expr);

        if (!ctxt->ty->get(expr.expr->get_type())->is_integral()) {
            ctxt->diag
                .error(expr.get_span(), DiagnosticKind::InvalidUnaryOperand,
                       expr.op,
                       ctxt->ty->get(expr.expr->get_type())->basic_name(ctxt))
                .add_primary_note("must have an integral type");
        }
        return;
    case ast::UnOp::Neg:
    case ast::UnOp::BitNot:
    case ast::UnOp::LogicNot:
        panic("Invalid postfix operator");
    }

    std::unreachable();
}

void SemChecker::visit(ast::BinaryExpr& expr) {
    expr.lhs->accept(*this);
    expr.rhs->accept(*this);

    if (!expr.lhs->has_type() || !expr.rhs->has_type())
        return;

    auto* l_type = ctxt->ty->get(expr.lhs->get_type());
    auto* r_type = ctxt->ty->get(expr.rhs->get_type());

    bool valid = true;
    using ast::BinOp;
    switch (expr.op) {
    case BinOp::AddEq:
    case BinOp::SubEq:
    case BinOp::MulEq:
    case BinOp::DivEq:
        check_expr_assignable(*expr.lhs);

        if (!l_type->is_numeric()) {
            ctxt->diag
                .error(expr.lhs->get_span(),
                       DiagnosticKind::InvalidBinaryOperand, expr.op,
                       l_type->basic_name(ctxt))
                .add_primary_note("must have an integral type");
            valid = false;
        }

        if (!r_type->is_numeric()) {
            ctxt->diag
                .error(expr.rhs->get_span(),
                       DiagnosticKind::InvalidBinaryOperand, expr.op,
                       r_type->basic_name(ctxt))
                .add_primary_note("must have an integral type");
            valid = false;
        }

        if (*l_type != *r_type) {
            ctxt->diag
                .error(expr.op_span, DiagnosticKind::InvalidAssignment,
                       r_type->basic_name(ctxt), l_type->basic_name(ctxt))
                .add_primary_note("must have equal types");
            valid = false;
        }

        break;
    case BinOp::ModEq:
    case BinOp::BitXorEq:
    case BinOp::BitAndEq:
    case BinOp::BitOrEq:
    case BinOp::ShlEq:
    case BinOp::ShrEq:
        check_expr_assignable(*expr.lhs);

        if (!l_type->is_integral()) {
            ctxt->diag
                .error(expr.lhs->get_span(),
                       DiagnosticKind::InvalidBinaryOperand, expr.op,
                       l_type->basic_name(ctxt))
                .add_primary_note("must have an integral type");
            valid = false;
        }

        if (!r_type->is_integral()) {
            ctxt->diag
                .error(expr.rhs->get_span(),
                       DiagnosticKind::InvalidBinaryOperand, expr.op,
                       r_type->basic_name(ctxt))
                .add_primary_note("must have an integral type");
            valid = false;
        }

        if (*l_type != *r_type) {
            ctxt->diag
                .error(expr.op_span, DiagnosticKind::InvalidAssignment,
                       r_type->basic_name(ctxt), l_type->basic_name(ctxt))
                .add_primary_note("must have equal types");
            valid = false;
        }

        break;
    case BinOp::Eq:
        check_expr_assignable(*expr.lhs);

        if (*l_type != *r_type) {
            ctxt->diag
                .error(expr.op_span, DiagnosticKind::InvalidAssignment,
                       r_type->basic_name(ctxt), l_type->basic_name(ctxt))
                .add_primary_note("must have equal types");
            valid = false;
        }
        break;
    case BinOp::LogicOr:
    case BinOp::LogicAnd:
        if (!l_type->is_logical() || !r_type->is_logical()) {
            ctxt->diag
                .error(expr.op_span, DiagnosticKind::InvalidOperands, expr.op,
                       l_type->basic_name(ctxt), r_type->basic_name(ctxt))
                .add_primary_note("must have boolean types");
            valid = false;
        }
        break;
    case BinOp::EqEq:
    case BinOp::Ne:
        if (*l_type != *r_type) {
            ctxt->diag
                .error(expr.op_span, DiagnosticKind::InvalidOperands, expr.op,
                       l_type->basic_name(ctxt), r_type->basic_name(ctxt))
                .add_primary_note("must have equal types");
            valid = false;
        }
        break;
    case BinOp::Range:
    case BinOp::RangeEq:
    case BinOp::BitOr:
    case BinOp::BitXor:
    case BinOp::BitAnd:
    case BinOp::Shl:
    case BinOp::Shr:
    case BinOp::Mod:
        if (!l_type->is_integral() || !r_type->is_integral() ||
            *l_type != *r_type) {
            ctxt->diag
                .error(expr.op_span, DiagnosticKind::InvalidOperands, expr.op,
                       l_type->basic_name(ctxt), r_type->basic_name(ctxt))
                .add_primary_note("must have equal integral types");
            valid = false;
        }
        break;
    case BinOp::Add:
    case BinOp::Sub:
    case BinOp::Mul:
    case BinOp::Div:
    case BinOp::Gt:
    case BinOp::Lt:
    case BinOp::Ge:
    case BinOp::Le:
        if (!l_type->is_numeric() || !r_type->is_numeric() ||
            *l_type != *r_type) {
            ctxt->diag
                .error(expr.op_span, DiagnosticKind::InvalidOperands, expr.op,
                       l_type->basic_name(ctxt), r_type->basic_name(ctxt))
                .add_primary_note("must have equal numeric types");
            valid = false;
        }

        break;
    case BinOp::ColonColon: {
        const auto* enum_type = type::dyn_cast<type::EnumType>(l_type);
        if (enum_type) { // TODO:
        }
        break;
    }
    }

    if (!valid)
        return;

    if (is_assignment_op(expr.op)) {
        if (const auto* ident = dyn_cast<ast::Identifier>(expr.lhs.get())) {
            if (ctxt->syms->is_var_const(ident->get_id())) {
                ctxt->diag
                    .error(expr.lhs->get_span(),
                           DiagnosticKind::AssignmentToConst,
                           ctxt->strings->get_string(ident->get_id()))
                    .add_primary_note("declared as const");
            }
        }
    }

    if (is_division_op(expr.op)) {
        if (const auto* int_expr = dyn_cast<ast::IntExpr>(expr.rhs.get())) {
            if (int_expr->val == 0) {
                ctxt->diag.error(expr.get_span(),
                                 DiagnosticKind::DivisionByZero);
            }
        } else if (const auto* float_expr =
                       dyn_cast<ast::FloatExpr>(expr.rhs.get())) {
            if (float_expr->val == 0) {
                ctxt->diag.error(expr.get_span(),
                                 DiagnosticKind::DivisionByZero);
            }
        }
    }
}

void SemChecker::visit(ast::CallExpr& expr) {
    expr.ident->accept(*this);
    for (const auto& arg : expr.args) {
        arg->accept(*this);
    }

    const auto* ident = dyn_cast<ast::Identifier>(expr.ident.get());
    if (!ident) {
        ctxt->diag
            .error(expr.ident->get_span(), DiagnosticKind::TypeMismatch,
                   "identifier",
                   ctxt->ty->get(expr.ident->get_type())->basic_name(ctxt))
            .add_primary_note("not callable");
        return;
    }

    auto* func = ctxt->ty->get_as<type::FunctionType>(expr.ident->get_type());
    if (!func) {
        ctxt->diag.error(ident->get_span(), DiagnosticKind::UndefinedIdentifier,
                         ctxt->strings->get_string(ident->get_id()));
        return;
    }

    auto params = func->get_params();
    if (expr.args.size() != params.size()) {
        ctxt->diag
            .error(expr.get_span(), DiagnosticKind::IncorrectArgQuantity,
                   params.size(), expr.args.size())
            .add_primary_note(expr.args.size() > params.size()
                                  ? "too many arguments"
                                  : "too few arguments");
        return;
    }

    for (size_t i = 0; i < params.size(); i++) {
        if (params[i] != expr.args[i]->get_type()) {
            ctxt->diag.error(
                expr.args[i]->get_span(), DiagnosticKind::TypeMismatch,
                ctxt->ty->get(params[i])->basic_name(ctxt),
                ctxt->ty->get(expr.args[i]->get_type())->basic_name(ctxt));
        }
    }
}

void SemChecker::visit(ast::ArrayExpr& expr) {
    expr.array->accept(*this);
    expr.val->accept(*this);

    if (!ctxt->ty->get(expr.array->get_type())->is_array()) {
        ctxt->diag
            .error(expr.get_span(), DiagnosticKind::TypeCannotBeIndexed,
                   ctxt->ty->get(expr.array->get_type())->basic_name(ctxt))
            .add_primary_note("must be an array type");
    }

    auto* val = ctxt->ty->get(expr.val->get_type());
    if (val && !val->is_integral()) {
        ctxt->diag
            .error(expr.val->get_span(), DiagnosticKind::InvalidIndexType,
                   val->basic_name(ctxt))
            .add_primary_note("index must be an integral type");
    }
}

void SemChecker::visit(ast::FieldExpr& expr) {
    expr.container->accept(*this);

    const auto* container = ctxt->ty->get(expr.container->get_type());

    const auto* struct_var =
        ctxt->ty->get_as<type::StructType>(expr.container->get_type());
    if (!struct_var) {
        ctxt->diag
            .error(expr.container->get_span(), DiagnosticKind::TypeHasNoFields,
                   container->basic_name(ctxt))
            .add_primary_note("not a struct type");

        return;
    }

    if (!struct_var->get_field_type(expr.field->get_id())) {
        ctxt->diag.error(expr.field->get_span(), DiagnosticKind::UnknownField,
                         container->basic_name(ctxt),
                         ctxt->strings->get_string(expr.field->get_id()));
    }
}

void SemChecker::visit(ast::ArrayInitExpr& expr) {
    for (auto& val : expr.vals) {
        val->accept(*this);
    }

    if (expr.vals.empty())
        return;

    const auto* valid_type = ctxt->ty->get(expr.vals.front()->get_type());

    for (auto& val : expr.vals) {
        if (expr.vals.front()->get_type() != val->get_type()) {
            ctxt->diag.error(val->get_span(), DiagnosticKind::TypeMismatch,
                             valid_type->basic_name(ctxt),
                             ctxt->ty->get(val->get_type())->basic_name(ctxt));
        }
    }
}

void SemChecker::visit(ast::StructExprField& expr) {
    if (!expr.ident->has_type()) {
        return;
    }

    if (expr.ident->get_type() != expr.val->get_type()) {
        ctxt->diag.error(
            expr.get_span(), DiagnosticKind::TypeMismatch,
            ctxt->ty->get(expr.ident->get_type())->basic_name(ctxt),
            ctxt->ty->get(expr.val->get_type())->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::StructInitExpr& expr) {
    for (auto& field : expr.fields) {
        field->accept(*this);
    }

    const auto* ident = dyn_cast<ast::Identifier>(expr.ident.get());
    if (!ident) {
        ctxt->diag.error(
            expr.ident->get_span(), DiagnosticKind::TypeMismatch, "identifier",
            ctxt->ty->get(expr.ident->get_type())->basic_name(ctxt));

        return;
    }
    const auto t = ctxt->syms->get_type(ident->get_id());
    if (!t) {
        ctxt->diag.error(expr.ident->get_span(),
                         DiagnosticKind::UndefinedIdentifier,
                         ctxt->strings->get_string(ident->get_id()));

        return;
    }

    const auto* struct_type = ctxt->ty->get_as<type::StructType>(*t);
    if (!struct_type) {
        ctxt->diag.error(
            expr.ident->get_span(), DiagnosticKind::NotAStruct,
            ctxt->strings->get_string(ident->get_id()),
            ctxt->ty->get(expr.ident->get_type())->basic_name(ctxt));

        return;
    }

    std::unordered_set<StringID> required_fields;
    for (const auto& field : struct_type->get_fields()) {
        required_fields.insert(field.first);
    }

    for (const auto& field : expr.fields) {
        if (required_fields.erase(field->ident->get_id()) == 0) {
            if (struct_type->has_field(field->ident->get_id())) {
                auto err = ctxt->diag.error(
                    field->get_span(),
                    DiagnosticKind::DuplicateFieldInitialization,
                    ctxt->strings->get_string(field->ident->get_id()));
                err.add_primary_note("initialized again here");
                for (const auto& f : expr.fields) {
                    if (f->ident->get_id() == field->ident->get_id()) {
                        err.add_note(f->get_span(), "first initialized here");
                        break;
                    }
                }

            } else {
                ctxt->diag.error(
                    field->ident->get_span(), DiagnosticKind::UnknownField,
                    ctxt->strings->get_string(ident->get_id()),
                    ctxt->strings->get_string(field->ident->get_id()));
            }
        }
    }

    if (!required_fields.empty()) {
        for (const auto& field : required_fields) {
            ctxt->diag.error(expr.get_span(),
                             DiagnosticKind::FieldNotInitialized,
                             ctxt->strings->get_string(field));
        }
    }
}

void SemChecker::visit(ast::TupleExpr& expr) {
    expr.first->accept(*this);
    expr.second->accept(*this);
}

void SemChecker::visit(ast::Block& block) {
    ctxt->syms->enter_scope(block.get_scope_id());
    for (auto& stmt : block.stmts) {
        stmt->accept(*this);
    }
    ctxt->syms->exit_scope();
}

void SemChecker::visit(ast::Param& /*param*/) {}

void SemChecker::visit(ast::SourceFileDecl& file) {
    for (auto& decl : file.const_decls)
        decl->accept(*this);

    for (auto& decl : file.decls)
        decl->accept(*this);
}

void SemChecker::visit(ast::FuncDecl& func) {
    const auto name = ctxt->strings->get_string(func.name->get_id());
    if (name == "main") {
        if (!func.params.empty()) {
            ctxt->diag.error(func.name->get_span(),
                             DiagnosticKind::MainFunctionParams);
        }

        if (func.ret.is_valid() && func.ret != type::builtin::VOID &&
            func.ret != type::builtin::I32) {
            ctxt->diag.error(func.name->get_span(),
                             DiagnosticKind::MainFunctionReturnType);
        }
    }

    current_return_type = func.ret;
    current_func_name = func.name.get();

    func.body->accept(*this);

    current_return_type = std::nullopt;
    current_func_name = nullptr;

    if (func.ret.is_valid() && func.ret != func.body->get_type()) {
        ctxt->diag.error(
            func.body->get_span(), DiagnosticKind::ReturnTypeMismatch,
            ctxt->strings->get_string(func.name->get_id()),
            ctxt->ty->get(func.ret)->basic_name(ctxt),
            ctxt->ty->get(func.body->get_type())->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::BreakStmt& stmt) {
    if (loop_depth == 0 || loop_stack.empty()) {
        ctxt->diag.error(stmt.get_span(), DiagnosticKind::BreakOutsideLoop);
        return;
    }

    if (stmt.expr)
        stmt.expr->accept(*this);

    auto& ctx = loop_stack.back();
    auto break_type = stmt.expr ? stmt.expr->get_type() : type::builtin::VOID;

    if (!ctx.has_break) {
        ctx.has_break = true;
        ctx.expected_type = break_type;
        ctx.first_break = stmt.get_span();
    } else if (ctx.expected_type != break_type) {
        ctxt->diag
            .error(stmt.get_span(), DiagnosticKind::BreakTypeMismatch,
                   ctxt->ty->get(ctx.expected_type)->basic_name(ctxt),
                   ctxt->ty->get(break_type)->basic_name(ctxt))
            .add_note(ctx.first_break, "first break here");
    }
}

void SemChecker::visit(ast::ContinueStmt& stmt) {
    if (loop_depth == 0) {
        ctxt->diag.error(stmt.get_span(), DiagnosticKind::ContinueOutsideLoop);
        return;
    }
}

void SemChecker::visit(ast::ForExpr& expr) {
    expr.expr->accept(*this);
    if (expr.expr->has_type() &&
        !ctxt->ty->get(expr.expr->get_type())->is_iterable()) {
        ctxt->diag
            .error(expr.expr->get_span(), DiagnosticKind::TypeNotIterable,
                   ctxt->ty->get(expr.expr->get_type())->basic_name(ctxt))
            .add_primary_note("must be an array or range type");
    }

    loop_depth++;
    loop_stack.emplace_back();
    expr.block->accept(*this);
    loop_depth--;
    loop_stack.pop_back();
}

void SemChecker::visit(ast::LetStmt& stmt) {
    if (stmt.val) {
        stmt.val->accept(*this);
    }

    if (!stmt.type.is_valid())
        return;

    if (stmt.val) {
        if (const auto* arr_type =
                ctxt->ty->get_as<type::ArrayType>(stmt.type)) {
            const auto expected_size = arr_type->get_size();

            if (expected_size && *expected_size <= 0) {
                ctxt->diag
                    .error(stmt.ident->get_span(),
                           DiagnosticKind::InvalidArraySize)
                    .add_primary_note("must be greater than zero");
            }
        }

        if (!stmt.val->has_type())
            return;

        if (stmt.type != stmt.val->get_type()) {
            ctxt->diag.error(
                stmt.val->get_span(), DiagnosticKind::InvalidAssignment,
                ctxt->ty->get(stmt.val->get_type())->basic_name(ctxt),
                ctxt->ty->get(stmt.type)->basic_name(ctxt));
        }
    }
}

void SemChecker::visit(ast::ReturnStmt& stmt) {
    if (stmt.expr)
        stmt.expr->accept(*this);

    expect(current_return_type && (current_func_name != nullptr),
           "ReturnStmt outside function");

    const auto return_type =
        stmt.expr ? stmt.expr->get_type() : type::builtin::VOID;
    if (!return_type.is_valid())
        return;

    if (*current_return_type != return_type) {
        ctxt->diag.error(stmt.get_span(), DiagnosticKind::ReturnTypeMismatch,
                         ctxt->strings->get_string(current_func_name->get_id()),
                         ctxt->ty->get(*current_return_type)->basic_name(ctxt),
                         ctxt->ty->get(return_type)->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::IfExpr& expr) {
    expr.expr->accept(*this);
    if (expr.expr->has_type() &&
        !ctxt->ty->get(expr.expr->get_type())->is_logical()) {
        ctxt->diag
            .error(expr.expr->get_span(), DiagnosticKind::TypeMismatch, "bool",
                   ctxt->ty->get(expr.expr->get_type())->basic_name(ctxt))
            .add_primary_note("condition must have a boolean type");
    }

    expr.block->accept(*this);

    if (expr.else_expr) {
        expr.else_expr->accept(*this);
        if (!expr.else_expr->has_type())
            return;

        if (expr.block->get_type() != expr.else_expr->get_type()) {
            ctxt->diag.error(
                expr.else_expr->get_span(),
                DiagnosticKind::ElseExprTypeMismatch,
                ctxt->ty->get(expr.block->get_type())->basic_name(ctxt),
                ctxt->ty->get(expr.else_expr->get_type())->basic_name(ctxt));
        }
    }
}

void SemChecker::visit(ast::ElseExpr& expr) {
    if (expr.if_expr)
        expr.if_expr->accept(*this);
    else if (expr.block)
        expr.block->accept(*this);
}

void SemChecker::visit(ast::LoopExpr& expr) {
    if (expr.expr) {
        expr.expr->accept(*this);
        if (expr.expr->has_type() &&
            !ctxt->ty->get(expr.expr->get_type())->is_integral()) {
            ctxt->diag.error(
                expr.expr->get_span(), DiagnosticKind::ExpectedInteger,
                ctxt->ty->get(expr.expr->get_type())->basic_name(ctxt));
        }
    }

    loop_depth++;
    loop_stack.emplace_back();
    expr.block->accept(*this);

    if (!expr.expr && !loop_stack.back().has_break) {
        ctxt->diag.error(expr.get_span(), DiagnosticKind::InfiniteLoop)
            .add_primary_note("add a `break` statement or a loop counter");
    }

    loop_depth--;
    loop_stack.pop_back();
}

void SemChecker::visit(ast::WhileExpr& expr) {
    expr.expr->accept(*this);
    if (expr.expr->has_type() &&
        !ctxt->ty->get(expr.expr->get_type())->is_logical()) {
        ctxt->diag
            .error(expr.expr->get_span(), DiagnosticKind::TypeMismatch, "bool",
                   ctxt->ty->get(expr.expr->get_type())->basic_name(ctxt))
            .add_primary_note("condition must be a `bool`");
    }

    loop_depth++;
    loop_stack.emplace_back();
    expr.block->accept(*this);
    loop_depth--;
    loop_stack.pop_back();
}

void SemChecker::visit(ast::StringExpr& /*expr*/) {}

void SemChecker::visit(ast::CharExpr& expr) {
    if (ctxt->src->get_string(expr.get_span()).length() > 1) {
        ctxt->diag.error(expr.get_span(), DiagnosticKind::MoreThanOneChar);
    }
}

void SemChecker::visit(ast::StructField& /*field*/) {}

void SemChecker::visit(ast::StructDecl& decl) {
    const auto* struct_type =
        ctxt->ty->get_as<type::StructType>(decl.get_type());
    expect(struct_type != nullptr, "StructDecl should have type StructType");

    if (is_recursive_struct(struct_type, decl.get_type())) {
        ctxt->diag.error(decl.ident->get_span(),
                         DiagnosticKind::RecursiveStructDefiniton,
                         ctxt->strings->get_string(decl.ident->get_id()));
    }
}

void SemChecker::visit(ast::EnumField& /*field*/) {}

void SemChecker::visit(ast::EnumDecl& /*decl*/) {}

void SemChecker::visit(ast::ConstDecl& decl) {
    decl.val->accept(*this);

    if (!decl.type.is_valid() || !decl.val->has_type())
        return;

    if (decl.type != decl.val->get_type()) {
        ctxt->diag.error(decl.val->get_span(),
                         DiagnosticKind::InvalidAssignment,
                         ctxt->ty->get(decl.val->get_type())->basic_name(ctxt),
                         ctxt->ty->get(decl.type)->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::StaticDecl& decl) {
    decl.val->accept(*this);

    if (!decl.type.is_valid() || !decl.val->has_type())
        return;

    if (decl.type != decl.val->get_type()) {
        ctxt->diag.error(decl.val->get_span(),
                         DiagnosticKind::InvalidAssignment,
                         ctxt->ty->get(decl.val->get_type())->basic_name(ctxt),
                         ctxt->ty->get(decl.type)->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::TraitDecl& decl) {
    ctxt->syms->enter_scope(decl.scope);

    for (auto& c : decl.consts) {
        c->accept(*this);
    }

    for (auto& type : decl.types) {
        type->accept(*this);
    }

    for (auto& func : decl.funcs) {
        func->accept(*this);
    }

    ctxt->syms->exit_scope();
}

void SemChecker::visit(ast::TypeAliasDecl& /*decl*/) {}

void SemChecker::visit(ast::TraitFuncDecl& decl) {
    if (decl.body) {
        current_return_type = decl.ret;
        current_func_name = decl.name.get();

        decl.body->accept(*this);

        if (decl.body->has_type() && decl.ret != decl.body->get_type()) {
            ctxt->diag.error(
                decl.body->get_span(), DiagnosticKind::ReturnTypeMismatch,
                ctxt->strings->get_string(decl.name->get_id()),
                ctxt->ty->get(decl.ret)->basic_name(ctxt),
                ctxt->ty->get(decl.body->get_type())->basic_name(ctxt));
        }

        current_return_type = std::nullopt;
        current_func_name = nullptr;
    }
}

void SemChecker::check_expr_assignable(ast::Expr& expr) const {
    if (!expr.is_assignable()) {
        ctxt->diag.error(expr.get_span(), DiagnosticKind::ExprNotAssignable)
            .add_primary_note("not a variable or field");
    }
}

bool SemChecker::is_recursive_struct(const type::StructType* s,
                                     type::TypeRef orig) const {
    const auto& fields = s->get_fields();

    return std::ranges::any_of(
        fields.cbegin(), fields.cend(), [this, orig](const auto& field) {
            const auto& field_type = field.second;

            if (field_type.first == orig)
                return true;

            if (const auto* nested_struct =
                    ctxt->ty->get_as<type::StructType>(field_type.first)) {
                if (is_recursive_struct(nested_struct, orig))
                    return true;
            }

            return false;
        });
}
} // namespace z
