#include "type_res.h"
#include "ast.h"
#include "inf_ctxt.h"
#include "token.h"
#include "type.h"
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace z {

// NOLINTBEGIN(bugprone-unchecked-optional-access)

void TypeResolver::fill_top_level_syms(
    const std::vector<std::unique_ptr<ast::Decl>>& decls) const {
    for (const auto& decl : decls) {
        decl->declare_type(syms);
    }

    for (const auto& decl : decls) {
        decl->resolve_sym(syms);
    }
}

void TypeResolver::visit(ast::Identifier& ident) {
    if (const auto type = syms->get_var(ident.get_ident())) {
        ident.node_type = type;
    }
}

void TypeResolver::visit(ast::IntExpr& expr) {
    expr.node_type = infctxt->new_type(type::InferType::IntLiteral);
}

void TypeResolver::visit(ast::FloatExpr& expr) {
    expr.node_type = infctxt->new_type(type::InferType::FloatLiteral);
}

void TypeResolver::visit(ast::BoolExpr& expr) {
    expr.node_type = std::make_shared<type::BooleanType>();
}

void TypeResolver::visit(ast::PrefixExpr& expr) {
    expr.expr->accept(*this);
    if (!expr.expr->has_type()) {
        return;
    }

    if (expr.expr->node_type->is_explicit()) {
        expr.node_type = expr.expr->node_type;
    } else {
        expr.node_type = infctxt->new_type(type::InferType::Var);
        infctxt->eq(expr.node_type, expr.expr->node_type);
    }
}

void TypeResolver::visit(ast::PostfixExpr& expr) {
    expr.expr->accept(*this);
    if (!expr.expr->has_type()) {
        return;
    }

    if (expr.expr->node_type->is_explicit()) {
        expr.node_type = expr.expr->node_type;
    } else {
        expr.node_type = infctxt->new_type(type::InferType::Var);
        infctxt->eq(expr.node_type, expr.expr->node_type);
    }
}

void TypeResolver::visit(ast::BinaryExpr& expr) {
    if (expr.op.is(TokenKind::ColonColon)) {
        visit_method_call(expr);
        return;
    }

    expr.lhs->accept(*this);
    expr.rhs->accept(*this);
    if (!expr.lhs->has_type() || !expr.rhs->has_type()) {
        return;
    }

    auto* l_type = expr.lhs->node_type.get();
    auto* r_type = expr.rhs->node_type.get();

    switch (expr.op.get_kind()) {
    case TokenKind::PlusEq:
    case TokenKind::MinusEq:
    case TokenKind::StarEq:
    case TokenKind::SlashEq:
    case TokenKind::PercentEq:
    case TokenKind::CaretEq:
    case TokenKind::AndEq:
    case TokenKind::OrEq:
    case TokenKind::ShlEq:
    case TokenKind::ShrEq:
    case TokenKind::Eq:
        if (!l_type->is_explicit() || !r_type->is_explicit()) {
            if (!infctxt->eq(expr.lhs->node_type, expr.rhs->node_type)) {
                return;
            }
        }
        expr.node_type = std::make_shared<type::VoidType>();
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
    case TokenKind::Range:
    case TokenKind::RangeEq:
    case TokenKind::Or:
    case TokenKind::Caret:
    case TokenKind::And:
    case TokenKind::Shl:
    case TokenKind::Shr:
    case TokenKind::Percent:
    case TokenKind::Plus:
    case TokenKind::Minus:
    case TokenKind::Star:
    case TokenKind::Slash:

        if (!l_type->is_explicit() || !r_type->is_explicit()) {
            if (!infctxt->eq(expr.lhs->node_type, expr.rhs->node_type)) {
                return;
            }

            expr.node_type = infctxt->new_type(type::InferType::Var);
            infctxt->eq(expr.node_type, expr.lhs->node_type);
            infctxt->eq(expr.node_type, expr.rhs->node_type);
        } else {
            expr.node_type = expr.lhs->node_type;
        }

        break;
    default:
        std::unreachable();
    }
}

void TypeResolver::visit(ast::TernaryExpr& expr) {
    expr.lhs->accept(*this);
    expr.mhs->accept(*this);
    expr.rhs->accept(*this);

    if (!expr.lhs->has_type() || !expr.mhs->has_type() ||
        !expr.rhs->has_type()) {
        return;
    }

    if (!expr.mhs->node_type->is_explicit() ||
        !expr.rhs->node_type->is_explicit()) {
        if (!infctxt->eq(expr.mhs->node_type, expr.rhs->node_type)) {
            return;
        }

        expr.node_type = infctxt->new_type(type::InferType::Var);
        infctxt->eq(expr.node_type, expr.mhs->node_type);
        infctxt->eq(expr.node_type, expr.rhs->node_type);
    } else {
        expr.node_type = expr.mhs->node_type;
    }
}

void TypeResolver::visit(ast::CallExpr& expr) {
    for (auto& arg : expr.args) {
        arg->accept(*this);
    }

    if (const auto* ident = dyn_cast<ast::Identifier>(expr.ident.get())) {
        auto func = syms->get_func(ident->to_string());
        const auto* func_ptr = dyn_cast<type::FunctionType>(func.get());
        if (func_ptr == nullptr) {
            return;
        }
        expr.node_type = func_ptr->get_return_val();
    } else {
        expr.ident->accept(*this);
    }
}

void TypeResolver::visit_method_call(ast::BinaryExpr& expr) {
    const auto* impl_type = cast<ast::Identifier>(expr.lhs.get());
    expr.lhs->node_type = syms->get_type(impl_type->get_ident());

    auto* func_call = cast<ast::CallExpr>(expr.rhs.get());
    for (auto& arg : func_call->args) {
        arg->accept(*this);
    }

    const std::string name =
        impl_type->to_string() +
        "::" + cast<ast::Identifier>(func_call->ident.get())->to_string();
    auto func = syms->get_func(name);
    const auto* func_ptr = cast<type::FunctionType>(func.get());
    func_call->node_type = func_ptr->get_return_val();
    func_call->ident->node_type = func;
    expr.node_type = func_call->node_type;
}

void TypeResolver::visit(ast::ArrayExpr& expr) {
    expr.val->accept(*this);

    if (const auto* ident = dyn_cast<ast::Identifier>(expr.ident.get())) {
        auto arr = syms->get_var(ident->get_ident());
        expr.ident->node_type = arr;

        if (const auto* type = dyn_cast<type::ArrayType>(arr.get())) {
            expr.node_type = type->get_type();
        }
    } else {
        expr.ident->accept(*this);
    }
}

void TypeResolver::visit(ast::FieldExpr& expr) {
    expr.container->accept(*this);
    if (expr.container->has_type())
        resolve(expr.container->node_type);

    auto* struct_var =
        dyn_cast<type::StructType>(expr.container->node_type.get());
    if (struct_var == nullptr) {
        return;
    }

    expr.node_type = struct_var->get_field_type(expr.field->get_ident());
    expr.field->node_type = expr.node_type;
}

void TypeResolver::visit(ast::ArrayInitExpr& expr) {
    auto internal_type = infctxt->new_type(type::InferType::Var);
    for (auto& val : expr.vals) {
        val->accept(*this);
        if (!val->has_type()) {
            return;
        }

        if (!infctxt->eq(internal_type, val->node_type))
            return;
    }
    expr.node_type =
        std::make_shared<type::ArrayType>(std::move(internal_type));
}

void TypeResolver::visit(ast::StructExprField& expr) {
    expr.val->accept(*this);

    expr.ident->node_type = infctxt->new_type(type::InferType::Var);
    expr.node_type = infctxt->new_type(type::InferType::Var);

    infctxt->eq(expr.ident->node_type, expr.node_type);
}

void TypeResolver::visit(ast::StructInitExpr& expr) {
    for (auto& field : expr.fields) {
        field->accept(*this);
    }

    if (auto* ident = dyn_cast<ast::Identifier>(expr.ident.get())) {
        expr.node_type = syms->get_type(ident->get_ident());
        expr.ident->node_type = expr.node_type;

        auto* struct_type = dyn_cast<type::StructType>(expr.node_type.get());
        if (struct_type == nullptr)
            return;

        for (auto& field : expr.fields) {
            const auto* ident = dyn_cast<ast::Identifier>(field->ident.get());
            if (ident != nullptr) {
                const auto field_type =
                    struct_type->get_field_type(ident->get_ident());
                if (field_type) {
                    infctxt->eq(field_type, field->ident->node_type);
                    infctxt->eq(field->ident->node_type, field->val->node_type);
                }
            }
        }
    } else {
        expr.ident->accept(*this);
    }
}

void TypeResolver::visit(ast::TupleExpr& expr) {
    expr.first->accept(*this);
    expr.second->accept(*this);

    expr.node_type = std::make_unique<type::TupleType>(expr.first->node_type,
                                                       expr.second->node_type);
}

void TypeResolver::visit(ast::Block& block) {
    syms->enter_scope(block.get_scope_ctxt());
    auto scope_type = infctxt->new_type(type::InferType::Block);
    syms->get_current_scope()->set_type(scope_type);

    block.node_type = infctxt->new_type(type::InferType::Block);
    infctxt->eq(scope_type, block.node_type);

    for (auto& stmt : block.stmts) {
        stmt->accept(*this);
    }

    if (!block.stmts.empty() && block.stmts.back()->has_type() &&
        (dyn_cast<type::VoidType>(block.stmts.back()->node_type.get()) ==
         nullptr)) {
        infctxt->eq(scope_type, block.stmts.back()->node_type);
    }

    syms->exit_scope();
}

void TypeResolver::visit(ast::Param& param) {
    syms->resolve_unk_type(param.type);
    param.node_type = param.type;
}

void TypeResolver::visit(ast::FuncDecl& func) {
    if (func.impl_type) {
        func.impl_type->get()->node_type =
            syms->get_type(func.impl_type->get()->get_ident());
    }

    auto* body = cast<ast::Block>(func.body.get());

    for (auto& param : func.params) {
        param->accept(*this);
        body->get_scope_ctxt()->declare_var(param->name, param->type);
    }

    func.body->accept(*this);

    if (!func.body->has_type()) {
        resolve(&func);
        return;
    }

    infctxt->eq(func.body->node_type, func.ret);

    func.node_type = syms->get_func(func.get_abs_name());
    func.name->node_type = syms->get_func(func.get_abs_name());

    resolve(&func);

    infctxt.emplace();
}

void TypeResolver::visit(ast::BreakStmt& stmt) {
    if (stmt.expr) {
        stmt.expr->accept(*this);
    }

    stmt.node_type = std::make_shared<type::VoidType>();
}

void TypeResolver::visit(ast::ContinueStmt& stmt) {
    stmt.node_type = std::make_shared<type::VoidType>();
}

void TypeResolver::visit(ast::ForExpr& expr) {
    expr.ident->node_type = infctxt->new_type(type::InferType::Var);
    expr.block->get_scope_ctxt()->declare_var(expr.ident,
                                              expr.ident->node_type);

    expr.block->accept(*this);
    expr.node_type = infctxt->new_type(type::InferType::Var);
    infctxt->eq(expr.node_type, expr.block->node_type);

    expr.expr->accept(*this);
    if (!expr.expr->has_type()) {
        return;
    }

    if (expr.expr->node_type->is_explicit()) {
        expr.ident->node_type = expr.expr->node_type;
    } else {
        expr.ident->node_type = infctxt->new_type(type::InferType::Var);
        infctxt->eq(expr.ident->node_type, expr.expr->node_type);
    }
}

void TypeResolver::visit(ast::LetStmt& stmt) {
    if (stmt.val) {
        stmt.val->accept(*this);

        if (!stmt.val->has_type()) {
            syms->declare_var(stmt.ident,
                              std::make_shared<type::InvalidType>());
            return;
        }
    }

    if (stmt.type) {
        if (stmt.val && !stmt.val->node_type->is_explicit() &&
            !infctxt->eq(stmt.type, stmt.val->node_type)) {
            return;
        }

        if (stmt.type->is_unknown()) {
            resolve(stmt.type);
        }
        stmt.ident->node_type = stmt.type;
    } else if (stmt.val) {
        auto var_type = infctxt->new_type(type::InferType::Var);
        infctxt->eq(var_type, stmt.val->node_type);
        stmt.ident->node_type = var_type;
    } else {
        stmt.ident->node_type = infctxt->new_type(type::InferType::Var);
    }

    syms->declare_var(stmt.ident, stmt.ident->node_type);
    stmt.node_type = std::make_shared<type::VoidType>();
}

void TypeResolver::visit(ast::ReturnStmt& stmt) {
    auto& current_scope_type = syms->get_current_scope()->get_type();

    if (stmt.expr) {
        stmt.expr->accept(*this);
        if (!stmt.expr->has_type()) {
            return;
        }

        infctxt->eq(current_scope_type, stmt.expr->node_type);
    } else {
        infctxt->eq(current_scope_type, std::make_shared<type::VoidType>());
    }

    stmt.node_type = std::make_shared<type::VoidType>();
}

void TypeResolver::visit(ast::IfExpr& expr) {
    expr.expr->accept(*this);

    expr.block->accept(*this);

    expr.node_type = infctxt->new_type(type::InferType::Var);
    infctxt->eq(expr.node_type, expr.block->node_type);

    if (expr.else_expr) {
        expr.else_expr->accept(*this);
        if (expr.else_expr->has_type()) {
            infctxt->eq(expr.node_type, expr.else_expr->node_type);
        }
    }
}

void TypeResolver::visit(ast::ElseExpr& expr) {
    if (expr.if_expr) {
        expr.if_expr->accept(*this);
        if (expr.if_expr->has_type()) {
            expr.node_type = infctxt->new_type(type::InferType::Var);
            infctxt->eq(expr.node_type, expr.if_expr->node_type);
        }
    } else if (expr.block) {
        expr.block->accept(*this);
        expr.node_type = infctxt->new_type(type::InferType::Var);
        infctxt->eq(expr.node_type, expr.block->node_type);
    }
}

void TypeResolver::visit(ast::LoopExpr& expr) {
    if (expr.expr) {
        expr.expr->accept(*this);
    }

    expr.block->accept(*this);
    expr.node_type = infctxt->new_type(type::InferType::Var);
    infctxt->eq(expr.node_type, expr.block->node_type);
}

void TypeResolver::visit(ast::WhileExpr& expr) {
    expr.expr->accept(*this);

    expr.block->accept(*this);
    expr.node_type = infctxt->new_type(type::InferType::Var);
    infctxt->eq(expr.node_type, expr.block->node_type);
}

void TypeResolver::visit(ast::StringExpr& expr) {
    expr.node_type = std::make_unique<type::StringType>();
}

void TypeResolver::visit(ast::CharExpr& expr) {
    expr.node_type = std::make_unique<type::CharType>();
}

void TypeResolver::visit(ast::StructField& /* field */) {}

void TypeResolver::visit(ast::StructDecl& decl) {
    decl.node_type = syms->get_type(decl.ident->get_ident());
    decl.ident->node_type = decl.node_type;

    const auto* struct_type = cast<type::StructType>(decl.node_type.get());
    for (auto& field : decl.fields) {
        field->ident->node_type =
            struct_type->get_field_type(field->ident->get_ident());
        field->node_type = field->ident->node_type;
    }
}

void TypeResolver::visit(ast::EnumField& /* field*/) {}

void TypeResolver::visit(ast::EnumDecl& decl) {
    decl.node_type = syms->get_type(decl.ident->get_ident());
    decl.ident->node_type = syms->get_type(decl.ident->get_ident());

    for (auto& field : decl.fields) {
        field->node_type =
            std::make_shared<type::EnumVariantType>(decl.ident->to_string());
    }
}

void TypeResolver::visit(ast::ConstDecl& decl) {
    decl.ident->node_type = syms->get_global_var(decl.ident->get_ident());

    decl.val->accept(*this);
    if (decl.val->has_type()) {
        infctxt->eq(decl.val->node_type, decl.type);
    }

    decl.node_type = std::make_shared<type::VoidType>();

    resolve(&decl);
    infctxt.emplace();
}

void TypeResolver::visit(ast::StaticDecl& decl) {
    decl.ident->node_type = decl.type;

    decl.val->accept(*this);
    if (decl.val->has_type()) {
        infctxt->eq(decl.val->node_type, decl.type);
    }

    decl.node_type = std::make_shared<type::VoidType>();

    resolve(&decl);
    infctxt.emplace();
}

void TypeResolver::resolve(std::shared_ptr<type::Type>& type) {
    type = infctxt->try_resolve(type);
    if (!type->is_explicit()) {
        const auto* infer = get_inf_type(type.get());
        if (infer->get_infer_type() == type::InferType::IntLiteral) {
            type = std::make_shared<type::IntegerType>(32, true);
        } else if (infer->get_infer_type() == type::InferType::FloatLiteral) {
            type = std::make_shared<type::FloatType>(64);
        } else if (infer->get_infer_type() == type::InferType::Block) {
            type = std::make_shared<type::VoidType>();
        } else {
            std::cerr << "UNK TYPE\n";
        }
    } else if (type->is_unknown()) {
        type = syms->get_type(
            cast<type::UnknownType>(type.get())->get_ident()->get_ident());
    }
}

void TypeResolver::resolve(ast::ASTNode* node) {
    if (node == nullptr)
        return;

    ResolutionVisitor res(*this);
    node->accept(res);
}

// NOLINTEND(bugprone-unchecked-optional-access)
} // namespace z
