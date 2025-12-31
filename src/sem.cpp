#include "sem.h"
#include "ast.h"
#include "diagnostics.h"
#include "token.h"
#include "type.h"
#include <cstddef>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace z {

void SemChecker::visit(ast::Identifier& ident) {
    if (!ident.has_type()) {
        diag.emit(ident.tok.get_span(), DiagnosticKind::UndefinedIdentifier,
                  ident.to_string());
    }
}

void SemChecker::visit(ast::IntExpr& /*expr*/) {}

void SemChecker::visit(ast::FloatExpr& /*expr*/) {}

void SemChecker::visit(ast::BoolExpr& /*expr*/) {}

void SemChecker::visit(ast::PrefixExpr& expr) {
    expr.expr->accept(*this);

    if (!expr.expr->has_type())
        return;

    auto* type = expr.expr->node_type.get();
    switch (expr.op.get_kind()) {
    case TokenKind::PlusPlus:
    case TokenKind::MinusMinus:
        check_expr_assignable(*expr.expr);

        if (!type->is_integral()) {
            diag.emit(
                expr.expr->get_span(), DiagnosticKind::InvalidUnaryOperand,
                operator_to_string(expr.op.get_kind()), type->basic_name());
        }
        break;
    case TokenKind::LogicalNot:
        if (!type->is_logical()) {
            diag.emit(
                expr.expr->get_span(), DiagnosticKind::InvalidUnaryOperand,
                operator_to_string(expr.op.get_kind()), type->basic_name());
        }
        break;
    case TokenKind::Not:
        if (!type->is_integral()) {
            diag.emit(
                expr.expr->get_span(), DiagnosticKind::InvalidUnaryOperand,
                operator_to_string(expr.op.get_kind()), type->basic_name());
        }
        break;
    case TokenKind::Minus:
        if (!type->is_numeric()) {
            diag.emit(
                expr.expr->get_span(), DiagnosticKind::InvalidUnaryOperand,
                operator_to_string(expr.op.get_kind()), type->basic_name());
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

    switch (expr.op.get_kind()) {
    case TokenKind::PlusPlus:
    case TokenKind::MinusMinus:
        check_expr_assignable(*expr.expr);

        if (!expr.expr->node_type->is_integral()) {
            diag.emit(expr.expr->get_span(),
                      DiagnosticKind::InvalidUnaryOperand,
                      operator_to_string(expr.op.get_kind()),
                      expr.expr->node_type->basic_name());
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

    auto* l_type = expr.lhs->node_type.get();
    auto* r_type = expr.rhs->node_type.get();

    switch (expr.op.get_kind()) {
    case TokenKind::PlusEq:
    case TokenKind::MinusEq:
    case TokenKind::StarEq:
    case TokenKind::SlashEq:
        check_expr_assignable(*expr.lhs);

        if (!l_type->is_numeric()) {
            diag.emit(
                expr.lhs->get_span(), DiagnosticKind::InvalidBinaryOperand,
                operator_to_string(expr.op.get_kind()), l_type->basic_name());
        }

        if (!r_type->is_numeric()) {
            diag.emit(
                expr.rhs->get_span(), DiagnosticKind::InvalidBinaryOperand,
                operator_to_string(expr.op.get_kind()), r_type->basic_name());
        }

        if (!l_type->is_assignment_compatible(r_type)) {
            diag.emit(expr.op.get_span(), DiagnosticKind::InvalidAssignment,
                      r_type->basic_name(), l_type->basic_name());
        }

        break;
    case TokenKind::PercentEq:
    case TokenKind::CaretEq:
    case TokenKind::AndEq:
    case TokenKind::OrEq:
    case TokenKind::ShlEq:
    case TokenKind::ShrEq:
        check_expr_assignable(*expr.lhs);

        if (!l_type->is_integral()) {
            diag.emit(
                expr.lhs->get_span(), DiagnosticKind::InvalidBinaryOperand,
                operator_to_string(expr.op.get_kind()), l_type->basic_name());
        }

        if (!r_type->is_integral()) {
            diag.emit(
                expr.rhs->get_span(), DiagnosticKind::InvalidBinaryOperand,
                operator_to_string(expr.op.get_kind()), r_type->basic_name());
        }

        if (!l_type->is_assignment_compatible(r_type)) {
            diag.emit(expr.op.get_span(), DiagnosticKind::InvalidAssignment,
                      r_type->basic_name(), l_type->basic_name());
        }

        break;
    case TokenKind::Eq:
        check_expr_assignable(*expr.lhs);

        if (!l_type->is_assignment_compatible(r_type)) {
            diag.emit(expr.op.get_span(), DiagnosticKind::InvalidAssignment,
                      r_type->basic_name(), l_type->basic_name());
        }
        break;
    case TokenKind::Colon:
    case TokenKind::OrOr:
    case TokenKind::AndAnd:
    case TokenKind::EqEq:
    case TokenKind::Ne:
    case TokenKind::Gt:
    case TokenKind::Lt:
    case TokenKind::Ge:
    case TokenKind::Le:
        if (!l_type->is_logical() || !r_type->is_logical()) {
            diag.emit(expr.op.get_span(), DiagnosticKind::InvalidOperands,
                      operator_to_string(expr.op.get_kind()),
                      l_type->basic_name(), r_type->basic_name());
        }
        break;
    case TokenKind::Range:
    case TokenKind::RangeEq:
    case TokenKind::Or:
    case TokenKind::Caret:
    case TokenKind::And:
    case TokenKind::Shl:
    case TokenKind::Shr:
    case TokenKind::Percent:
        if (!l_type->is_integral() || !r_type->is_integral() ||
            !l_type->is_arithmetic_compatible(r_type)) {
            diag.emit(expr.op.get_span(), DiagnosticKind::InvalidOperands,
                      operator_to_string(expr.op.get_kind()),
                      l_type->basic_name(), r_type->basic_name());
        }
        break;
    case TokenKind::Plus:
    case TokenKind::Minus:
    case TokenKind::Star:
    case TokenKind::Slash:
        if (!l_type->is_numeric() || !r_type->is_numeric() ||
            !l_type->is_arithmetic_compatible(r_type)) {
            diag.emit(expr.op.get_span(), DiagnosticKind::InvalidOperands,
                      operator_to_string(expr.op.get_kind()),
                      l_type->basic_name(), r_type->basic_name());
        }

        break;
    case TokenKind::ColonColon:
        break;
    case TokenKind::Dot: {
        if (!l_type->is_struct()) {
            diag.emit(expr.lhs->get_span(), DiagnosticKind::TypeHasNoFields,
                      l_type->basic_name());
        }

        break;
    }
    default:
        std::unreachable();
    }
}

void SemChecker::visit(ast::TernaryExpr& expr) {
    expr.lhs->accept(*this);
    expr.mhs->accept(*this);
    expr.rhs->accept(*this);

    if (expr.lhs->has_type() && expr.lhs->node_type->is_logical()) {
        diag.emit(expr.lhs->get_span(), DiagnosticKind::TypeMismatch, "bool",
                  expr.lhs->node_type->basic_name());
    }

    if (!expr.mhs->has_type() || !expr.rhs->has_type())
        return;

    if (!expr.mhs->node_type->is_assignment_compatible(
            expr.rhs->node_type.get())) {
        diag.emit(expr.op2.get_span(), DiagnosticKind::InvalidOperands,
                  operator_to_string(expr.op2.get_kind()),
                  expr.mhs->node_type->basic_name(),
                  expr.rhs->node_type->basic_name());
    }
}

void SemChecker::visit(ast::CallExpr& expr) {
    expr.ident->accept(*this);
    if (!expr.ident->has_type())
        return;

    if (const auto* ident = dyn_cast<ast::Identifier>(expr.ident.get())) {
        auto* func = dyn_cast<type::FunctionType>(expr.ident->node_type.get());
        if (func == nullptr) {
            diag.emit(ident->get_span(), DiagnosticKind::UndefinedIdentifier,
                      ident->to_string());
        }

        auto params = func->get_params();
        if (expr.args.size() != params.size()) {
            diag.emit(expr.get_span(), DiagnosticKind::IncorrectArgQuantity,
                      params.size(), expr.args.size());
        }

        for (size_t i = 0; i < params.size(); i++) {
            if (!params[i]->is_assignment_compatible(
                    expr.args[i]->node_type.get())) {
                diag.emit(expr.args[i]->get_span(),
                          DiagnosticKind::TypeMismatch, params[i]->basic_name(),
                          expr.args[i]->node_type->basic_name());
            }
        }
    } else {
        diag.emit(expr.ident->get_span(), DiagnosticKind::TypeMismatch,
                  "identifier", expr.ident->node_type->basic_name());
    }
}

void SemChecker::visit(ast::ArrayExpr& expr) {
    expr.ident->accept(*this);
    expr.val->accept(*this);

    if (expr.ident->has_type()) {
        if (const auto* ident = dyn_cast<ast::Identifier>(expr.ident.get())) {
            const auto var = syms->get_var(ident->get_ident());
            if (!var) {
                diag.emit(expr.ident->get_span(),
                          DiagnosticKind::UndefinedIdentifier,
                          ident->to_string());
            }

            if (!var->is_array()) {
                diag.emit(expr.val->get_span(),
                          DiagnosticKind::TypeCannotBeIndexed,
                          expr.ident->node_type->basic_name());
            }
        } else {
            diag.emit(expr.val->get_span(), DiagnosticKind::TypeCannotBeIndexed,
                      expr.ident->node_type->basic_name());
        }
    }

    if (!expr.val->has_type())
        return;

    if (!expr.val->node_type->is_integral()) {
        diag.emit(expr.val->get_span(), DiagnosticKind::InvalidIndexType,
                  expr.val->node_type->basic_name());
    }
}

void SemChecker::visit(ast::FieldExpr& expr) {
    if (!expr.container->node_type->is_struct()) {
        diag.emit(expr.container->get_span(), DiagnosticKind::TypeHasNoFields,
                  expr.container->node_type->basic_name());
        return;
    }

    auto* struct_var =
        dyn_cast<type::StructType>(expr.container->node_type.get());
    if (struct_var == nullptr) {
        return;
    }

    if (!struct_var->get_field_type(expr.field->get_ident())) {
        diag.emit(expr.field->get_span(), DiagnosticKind::UnknownField,
                  expr.container->node_type->basic_name(),
                  expr.field->to_string());
    }
}

void SemChecker::visit(ast::ArrayInitExpr& expr) {
    for (auto& val : expr.vals) {
        val->accept(*this);
    }

    if (expr.vals.empty())
        return;

    const auto* valid_type = expr.vals.front()->node_type.get();
    if (valid_type == nullptr) {
        return;
    }

    for (auto& val : expr.vals) {
        if (val->has_type() &&
            !valid_type->is_assignment_compatible(val->node_type.get())) {
            diag.emit(val->get_span(), DiagnosticKind::TypeMismatch,
                      valid_type->basic_name(), val->node_type->basic_name());
        }
    }
}

void SemChecker::visit(ast::StructExprField& expr) {
    if (!expr.ident->node_type->is_assignment_compatible(
            expr.val->node_type.get())) {
        diag.emit(expr.get_span(), DiagnosticKind::TypeMismatch,
                  expr.ident->node_type->basic_name(),
                  expr.val->node_type->basic_name());
    }
}

void SemChecker::visit(ast::StructInitExpr& expr) {
    expr.ident->accept(*this);
    for (auto& field : expr.fields) {
        field->accept(*this);
    }

    if (const auto* ident = dyn_cast<ast::Identifier>(expr.ident.get())) {
        if (!ident->is_valid())
            return;

        const auto* struct_type = dyn_cast<type::StructType>(
            syms->get_type(ident->get_ident()).get());
        if (struct_type == nullptr) {
            diag.emit(expr.ident->get_span(), DiagnosticKind::NotAStruct,
                      ident->to_string(), expr.ident->node_type->basic_name());
        }

        std::unordered_set<std::string_view> required_fields;
        for (const auto& field : struct_type->get_fields()) {
            required_fields.insert(field.first);
        }

        for (const auto& field : expr.fields) {
            if (required_fields.erase(field->ident->get_ident()) == 0) {
                if (struct_type->has_field(field->ident->get_ident())) {
                    diag.emit(field->get_span(),
                              DiagnosticKind::DuplicateFieldInitialization,
                              field->ident->to_string());
                } else {
                    diag.emit(field->ident->get_span(),
                              DiagnosticKind::UnknownField, ident->to_string(),
                              field->ident->to_string());
                }
            }
        }

        if (!required_fields.empty()) {
            for (const auto& field : required_fields) {
                diag.emit(expr.get_span(), DiagnosticKind::FieldNotInitialized,
                          field);
            }
        }
    } else {
        diag.emit(expr.ident->get_span(), DiagnosticKind::TypeMismatch,
                  "identifier", expr.ident->node_type->basic_name());
    }
}

void SemChecker::visit(ast::TupleExpr& expr) {
    expr.first->accept(*this);
    expr.second->accept(*this);
}

void SemChecker::visit(ast::Block& block) {
    syms->enter_scope(block.get_scope_ctxt());
    for (auto& stmt : block.stmts) {
        stmt->accept(*this);
    }
    syms->exit_scope();
}

void SemChecker::visit(ast::Param& /*param*/) {}

void SemChecker::visit(ast::SourceFileDecl& file) {
    for (auto& decl : file.decls)
        decl->accept(*this);
}

void SemChecker::visit(ast::FuncDecl& func) {
    if (func.impl_type)
        func.impl_type->get()->accept(*this);

    /* for (auto& param : func.params) {
        param->accept(*this);
    } */

    func.body->accept(*this);

    if (!func.ret->is_assignment_compatible(func.body->node_type.get())) {
        diag.emit(func.body->get_span(), DiagnosticKind::ReturnTypeMismatch,
                  func.get_abs_name(), func.ret->basic_name(),
                  func.body->node_type->basic_name());
    }
}

void SemChecker::visit(ast::BreakStmt& stmt) {
    if (stmt.expr) {
        stmt.expr->accept(*this);
        if (stmt.expr->has_type() &&
            !syms->get_current_scope()->get_type()->is_assignment_compatible(
                stmt.expr->node_type.get())) {
            diag.emit(stmt.expr->get_span(), DiagnosticKind::TypeMismatch,
                      syms->get_current_scope()->get_type()->basic_name(),
                      stmt.expr->node_type->basic_name());
        }
    } else {
        if (!syms->get_current_scope()->get_type()->is_void()) {
            diag.emit(stmt.expr->get_span(), DiagnosticKind::TypeMismatch,
                      syms->get_current_scope()->get_type()->basic_name(),
                      "()");
        }
    }
}

void SemChecker::visit(ast::ContinueStmt& /*stmt*/) {}

void SemChecker::visit(ast::ForExpr& expr) {
    // expr.ident->accept(*this);
    expr.expr->accept(*this);
    if (expr.expr->has_type() && !expr.expr->node_type->is_iterable()) {
        diag.emit(expr.expr->get_span(), DiagnosticKind::TypeNotIterable,
                  expr.expr->node_type->basic_name());
    }

    expr.block->accept(*this);
}

void SemChecker::visit(ast::LetStmt& stmt) {
    // stmt.ident->accept(*this);
    if (stmt.val) {
        stmt.val->accept(*this);
    }

    if (stmt.type && stmt.val) {
        if (stmt.val->has_type() &&
            !stmt.type->is_assignment_compatible(stmt.val->node_type.get())) {
            diag.emit(stmt.val->get_span(), DiagnosticKind::InvalidAssignment,
                      stmt.val->node_type->basic_name(),
                      stmt.type->basic_name());
        }
    }
}

void SemChecker::visit(ast::ReturnStmt& stmt) {
    if (stmt.expr)
        stmt.expr->accept(*this);
}

void SemChecker::visit(ast::IfExpr& expr) {
    expr.expr->accept(*this);
    if (expr.expr->has_type() && !expr.expr->node_type->is_logical()) {
        diag.emit(expr.expr->get_span(), DiagnosticKind::TypeMismatch, "bool",
                  expr.expr->node_type->basic_name());
    }

    expr.block->accept(*this);

    if (expr.else_expr) {
        expr.else_expr->accept(*this);

        if (!expr.block->node_type->is_assignment_compatible(
                expr.else_expr->node_type.get())) {
            diag.emit(expr.else_expr->get_span(),
                      DiagnosticKind::ElseExprTypeMismatch,
                      expr.block->node_type->basic_name(),
                      expr.else_expr->node_type->basic_name());
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
        if (expr.expr->has_type() && !expr.expr->node_type->is_integral()) {
            diag.emit(expr.expr->get_span(), DiagnosticKind::ExpectedInteger,
                      expr.expr->node_type->basic_name());
        }
    }

    expr.block->accept(*this);
}

void SemChecker::visit(ast::WhileExpr& expr) {
    expr.expr->accept(*this);
    if (expr.expr->has_type() && !expr.expr->node_type->is_logical()) {
        diag.emit(expr.expr->get_span(), DiagnosticKind::TypeMismatch, "bool",
                  expr.expr->node_type->basic_name());
    }

    expr.block->accept(*this);
}

void SemChecker::visit(ast::StringExpr& /*expr*/) {}

void SemChecker::visit(ast::CharExpr& expr) {
    if (expr.get_span().len > 1) {
        diag.emit(expr.get_span(), DiagnosticKind::MoreThanOneChar);
    }
}

void SemChecker::visit(ast::StructField& /*field*/) {}

void SemChecker::visit(ast::StructDecl& /*decl*/) {}

void SemChecker::visit(ast::EnumField& /*field*/) {}

void SemChecker::visit(ast::EnumDecl& /*decl*/) {}

void SemChecker::visit(ast::ConstDecl& /*decl*/) {}

void SemChecker::visit(ast::StaticDecl& /*decl*/) {}

void SemChecker::check_expr_assignable(ast::Expr& expr) {
    if (!expr.is_assignable()) {
        diag.emit(expr.get_span(), DiagnosticKind::ExprNotAssignable);
    }
}
} // namespace z
