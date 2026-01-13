#include "constraint_gen.h"
#include "ast.h"
#include "token.h"
#include "type.h"
#include "type_arena.h"
#include <cstddef>
#include <memory>
#include <utility>

namespace z::type {
void ConstraintGenerator::visit(ast::Identifier& ident) {
    ident.node_type = new_var();

    if (const auto type = syms->get_var(ident.get_id())) {
        eq(ident.node_type, *type);
    }
}

void ConstraintGenerator::visit(ast::IntExpr& expr) {
    expr.node_type = new_type(InferType::IntLiteral);
}

void ConstraintGenerator::visit(ast::FloatExpr& expr) {
    expr.node_type = new_type(InferType::FloatLiteral);
}

void ConstraintGenerator::visit(ast::BoolExpr& expr) {
    expr.node_type = TypeArena::BOOL;
}

void ConstraintGenerator::visit(ast::PrefixExpr& expr) {
    expr.expr->accept(*this);

    expr.node_type = new_var();

    eq(expr.node_type, expr.expr->node_type);
}

void ConstraintGenerator::visit(ast::PostfixExpr& expr) {
    expr.expr->accept(*this);

    expr.node_type = new_var();

    eq(expr.node_type, expr.expr->node_type);
}

void ConstraintGenerator::visit(ast::BinaryExpr& expr) {
    expr.lhs->accept(*this);
    expr.rhs->accept(*this);

    expr.node_type = new_var();

    eq(expr.lhs->node_type, expr.rhs->node_type);

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
        expr.node_type = TypeArena::VOID;
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
        eq(expr.node_type, expr.lhs->node_type);
        break;
    default:
        std::unreachable();
    }
}

void ConstraintGenerator::visit(ast::TernaryExpr& expr) {
    expr.lhs->accept(*this);
    expr.mhs->accept(*this);
    expr.rhs->accept(*this);

    expr.node_type = new_var();

    eq(expr.mhs->node_type, expr.rhs->node_type);
    eq(expr.node_type, expr.mhs->node_type);
}

void ConstraintGenerator::visit(ast::CallExpr& expr) {
    for (auto& arg : expr.args)
        arg->accept(*this);

    expr.node_type = new_var();
    expr.ident->node_type = new_var();

    if (const auto* ident = dyn_cast<ast::Identifier>(expr.ident.get())) {
        auto func = syms->get_func(ident->get_id());
        if (!func)
            return;

        eq(expr.ident->node_type, *func);

        const auto* func_ptr = ty->get_as<type::FunctionType>(*func);
        if (!func_ptr)
            return;

        eq(expr.node_type, func_ptr->get_return_val());

        if (func_ptr->get_params().size() != expr.args.size())
            return;

        for (size_t i = 0; i < func_ptr->get_params().size(); i++) {
            eq(expr.args[i]->node_type, func_ptr->get_params()[i]);
        }
    }
}

void ConstraintGenerator::visit(ast::ArrayExpr& expr) {
    expr.ident->accept(*this);
    expr.val->accept(*this);

    expr.node_type = new_var();

    if (const auto* ident = dyn_cast<ast::Identifier>(expr.ident.get())) {
        if (auto arr = syms->get_var(ident->get_id())) {
            if (const auto* type = ty->get_as<ArrayType>(*arr))
                eq(expr.node_type, type->get_type());
        }
    }
}

void ConstraintGenerator::visit(ast::FieldExpr& expr) {
    expr.container->accept(*this);

    expr.field->node_type = new_var();
    expr.node_type = new_var();

    eq(expr.node_type, expr.field->node_type);

    auto* ident = dyn_cast<ast::Identifier>(expr.container.get());
    if (!ident)
        return;

    auto type = syms->get_type(ident->get_id());
    if (!type)
        return;

    auto* struct_type = ty->get_as<StructType>(*type);
    if (!struct_type)
        return;

    auto field = struct_type->get_field_type(expr.field->get_id());
    if (!field)
        return;

    eq(expr.field->node_type, *field);
}

void ConstraintGenerator::visit(ast::ArrayInitExpr& expr) {
    auto array_type = new_var();
    for (auto& val : expr.vals) {
        val->accept(*this);
        eq(val->node_type, array_type);
    }

    if (const auto expected = peek_expected()) {
        if (const auto* concrete = ty->get_as<ArrayType>(*expected)) {
            eq(array_type, concrete->get_type());
        }
    }

    expr.node_type = ty->make<ArrayType>(array_type, expr.vals.size());
}

void ConstraintGenerator::visit(ast::StructExprField& expr) {
    expr.ident->node_type = new_var();
    expr.val->node_type = new_var();
    eq(expr.ident->node_type, expr.val->node_type);
}

