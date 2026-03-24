#include "constraint_gen.h"
#include "core/panic.h"
#include "parser/ast.h"
#include "type.h"
#include "type_arena.h"
#include "type_ref.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

namespace z::type {
void ConstraintGenerator::visit(ast::Identifier& ident) {
    if (const auto type = syms->get_var(ident.get_id()))
        ident.set_type(*type);
}

void ConstraintGenerator::visit(ast::IntExpr& expr) {
    expr.set_type(new_type(InferType::IntLiteral));
}

void ConstraintGenerator::visit(ast::FloatExpr& expr) {
    expr.set_type(new_type(InferType::FloatLiteral));
}

void ConstraintGenerator::visit(ast::BoolExpr& expr) {
    expr.set_type(builtin::BOOL);
}

void ConstraintGenerator::visit(ast::PrefixExpr& expr) {
    expr.expr->accept(*this);

    expr.set_type(expr.expr->get_type());
}

void ConstraintGenerator::visit(ast::PostfixExpr& expr) {
    expr.expr->accept(*this);

    expr.set_type(expr.expr->get_type());
}

void ConstraintGenerator::visit(ast::BinaryExpr& expr) {
    expr.lhs->accept(*this);
    expr.rhs->accept(*this);

    using ast::BinOp;
    switch (expr.op) {
    case BinOp::AddEq:
    case BinOp::SubEq:
    case BinOp::MulEq:
    case BinOp::DivEq:
    case BinOp::ModEq:
    case BinOp::BitXorEq:
    case BinOp::BitAndEq:
    case BinOp::BitOrEq:
    case BinOp::ShlEq:
    case BinOp::ShrEq:
    case BinOp::Eq:
        eq(expr.lhs->get_type(), expr.rhs->get_type());
        expr.set_type(builtin::VOID);
        return;
    case BinOp::Range:
    case BinOp::RangeEq:
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
        expr.set_type(new_var());
        eq(expr.get_type(), expr.lhs->get_type());
        eq(expr.get_type(), expr.rhs->get_type());
        return;
    case BinOp::ColonColon:
        expr.set_type(new_var());
        eq(expr.get_type(), expr.rhs->get_type());
        return;
    case BinOp::LogicOr:
    case BinOp::LogicAnd:
    case BinOp::EqEq:
    case BinOp::Ne:
    case BinOp::Gt:
    case BinOp::Lt:
    case BinOp::Ge:
    case BinOp::Le:
        eq(expr.lhs->get_type(), expr.rhs->get_type());
        expr.set_type(builtin::BOOL);
        return;
    }

    std::unreachable();
}

void ConstraintGenerator::visit(ast::CallExpr& expr) {
    for (auto& arg : expr.args)
        arg->accept(*this);

    if (const auto* ident = dyn_cast<ast::Identifier>(expr.ident.get())) {
        auto func = syms->get_func(ident->get_id());
        if (!func)
            return;

        expr.ident->set_type(*func);

        const auto* func_ptr = ty->get_as<FunctionType>(*func);
        if (!func_ptr)
            return;

        expr.set_type(func_ptr->get_return_val());

        if (func_ptr->get_params().size() != expr.args.size())
            return;

        for (std::size_t i = 0; i < func_ptr->get_params().size(); i++)
            eq(expr.args[i]->get_type(), func_ptr->get_params()[i]);
    }
}

void ConstraintGenerator::visit(ast::ArrayExpr& expr) {
    expr.array->accept(*this);
    expr.val->accept(*this);

    if (const auto* type = ty->get_as<ArrayType>(expr.array->get_type()))
        expr.set_type(type->get_type());
}

void ConstraintGenerator::visit(ast::FieldExpr& expr) {
    expr.container->accept(*this);

    auto* struct_type = ty->get_as<StructType>(expr.container->get_type());
    if (!struct_type)
        return;

    auto field = struct_type->get_field_type(expr.field->get_id());
    if (!field)
        return;

    expr.field->set_type(*field);
    expr.set_type(*field);
}

void ConstraintGenerator::visit(ast::ArrayInitExpr& expr) {
    auto array_type = new_var();
    if (const auto expected = peek_expected()) {
        if (const auto* concrete = ty->get_as<ArrayType>(*expected)) {
            eq(array_type, concrete->get_type());
        }
    }

    for (auto& val : expr.vals) {
        val->accept(*this);
        eq(array_type, val->get_type());
    }

    expr.set_type(ty->make<ArrayType>(array_type, expr.vals.size()));
}

void ConstraintGenerator::visit(ast::StructExprField& expr) {
    expr.val->accept(*this);
}

void ConstraintGenerator::visit(ast::StructInitExpr& expr) {
    auto* ident = dyn_cast<ast::Identifier>(expr.ident.get());
    if (!ident)
        return;

    auto type = syms->get_type(ident->get_id());
    if (!type)
        return;

    expr.set_type(*type);
    expr.ident->set_type(ty->make<TypeType>(*type));

    auto* struct_type = ty->get_as<StructType>(*type);
    if (!struct_type)
        return;

    for (auto& field : expr.fields) {
        field->accept(*this);

        if (const auto field_type =
                struct_type->get_field_type(field->ident->get_id());
            field_type) {
            field->ident->set_type(*field_type);
            eq(field->val->get_type(), *field_type);
        }
    }
}

void ConstraintGenerator::visit(ast::TupleExpr& expr) {
    expr.first->accept(*this);
    expr.second->accept(*this);

    expr.set_type(
        ty->make<TupleType>(expr.first->get_type(), expr.second->get_type()));
}

void ConstraintGenerator::visit(ast::Block& block) {
    syms->enter_scope(block.get_scope_id());

    if (!block.get_type().is_initialized())
        block.set_type(new_type(InferType::Block));

    for (auto& stmt : block.stmts)
        stmt->accept(*this);

    if (!block.stmts.empty()) {
        if (!isa<VoidType>(ty->get(block.stmts.back()->get_type())) &&
            !block.stmts.back()->semi_terminated)
            eq(block.get_type(), block.stmts.back()->get_type());
        else
            eq(block.get_type(), builtin::VOID);
    }

    syms->exit_scope();
}

void ConstraintGenerator::visit(ast::Param& param) {
    param.name->set_type(param.type);
    param.set_type(param.type);
}

void ConstraintGenerator::visit(ast::SourceFileDecl& file) {
    for (auto& decl : file.const_decls)
        decl->accept(*this);

    for (auto& decl : file.decls)
        decl->accept(*this);
}

void ConstraintGenerator::visit(ast::FuncDecl& func) {
    auto& scope = syms->get_scope(func.body->get_scope_id());
    for (auto& param : func.params) {
        param->accept(*this);
        scope.declare_var(param->name, param->type, false, true);
    }

    func.body->set_type(new_type(InferType::Block));
    func_type = func.body->get_type();
    func.body->accept(*this);
    func_type = std::nullopt;

    eq(func.body->get_type(), func.ret);

    if (const auto f = syms->get_func(func.name->get_id())) {
        func.set_type(*f);
        func.name->set_type(*f);
    }
}

void ConstraintGenerator::visit(ast::BreakStmt& stmt) {
    TypeRef break_type = builtin::VOID;
    if (stmt.expr) {
        stmt.expr->accept(*this);
        break_type = stmt.expr->get_type();
    }

    if (auto loop_result = peek_loop_result())
        eq(break_type, *loop_result);

    stmt.set_type(builtin::VOID);
}

void ConstraintGenerator::visit(ast::ContinueStmt& stmt) {
    stmt.set_type(builtin::VOID);
}

void ConstraintGenerator::visit(ast::ForExpr& expr) {
    expr.ident->set_type(new_var());
    syms->get_scope(expr.block->get_scope_id())
        .declare_var(expr.ident, expr.ident->get_type(), false, true);

    expr.expr->accept(*this);
    // TODO: expr should be an iterator over some type T, ident should have type
    // T

    push_loop_result(builtin::VOID);
    expr.block->accept(*this);
    pop_loop_result();

    expr.set_type(builtin::VOID);
}

void ConstraintGenerator::visit(ast::LetStmt& stmt) {
    if (stmt.type.is_valid()) {
        resolve_type_name(stmt.type);
        push_expected(stmt.type);
    }

    if (stmt.val) {
        stmt.val->accept(*this);
    }

    if (stmt.type.is_valid()) {
        stmt.ident->set_type(stmt.type);
        if (stmt.val) {
            eq(stmt.ident->get_type(), stmt.val->get_type());
        }
    } else if (stmt.val) {
        stmt.ident->set_type(stmt.val->get_type());
    } else {
        stmt.ident->set_type(new_var());
    }

    if (stmt.type.is_valid())
        pop_expected();

    syms->declare_var(stmt.ident, stmt.ident->get_type(), false,
                      stmt.val != nullptr);
    stmt.set_type(builtin::VOID);
}

void ConstraintGenerator::visit(ast::ReturnStmt& stmt) {
    expect(func_type.has_value(), "Return outside function");
    if (stmt.expr) {
        stmt.expr->accept(*this);
        eq(func_type.value(), stmt.expr->get_type());
    } else {
        eq(func_type.value(), builtin::VOID);
    }

    stmt.set_type(builtin::VOID);
}

void ConstraintGenerator::visit(ast::IfExpr& expr) {
    expr.expr->accept(*this);
    eq(expr.expr->get_type(), builtin::BOOL);

    expr.block->accept(*this);

    if (expr.else_expr) {
        expr.else_expr->accept(*this);

        expr.set_type(new_var());
        eq(expr.get_type(), expr.block->get_type());
        eq(expr.block->get_type(), expr.else_expr->get_type());
    } else {
        expr.set_type(expr.block->get_type());
    }
}

void ConstraintGenerator::visit(ast::ElseExpr& expr) {
    if (expr.if_expr) {
        expr.if_expr->accept(*this);
        expr.set_type(expr.if_expr->get_type());
    } else if (expr.block) {
        expr.block->accept(*this);
        expr.set_type(expr.block->get_type());
    }
}

void ConstraintGenerator::visit(ast::LoopExpr& expr) {
    if (expr.expr)
        expr.expr->accept(*this);

    auto loop_result = new_var();
    push_loop_result(loop_result);

    expr.block->accept(*this);

    pop_loop_result();

    expr.set_type(loop_result);
}

void ConstraintGenerator::visit(ast::WhileExpr& expr) {
    expr.expr->accept(*this);

    push_loop_result(builtin::VOID);
    expr.block->accept(*this);
    pop_loop_result();

    expr.set_type(builtin::VOID);
}

void ConstraintGenerator::visit(ast::StringExpr& expr) {
    expr.set_type(builtin::STR);
}

void ConstraintGenerator::visit(ast::CharExpr& expr) {
    expr.set_type(builtin::CHAR);
}

void ConstraintGenerator::visit(ast::StructField& field) {
    field.ident->set_type(field.type);
    field.set_type(field.type);
}

void ConstraintGenerator::visit(ast::StructDecl& decl) {
    for (auto& field : decl.fields) {
        field->accept(*this);
    }

    const auto t = syms->get_type(decl.ident->get_id());
    if (!t)
        return;

    decl.set_type(*t);
    decl.ident->set_type(ty->make<TypeType>(*t));

    const auto* struct_type = ty->get_as<StructType>(decl.get_type());

    syms->enter_scope(decl.scope);
    for (auto& func : decl.funcs) {
        func->accept(*this);
        auto* func_decl = cast<ast::FuncDecl>(func.get());
        if (const auto t =
                struct_type->get_func_type(func_decl->name->get_id())) {
            func->set_type(*t);
            func_decl->name->set_type(func->get_type());
        }
    }
    syms->exit_scope();
}

void ConstraintGenerator::visit(ast::EnumField& /*field*/) {}

void ConstraintGenerator::visit(ast::EnumDecl& decl) {
    if (const auto t = syms->get_type(decl.ident->get_id()); t) {
        decl.set_type(*t);
        decl.ident->set_type(ty->make<TypeType>(decl.get_type()));
    }

    for (auto& field : decl.fields) {
        field->accept(*this);
        field->set_type(decl.get_type());
    }
}

void ConstraintGenerator::visit(ast::ConstDecl& decl) {
    decl.ident->set_type(decl.type);
    decl.set_type(builtin::VOID);

    decl.val->accept(*this);
    eq(decl.val->get_type(), decl.type);
}

void ConstraintGenerator::visit(ast::StaticDecl& decl) {
    decl.ident->set_type(decl.type);
    decl.set_type(builtin::VOID);

    decl.val->accept(*this);
    eq(decl.val->get_type(), decl.type);
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

    decl.set_type(builtin::VOID);
    decl.ident->set_type(ty->make<TraitType>(decl.ident->get_id()));
}
void ConstraintGenerator::visit(ast::TypeAliasDecl& decl) {
    decl.ident->set_type(decl.type);
    decl.set_type(builtin::VOID);
}
void ConstraintGenerator::visit(ast::TraitFuncDecl& decl) {
    for (auto& param : decl.params) {
        param->accept(*this);
        if (decl.body)
            syms->get_scope(decl.body->get_scope_id())
                .declare_var(param->name, param->type, false, true);
    }

    if (decl.body) {
        decl.body->accept(*this);
        eq(decl.body->get_type(), decl.ret);
    }

    if (const auto t = syms->get_func(decl.name->get_id())) {
        decl.set_type(*t);
        decl.name->set_type(*t);
    }
}

void ConstraintGenerator::resolve_type_name(TypeRef& type) {
    if (auto* unk = ty->get_as<UnknownType>(type)) {
        auto t = syms->get_type(unk->get_id());
        if (t) {
            type = *t;
        } else {
            type = builtin::INVALID;
        }
    }
}

} // namespace z::type
