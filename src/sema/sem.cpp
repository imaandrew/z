#include "sem.h"
#include "core/string_pool.h"
#include "diag/diagnostics.h"
#include "parser/ast.h"
#include "type/type.h"
#include "type/type_arena.h"
#include "type/type_ref.h"
#include <cassert>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <unordered_set>
#include <utility>

namespace z {

void SemChecker::visit(ast::Identifier& ident) {
    if (!ident.has_type()) {
        ctxt->diag.emit(ident.get_span(), DiagnosticKind::UndefinedIdentifier,
                        ident.to_string(ctxt->strings.get()));

        return;
    }

    if (!ctxt->syms->is_var_initialized(ident.get_id())) {
        ctxt->diag.emit(ident.get_span(), DiagnosticKind::UninitializedVar,
                        ctxt->strings->get_string(ident.get_id()));
    }
}

void SemChecker::visit(ast::IntExpr& expr) {
    const auto* type = ctxt->ty->get_as<type::IntegerType>(expr.node_type);
    assert(type && "IntExpr should have IntegerType");

    std::uint64_t max = 0;
    switch (type->get_width()) {
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
        std::unreachable();
    }

    if (expr.val > max) {
        ctxt->diag.emit(expr.get_span(), DiagnosticKind::NumericLiteralTooBig,
                        type->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::FloatExpr& expr) {
    const auto* type = ctxt->ty->get_as<type::FloatType>(expr.node_type);
    assert(type && "FloatExpr should have FloatType");

    double max = 0;
    switch (type->get_width()) {
    case 32:
        max = FLT_MAX;
        break;
    case 64:
        max = DBL_MAX;
        break;
    default:
        std::unreachable();
    }

    if (expr.val > max) {
        ctxt->diag.emit(expr.get_span(), DiagnosticKind::NumericLiteralTooBig,
                        type->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::BoolExpr& /*expr*/) {}

void SemChecker::visit(ast::PrefixExpr& expr) {
    expr.expr->accept(*this);
    if (!expr.expr->has_type())
        return;

    const auto* type = ctxt->ty->get(expr.expr->node_type);
    using ast::UnOp;
    switch (expr.op) {
    case UnOp::Inc:
    case UnOp::Dec:
        check_expr_assignable(*expr.expr);

        if (!type->is_integral()) {
            ctxt->diag.emit(expr.expr->get_span(),
                            DiagnosticKind::InvalidUnaryOperand, expr.op,
                            type->basic_name(ctxt));
        }
        break;
    case UnOp::LogicNot:
        if (!type->is_logical()) {
            ctxt->diag.emit(expr.expr->get_span(),
                            DiagnosticKind::InvalidUnaryOperand, expr.op,
                            type->basic_name(ctxt));
        }
        break;
    case UnOp::BitNot:
        if (!type->is_integral()) {
            ctxt->diag.emit(expr.expr->get_span(),
                            DiagnosticKind::InvalidUnaryOperand, expr.op,
                            type->basic_name(ctxt));
        }
        break;
    case UnOp::Neg:
        if (!type->is_numeric()) {
            ctxt->diag.emit(expr.expr->get_span(),
                            DiagnosticKind::InvalidUnaryOperand, expr.op,
                            type->basic_name(ctxt));
        }
        break;
    default:
        std::unreachable();
    }
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

        if (!ctxt->ty->get(expr.expr->node_type)->is_integral()) {
            ctxt->diag.emit(
                expr.expr->get_span(), DiagnosticKind::InvalidUnaryOperand,
                expr.op, ctxt->ty->get(expr.expr->node_type)->basic_name(ctxt));
        }
        break;
    default:
        std::unreachable();
    }
}

void SemChecker::visit(ast::BinaryExpr& expr) {
    expr.lhs->accept(*this);
    expr.rhs->accept(*this);

    if (!expr.lhs->has_type() || !expr.rhs->has_type())
        return;

    auto* l_type = ctxt->ty->get(expr.lhs->node_type);
    auto* r_type = ctxt->ty->get(expr.rhs->node_type);

    bool valid = true;
    using ast::BinOp;
    switch (expr.op) {
    case BinOp::AddEq:
    case BinOp::SubEq:
    case BinOp::MulEq:
    case BinOp::DivEq:
        check_expr_assignable(*expr.lhs);

        if (!l_type->is_numeric()) {
            ctxt->diag.emit(expr.lhs->get_span(),
                            DiagnosticKind::InvalidBinaryOperand, expr.op,
                            l_type->basic_name(ctxt));
            valid = false;
        }

        if (!r_type->is_numeric()) {
            ctxt->diag.emit(expr.rhs->get_span(),
                            DiagnosticKind::InvalidBinaryOperand, expr.op,
                            r_type->basic_name(ctxt));
            valid = false;
        }

        if (*l_type != *r_type) {
            ctxt->diag.emit(expr.op_span, DiagnosticKind::InvalidAssignment,
                            r_type->basic_name(ctxt), l_type->basic_name(ctxt));
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
            ctxt->diag.emit(expr.lhs->get_span(),
                            DiagnosticKind::InvalidBinaryOperand, expr.op,
                            l_type->basic_name(ctxt));
            valid = false;
        }

        if (!r_type->is_integral()) {
            ctxt->diag.emit(expr.rhs->get_span(),
                            DiagnosticKind::InvalidBinaryOperand, expr.op,
                            r_type->basic_name(ctxt));
            valid = false;
        }

        if (*l_type != *r_type) {
            ctxt->diag.emit(expr.op_span, DiagnosticKind::InvalidAssignment,
                            r_type->basic_name(ctxt), l_type->basic_name(ctxt));
            valid = false;
        }

        break;
    case BinOp::Eq:
        check_expr_assignable(*expr.lhs);

        if (*l_type != *r_type) {
            ctxt->diag.emit(expr.op_span, DiagnosticKind::InvalidAssignment,
                            r_type->basic_name(ctxt), l_type->basic_name(ctxt));
            valid = false;
        }
        break;
    case BinOp::LogicOr:
    case BinOp::LogicAnd:
        if (!l_type->is_logical() || !r_type->is_logical()) {
            ctxt->diag.emit(expr.op_span, DiagnosticKind::InvalidOperands,
                            expr.op, l_type->basic_name(ctxt),
                            r_type->basic_name(ctxt));
            valid = false;
        }
        break;
    case BinOp::EqEq:
    case BinOp::Ne:
        if (*l_type != *r_type) {
            ctxt->diag.emit(expr.op_span, DiagnosticKind::InvalidOperands,
                            expr.op, l_type->basic_name(ctxt),
                            r_type->basic_name(ctxt));
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
            ctxt->diag.emit(expr.op_span, DiagnosticKind::InvalidOperands,
                            expr.op, l_type->basic_name(ctxt),
                            r_type->basic_name(ctxt));
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
            ctxt->diag.emit(expr.op_span, DiagnosticKind::InvalidOperands,
                            expr.op, l_type->basic_name(ctxt),
                            r_type->basic_name(ctxt));
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
                ctxt->diag.emit(expr.lhs->get_span(),
                                DiagnosticKind::AssignmentToConst,
                                ctxt->strings->get_string(ident->get_id()));
            }
        }
    }

    if (is_division_op(expr.op)) {
        if (const auto* int_expr = dyn_cast<ast::IntExpr>(expr.rhs.get())) {
            if (int_expr->val == 0) {
                ctxt->diag.emit(expr.rhs->get_span(),
                                DiagnosticKind::DivisionByZero);
            }
        } else if (const auto* float_expr =
                       dyn_cast<ast::FloatExpr>(expr.rhs.get())) {
            if (float_expr->val == 0) {
                ctxt->diag.emit(expr.rhs->get_span(),
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
        ctxt->diag.emit(expr.ident->get_span(), DiagnosticKind::TypeMismatch,
                        "identifier",
                        ctxt->ty->get(expr.ident->node_type)->basic_name(ctxt));
        return;
    }

    auto* func = ctxt->ty->get_as<type::FunctionType>(expr.ident->node_type);
    if (!func) {
        ctxt->diag.emit(ident->get_span(), DiagnosticKind::UndefinedIdentifier,
                        ident->to_string(ctxt->strings.get()));
        return;
    }

    auto params = func->get_params();
    if (expr.args.size() != params.size()) {
        ctxt->diag.emit(expr.get_span(), DiagnosticKind::IncorrectArgQuantity,
                        params.size(), expr.args.size());
        return;
    }

    for (size_t i = 0; i < params.size(); i++) {
        if (params[i] != expr.args[i]->node_type) {
            ctxt->diag.emit(
                expr.args[i]->get_span(), DiagnosticKind::TypeMismatch,
                ctxt->ty->get(params[i])->basic_name(ctxt),
                ctxt->ty->get(expr.args[i]->node_type)->basic_name(ctxt));
        }
    }
}

void SemChecker::visit(ast::ArrayExpr& expr) {
    expr.ident->accept(*this);
    expr.val->accept(*this);

    const auto* ident = dyn_cast<ast::Identifier>(expr.ident.get());
    if (!ident || !ctxt->ty->get(ident->node_type)->is_array()) {
        ctxt->diag.emit(expr.val->get_span(),
                        DiagnosticKind::TypeCannotBeIndexed,
                        ctxt->ty->get(expr.ident->node_type)->basic_name(ctxt));
    }

    auto* val = ctxt->ty->get(expr.val->node_type);
    if (val && !val->is_integral()) {
        ctxt->diag.emit(expr.val->get_span(), DiagnosticKind::InvalidIndexType,
                        val->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::FieldExpr& expr) {
    expr.container->accept(*this);

    const auto* container = ctxt->ty->get(expr.container->node_type);

    const auto* struct_var =
        ctxt->ty->get_as<type::StructType>(expr.container->node_type);
    if (!struct_var) {
        ctxt->diag.emit(expr.container->get_span(),
                        DiagnosticKind::TypeHasNoFields,
                        container->basic_name(ctxt));

        return;
    }

    if (!struct_var->get_field_type(expr.field->get_id())) {
        ctxt->diag.emit(expr.field->get_span(), DiagnosticKind::UnknownField,
                        container->basic_name(ctxt),
                        expr.field->to_string(ctxt->strings.get()));
    }
}

void SemChecker::visit(ast::ArrayInitExpr& expr) {
    for (auto& val : expr.vals) {
        val->accept(*this);
    }

    if (expr.vals.empty())
        return;

    const auto* valid_type = ctxt->ty->get(expr.vals.front()->node_type);

    for (auto& val : expr.vals) {
        if (expr.vals.front()->node_type != val->node_type) {
            ctxt->diag.emit(val->get_span(), DiagnosticKind::TypeMismatch,
                            valid_type->basic_name(ctxt),
                            ctxt->ty->get(val->node_type)->basic_name(ctxt));
        }
    }
}

void SemChecker::visit(ast::StructExprField& expr) {
    if (!expr.ident->has_type()) {
        return;
    }

    if (expr.ident->node_type != expr.val->node_type) {
        ctxt->diag.emit(expr.get_span(), DiagnosticKind::TypeMismatch,
                        ctxt->ty->get(expr.ident->node_type)->basic_name(ctxt),
                        ctxt->ty->get(expr.val->node_type)->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::StructInitExpr& expr) {
    for (auto& field : expr.fields) {
        field->accept(*this);
    }

    const auto* ident = dyn_cast<ast::Identifier>(expr.ident.get());
    if (!ident) {
        ctxt->diag.emit(expr.ident->get_span(), DiagnosticKind::TypeMismatch,
                        "identifier",
                        ctxt->ty->get(expr.ident->node_type)->basic_name(ctxt));

        return;
    }
    const auto t = ctxt->syms->get_type(ident->get_id());
    if (!t) {
        ctxt->diag.emit(expr.ident->get_span(),
                        DiagnosticKind::UndefinedIdentifier,
                        ctxt->strings->get_string(ident->get_id()));

        return;
    }

    const auto* struct_type = ctxt->ty->get_as<type::StructType>(*t);
    if (!struct_type) {
        ctxt->diag.emit(expr.ident->get_span(), DiagnosticKind::NotAStruct,
                        ident->to_string(ctxt->strings.get()),
                        ctxt->ty->get(expr.ident->node_type)->basic_name(ctxt));

        return;
    }

    std::unordered_set<StringID> required_fields;
    for (const auto& field : struct_type->get_fields()) {
        required_fields.insert(field.first);
    }

    for (const auto& field : expr.fields) {
        if (required_fields.erase(field->ident->get_id()) == 0) {
            if (struct_type->has_field(field->ident->get_id())) {
                ctxt->diag.emit(field->get_span(),
                                DiagnosticKind::DuplicateFieldInitialization,
                                field->ident->to_string(ctxt->strings.get()));

            } else {
                ctxt->diag.emit(field->ident->get_span(),
                                DiagnosticKind::UnknownField,
                                ident->to_string(ctxt->strings.get()),
                                field->ident->to_string(ctxt->strings.get()));
            }
        }
    }

    if (!required_fields.empty()) {
        for (const auto& field : required_fields) {
            ctxt->diag.emit(expr.get_span(),
                            DiagnosticKind::FieldNotInitialized,
                            std::format("FIELD: {}", field.id));
        }
    }
}

void SemChecker::visit(ast::TupleExpr& expr) {
    expr.first->accept(*this);
    expr.second->accept(*this);
}

void SemChecker::visit(ast::Block& block) {
    ctxt->syms->enter_scope(block.get_scope_id());
    is_stmt_reachable = ReachableStatus::Reachable;
    for (auto& stmt : block.stmts) {
        if (is_stmt_reachable == ReachableStatus::Unreachable) {
            // ctxt->diag.emit(stmt->get_span(),
            // DiagnosticKind::UnreachableStmt);
            is_stmt_reachable = ReachableStatus::WarningEmitted;
        }
        stmt->accept(*this);
    }
    ctxt->syms->exit_scope();
}

void SemChecker::visit(ast::Param& /*param*/) {}

void SemChecker::visit(ast::SourceFileDecl& file) {
    for (auto& decl : file.decls)
        decl->accept(*this);
}

void SemChecker::visit(ast::FuncDecl& func) {
    const auto name = ctxt->strings->get_string(func.name->get_id());
    if (name == "main") {
        if (!func.params.empty()) {
            ctxt->diag.emit(func.name->get_span(),
                            DiagnosticKind::MainFunctionParams);
        }

        if (func.ret.is_valid() && func.ret != type::TypeArena::VOID &&
            func.ret != type::TypeArena::I32) {
            ctxt->diag.emit(func.name->get_span(),
                            DiagnosticKind::MainFunctionReturnType);
        }
    }

    current_return_type = func.ret;
    current_func_name = func.name.get();

    func.body->accept(*this);

    current_return_type = std::nullopt;
    current_func_name = nullptr;

    if (func.ret.is_valid() && func.ret != func.body->node_type) {
        ctxt->diag.emit(func.body->get_span(),
                        DiagnosticKind::ReturnTypeMismatch,
                        func.name->to_string(ctxt->strings.get()),
                        ctxt->ty->get(func.ret)->basic_name(ctxt),
                        ctxt->ty->get(func.body->node_type)->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::BreakStmt& stmt) {
    if (loop_depth == 0 || loop_stack.empty()) {
        ctxt->diag.emit(stmt.get_span(), DiagnosticKind::BreakOutsideLoop);
        return;
    }

    is_stmt_reachable = ReachableStatus::Unreachable;

    if (stmt.expr)
        stmt.expr->accept(*this);

    auto& ctx = loop_stack.back();
    auto break_type = stmt.expr ? stmt.expr->node_type : type::TypeArena::VOID;

    if (!ctx.has_break) {
        ctx.has_break = true;
        ctx.expected_type = break_type;
        ctx.first_break = stmt.get_span();
    } else if (ctx.expected_type != break_type) {
        ctxt->diag.emit(stmt.get_span(), DiagnosticKind::BreakTypeMismatch,
                        ctxt->ty->get(ctx.expected_type)->basic_name(ctxt),
                        ctxt->ty->get(break_type)->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::ContinueStmt& stmt) {
    if (loop_depth == 0) {
        ctxt->diag.emit(stmt.get_span(), DiagnosticKind::ContinueOutsideLoop);
        return;
    }

    is_stmt_reachable = ReachableStatus::Unreachable;
}

void SemChecker::visit(ast::ForExpr& expr) {
    expr.expr->accept(*this);
    if (expr.expr->has_type() &&
        !ctxt->ty->get(expr.expr->node_type)->is_iterable()) {
        ctxt->diag.emit(expr.expr->get_span(), DiagnosticKind::TypeNotIterable,
                        ctxt->ty->get(expr.expr->node_type)->basic_name(ctxt));
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
                ctxt->diag.emit(stmt.ident->get_span(),
                                DiagnosticKind::InvalidArraySize);
            }
        }

        if (!stmt.val->has_type())
            return;

        if (stmt.type != stmt.val->node_type) {
            ctxt->diag.emit(
                stmt.val->get_span(), DiagnosticKind::InvalidAssignment,
                ctxt->ty->get(stmt.val->node_type)->basic_name(ctxt),
                ctxt->ty->get(stmt.type)->basic_name(ctxt));
        }
    }
}

void SemChecker::visit(ast::ReturnStmt& stmt) {
    if (stmt.expr)
        stmt.expr->accept(*this);

    is_stmt_reachable = ReachableStatus::Unreachable;

    assert(current_return_type && current_func_name);

    const auto return_type =
        stmt.expr ? stmt.expr->node_type : type::TypeArena::VOID;
    if (!return_type.is_valid())
        return;

    if (*current_return_type != return_type) {
        ctxt->diag.emit(stmt.get_span(), DiagnosticKind::ReturnTypeMismatch,
                        ctxt->strings->get_string(current_func_name->get_id()),
                        ctxt->ty->get(*current_return_type)->basic_name(ctxt),
                        ctxt->ty->get(return_type)->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::IfExpr& expr) {
    expr.expr->accept(*this);
    if (expr.expr->has_type() &&
        !ctxt->ty->get(expr.expr->node_type)->is_logical()) {
        ctxt->diag.emit(expr.expr->get_span(), DiagnosticKind::TypeMismatch,
                        "bool",
                        ctxt->ty->get(expr.expr->node_type)->basic_name(ctxt));
    }

    expr.block->accept(*this);

    if (expr.else_expr) {
        expr.else_expr->accept(*this);
        if (!expr.else_expr->has_type())
            return;

        if (expr.block->node_type != expr.else_expr->node_type) {
            ctxt->diag.emit(
                expr.else_expr->get_span(),
                DiagnosticKind::ElseExprTypeMismatch,
                ctxt->ty->get(expr.block->node_type)->basic_name(ctxt),
                ctxt->ty->get(expr.else_expr->node_type)->basic_name(ctxt));
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
            !ctxt->ty->get(expr.expr->node_type)->is_integral()) {
            ctxt->diag.emit(
                expr.expr->get_span(), DiagnosticKind::ExpectedInteger,
                ctxt->ty->get(expr.expr->node_type)->basic_name(ctxt));
        }
    }

    loop_depth++;
    loop_stack.emplace_back();
    expr.block->accept(*this);

    if (!expr.expr && !loop_stack.back().has_break) {
        ctxt->diag.emit(expr.get_span(), DiagnosticKind::InfiniteLoop);
    }

    loop_depth--;
    loop_stack.pop_back();
}

void SemChecker::visit(ast::WhileExpr& expr) {
    expr.expr->accept(*this);
    if (expr.expr->has_type() &&
        !ctxt->ty->get(expr.expr->node_type)->is_logical()) {
        ctxt->diag.emit(expr.expr->get_span(), DiagnosticKind::TypeMismatch,
                        "bool",
                        ctxt->ty->get(expr.expr->node_type)->basic_name(ctxt));
    }

    loop_depth++;
    loop_stack.emplace_back();
    expr.block->accept(*this);
    loop_depth--;
    loop_stack.pop_back();
}

void SemChecker::visit(ast::StringExpr& /*expr*/) {}

void SemChecker::visit(ast::CharExpr& expr) {
    if (ctxt->src->get_string(expr.span).length() > 1) {
        ctxt->diag.emit(expr.get_span(), DiagnosticKind::MoreThanOneChar);
    }
}

void SemChecker::visit(ast::StructField& /*field*/) {}

void SemChecker::visit(ast::StructDecl& decl) {
    const auto* struct_type =
        ctxt->ty->get_as<type::StructType>(decl.node_type);
    assert(struct_type && "StructDecl has type StructType");

    if (is_recursive_struct(struct_type, decl.node_type)) {
        ctxt->diag.emit(decl.ident->get_span(),
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

    if (decl.type != decl.val->node_type) {
        ctxt->diag.emit(decl.val->get_span(), DiagnosticKind::InvalidAssignment,
                        ctxt->ty->get(decl.val->node_type)->basic_name(ctxt),
                        ctxt->ty->get(decl.type)->basic_name(ctxt));
    }
}

void SemChecker::visit(ast::StaticDecl& decl) {
    decl.val->accept(*this);

    if (!decl.type.is_valid() || !decl.val->has_type())
        return;

    if (decl.type != decl.val->node_type) {
        ctxt->diag.emit(decl.val->get_span(), DiagnosticKind::InvalidAssignment,
                        ctxt->ty->get(decl.val->node_type)->basic_name(ctxt),
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

        if (decl.body->has_type() && decl.ret != decl.body->node_type) {
            ctxt->diag.emit(
                decl.body->get_span(), DiagnosticKind::ReturnTypeMismatch,
                decl.name->to_string(ctxt->strings.get()),
                ctxt->ty->get(decl.ret)->basic_name(ctxt),
                ctxt->ty->get(decl.body->node_type)->basic_name(ctxt));
        }

        current_return_type = std::nullopt;
        current_func_name = nullptr;
    }
}

void SemChecker::check_expr_assignable(ast::Expr& expr) const {
    if (!expr.is_assignable()) {
        ctxt->diag.emit(expr.get_span(), DiagnosticKind::ExprNotAssignable);
    }
}

bool SemChecker::is_recursive_struct(const type::StructType* s,
                                     type::TypeRef orig) const {
    for (const auto& [_, field_type] : s->get_fields()) {
        if (field_type.first == orig)
            return true;

        if (const auto* nested_struct =
                ctxt->ty->get_as<type::StructType>(field_type.first)) {
            if (is_recursive_struct(nested_struct, orig))
                return true;
        }
    }

    return false;
}
} // namespace z