void ConstraintGenerator::visit(ast::StructInitExpr& expr) {
    expr.ident->node_type = new_var();
    expr.node_type = new_var();

    auto* ident = dyn_cast<ast::Identifier>(expr.ident.get());
    if (!ident)
        return;

    auto type = syms->get_type(ident->get_id());
    if (!type)
        return;

    eq(expr.node_type, *type);

    auto* struct_type = ty->get_as<StructType>(*type);
    if (!struct_type)
        return;

    eq(expr.ident->node_type, ty->make<TypeType>(*type));

    for (auto& field : expr.fields) {
        field->accept(*this);

        if (const auto field_type =
                struct_type->get_field_type(field->ident->get_id());
            field_type) {
            eq(field->ident->node_type, *field_type);
            eq(field->val->node_type, field->ident->node_type);
        }
    }
}

void ConstraintGenerator::visit(ast::TupleExpr& expr) {
    expr.first->accept(*this);
    expr.second->accept(*this);

    auto first_type = new_var();
    auto second_type = new_var();

    expr.node_type = ty->make<TupleType>(first_type, second_type);
}

void ConstraintGenerator::visit(ast::Block& block) {
    syms->enter_scope(block.get_scope_id());

    block.node_type = new_type(InferType::Block);
    push_block_type(block.node_type);

    for (auto& stmt : block.stmts)
        stmt->accept(*this);

    if (!block.stmts.empty() &&
        !isa<VoidType>(ty->get(block.stmts.back()->node_type))) {
        eq(block.node_type, block.stmts.back()->node_type);
    }

    pop_block_type();
    syms->exit_scope();
}

void ConstraintGenerator::visit(ast::Param& param) {
    resolve_type_name(param.type);
    param.name->node_type = param.type;
    param.node_type = param.type;
}

void ConstraintGenerator::visit(ast::SourceFileDecl& file) {
    for (auto& decl : file.decls)
        decl->accept(*this);
}

void ConstraintGenerator::visit(ast::FuncDecl& func) {
    auto& scope = syms->get_scope(func.body->get_scope_id());
    for (auto& param : func.params) {
        param->accept(*this);
        resolve_type_name(param->type);
        scope.declare_var(param->name, param->type);
    }

    func.body->accept(*this);

    resolve_type_name(func.ret);
    eq(func.body->node_type, func.ret);

    if (const auto f = syms->get_func(func.name->get_id())) {
        func.node_type = *f;
        func.name->node_type = *f;
    }
}

void ConstraintGenerator::visit(ast::BreakStmt& stmt) {
    if (stmt.expr)
        stmt.expr->accept(*this);

    stmt.node_type = ty->make<type::VoidType>();
}

void ConstraintGenerator::visit(ast::ContinueStmt& stmt) {
    stmt.node_type = ty->make<type::VoidType>();
}

void ConstraintGenerator::visit(ast::ForExpr& expr) {
    expr.ident->node_type = new_var();
    syms->get_scope(expr.block->get_scope_id())
        .declare_var(expr.ident, expr.ident->node_type);

    expr.block->accept(*this);
    expr.node_type = new_var();
    eq(expr.node_type, expr.block->node_type);

    expr.expr->accept(*this);
    // TODO: expr should be an iterator over some type T, ident should have type
    // T
}

void ConstraintGenerator::visit(ast::LetStmt& stmt) {
    stmt.ident->node_type = new_var();

    if (stmt.type.is_valid()) {
        resolve_type_name(stmt.type);
        eq(stmt.ident->node_type, stmt.type);
        push_expected(stmt.type);
    }

    if (stmt.val) {
        stmt.val->accept(*this);
        eq(stmt.ident->node_type, stmt.val->node_type);
        if (stmt.type.is_valid())
            pop_expected();
    }

    syms->declare_var(stmt.ident, stmt.ident->node_type);
    stmt.node_type = ty->make<type::VoidType>();
}

void ConstraintGenerator::visit(ast::ReturnStmt& stmt) {
    if (stmt.expr) {
        stmt.expr->accept(*this);
        eq(peek_block_type(), stmt.expr->node_type);
    } else {
        eq(peek_block_type(), TypeArena::VOID);
    }

    stmt.node_type = ty->make<type::VoidType>();
}

void ConstraintGenerator::visit(ast::IfExpr& expr) {
    expr.expr->accept(*this);
    expr.block->accept(*this);

    expr.node_type = new_var();
    eq(expr.node_type, expr.block->node_type);

    if (expr.else_expr) {
        expr.else_expr->accept(*this);
        eq(expr.block->node_type, expr.else_expr->node_type);
    }
}

void ConstraintGenerator::visit(ast::ElseExpr& expr) {
    expr.node_type = new_var();

    if (expr.if_expr) {
        expr.if_expr->accept(*this);
        eq(expr.node_type, expr.if_expr->node_type);
    } else if (expr.block) {
        expr.block->accept(*this);
        eq(expr.node_type, expr.block->node_type);
    }
}

void ConstraintGenerator::visit(ast::LoopExpr& expr) {
    if (expr.expr)
        expr.expr->accept(*this);

    expr.block->accept(*this);
    expr.node_type = new_var();
    eq(expr.node_type, expr.block->node_type);
}

void ConstraintGenerator::visit(ast::WhileExpr& expr) {
    expr.expr->accept(*this);

    expr.block->accept(*this);
    expr.node_type = new_var();
    eq(expr.node_type, expr.block->node_type);
}

void ConstraintGenerator::visit(ast::StringExpr& expr) {
    expr.node_type = TypeArena::STR;
}

void ConstraintGenerator::visit(ast::CharExpr& expr) {
    expr.node_type = TypeArena::CHAR;
}

void ConstraintGenerator::visit(ast::StructField& field) {
    resolve_type_name(field.type);

    field.ident->node_type = field.type;
    field.node_type = field.type;
}

void ConstraintGenerator::visit(ast::StructDecl& decl) {
    const auto t = syms->get_type(decl.ident->get_id());
    if (!t)
        return;

    decl.node_type = *t;
    decl.ident->node_type = ty->make<TypeType>(decl.node_type);

    const auto* struct_type = ty->get_as<StructType>(decl.node_type);

    for (auto& field : decl.fields) {
        field->accept(*this);
    }

    for (auto& func : decl.funcs) {
        func->accept(*this);
        auto* func_decl = cast<ast::FuncDecl>(func.get());
        if (const auto t =
                struct_type->get_func_type(func_decl->name->get_id())) {
            func->node_type = *t;
            func_decl->name->node_type = func->node_type;
        }
    }
}

void ConstraintGenerator::visit(ast::EnumField& field) {
    for (auto& type : field.types) {
        resolve_type_name(type);
    }
}

void ConstraintGenerator::visit(ast::EnumDecl& decl) {
    if (const auto t = syms->get_type(decl.ident->get_id()); t) {
        decl.node_type = *t;
        decl.ident->node_type = ty->make<TypeType>(decl.node_type);
    }

    for (auto& field : decl.fields) {
        field->accept(*this);
    }
}

void ConstraintGenerator::visit(ast::ConstDecl& decl) {
    resolve_type_name(decl.type);
    decl.ident->node_type = decl.type;
    decl.node_type = TypeArena::VOID;

    decl.val->accept(*this);
    eq(decl.val->node_type, decl.type);
}

void ConstraintGenerator::visit(ast::StaticDecl& decl) {
    resolve_type_name(decl.type);
    decl.ident->node_type = decl.type;
    decl.node_type = TypeArena::VOID;

    decl.val->accept(*this);
    eq(decl.val->node_type, decl.type);
}

void ConstraintGenerator::visit(ast::TraitDecl& decl) {
    syms->enter_scope(decl.scope);

    for (const auto& c : decl.consts)
        c->accept(*this);

    for (const auto& type : decl.types)
        type->accept(*this);

    for (const auto& func : decl.funcs)
        func->accept(*this);

    syms->exit_scope();

    decl.node_type = TypeArena::VOID;
    decl.ident->node_type = ty->make<TraitType>(decl.ident->get_id());
}
void ConstraintGenerator::visit(ast::TypeAliasDecl& decl) {
    resolve_type_name(decl.type);
    decl.ident->node_type = decl.type;
    decl.node_type = TypeArena::VOID;
}
void ConstraintGenerator::visit(ast::TraitFuncDecl& decl) {
    for (auto& param : decl.params) {
        param->accept(*this);
        resolve_type_name(param->type);
        if (decl.body)
            syms->get_scope(decl.body->get_scope_id())
                .declare_var(param->name, param->type);
    }

    resolve_type_name(decl.ret);

    if (decl.body) {
        decl.body->accept(*this);
        eq(decl.body->node_type, decl.ret);
    }

    if (const auto t = syms->get_func(decl.name->get_id())) {
        decl.node_type = *t;
        decl.name->node_type = *t;
    }
}

void ConstraintGenerator::resolve_type_name(TypeRef& type) {
    if (auto* unk = ty->get_as<UnknownType>(type)) {
        auto t = syms->get_type(unk->get_id());
        if (t) {
            type = *t;
        } else {
            type = TypeArena::INVALID;
        }
    }
}

} // namespace z::type
