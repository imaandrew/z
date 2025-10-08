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

// NOLINTBEGIN(bugprone-unchecked-optional-access)

void TypeResolver::fill_top_level_syms(
    const std::vector<std::unique_ptr<Decl>>& decls) const {
    for (const auto& decl : decls) {
        decl->declare_type(syms);
    }

    for (const auto& decl : decls) {
        decl->resolve_sym(syms);
    }
}

void TypeResolver::visit(Identifier& ident) {
    if (const auto type = syms->get_var(ident.to_string())) {
        ident.node_type = type;
    }
}

void TypeResolver::visit(IntExpr& expr) {
    expr.node_type = infctxt->new_type(InferType::IntLiteral);
}

void TypeResolver::visit(FloatExpr& expr) {
    expr.node_type = infctxt->new_type(InferType::FloatLiteral);
}

void TypeResolver::visit(PrefixExpr& expr) {
    expr.expr->accept(*this);
    if (!expr.expr->has_type()) {
        return;
    }

    if (expr.expr->node_type->is_explicit()) {
        expr.node_type = expr.expr->node_type;
    } else {
        expr.node_type = infctxt->new_type(InferType::Var);
        infctxt->eq(expr.node_type, expr.expr->node_type);
    }
}

void TypeResolver::visit(PostfixExpr& expr) {
    expr.expr->accept(*this);
    if (!expr.expr->has_type()) {
        return;
    }

    if (expr.expr->node_type->is_explicit()) {
        expr.node_type = expr.expr->node_type;
    } else {
        expr.node_type = infctxt->new_type(InferType::Var);
        infctxt->eq(expr.node_type, expr.expr->node_type);
    }
}

void TypeResolver::visit(BinaryExpr& expr) {
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
        expr.node_type = std::make_shared<VoidType>();
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

            expr.node_type = infctxt->new_type(InferType::Var);
            infctxt->eq(expr.node_type, expr.lhs->node_type);
            infctxt->eq(expr.node_type, expr.rhs->node_type);
        } else {
            expr.node_type = expr.lhs->node_type;
        }

        break;
    case TokenKind::Dot: {
        auto* var_type = dynamic_cast<VariableType*>(expr.lhs->node_type.get());
        if (var_type == nullptr) {
            break;
        }
        auto* struct_var =
            dynamic_cast<StructType*>(var_type->get_type().get());
        if (const auto* ident = dynamic_cast<Identifier*>(expr.rhs.get())) {
            expr.node_type = std::make_shared<VariableType>(
                struct_var->get_field_type(ident->to_string()));
        }
        break;
    }
    default:
        std::unreachable();
    }
}

void TypeResolver::visit(TernaryExpr& expr) {
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

        expr.node_type = infctxt->new_type(InferType::Var);
        infctxt->eq(expr.node_type, expr.mhs->node_type);
        infctxt->eq(expr.node_type, expr.rhs->node_type);
    } else {
        expr.node_type = expr.mhs->node_type;
    }
}

void TypeResolver::visit(CallExpr& expr) {
    for (auto& arg : expr.args) {
        arg->accept(*this);
    }

    if (const auto* ident = dynamic_cast<Identifier*>(expr.ident.get())) {
        auto func = syms->get_func(ident->to_string());
        const auto* func_ptr = dynamic_cast<FunctionType*>(func.get());
        if (func_ptr == nullptr) {
            return;
        }
        expr.node_type = func_ptr->get_return_val();
    } else {
        expr.ident->accept(*this);
    }
}

void TypeResolver::visit_method_call(BinaryExpr& expr) {
    const auto* impl_type = dynamic_cast<Identifier*>(expr.lhs.get());
    expr.lhs->node_type = syms->get_type(impl_type->to_string());

    auto* func_call = dynamic_cast<CallExpr*>(expr.rhs.get());
    for (auto& arg : func_call->args) {
        arg->accept(*this);
    }

    const std::string name =
        impl_type->to_string() +
        "::" + dynamic_cast<Identifier*>(func_call->ident.get())->to_string();
    auto func = syms->get_func(name);
    const auto* func_ptr = dynamic_cast<FunctionType*>(func.get());
    func_call->node_type = func_ptr->get_return_val();
    func_call->ident->node_type = func;
    expr.node_type = func_call->node_type;
}

void TypeResolver::visit(ArrayExpr& expr) {
    expr.val->accept(*this);

    if (const auto* ident = dynamic_cast<Identifier*>(expr.ident.get())) {
        auto arr = syms->get_var(ident->to_string());
        expr.ident->node_type = arr;

        if (const auto* type = dynamic_cast<ArrayType*>(arr.get())) {
            expr.node_type = type->get_type();
        }
    } else {
        expr.ident->accept(*this);
    }
}

void TypeResolver::visit(ArrayInitExpr& expr) {
    auto internal_type = infctxt->new_type(InferType::Var);
    for (auto& val : expr.vals) {
        val->accept(*this);
        if (!val->has_type()) {
            return;
        }

        if (!infctxt->eq(internal_type, val->node_type))
            return;
    }
    expr.node_type = std::make_shared<ArrayType>(std::move(internal_type));
}

void TypeResolver::visit(StructInitExpr& expr) {
    for (auto& val : expr.vals) {
        val->accept(*this);
    }

    if (auto* ident = dynamic_cast<Identifier*>(expr.ident.get())) {
        expr.node_type = syms->get_type(ident->to_string());
        expr.ident->node_type = expr.node_type;
    } else {
        expr.ident->accept(*this);
    }
}

void TypeResolver::visit(Block& block) {
    syms->enter_scope(block.get_scope_ctxt());
    auto scope_type = infctxt->new_type(InferType::Block);
    syms->get_current_scope()->set_type(scope_type);

    block.node_type = infctxt->new_type(InferType::Block);
    infctxt->eq(scope_type, block.node_type);

    for (auto& stmt : block.stmts) {
        stmt->accept(*this);
    }

    if (!block.stmts.empty() && block.stmts.back()->has_type() &&
        (dynamic_cast<VoidType*>(block.stmts.back()->node_type.get()) ==
         nullptr)) {
        infctxt->eq(scope_type, block.stmts.back()->node_type);
    }

    syms->exit_scope();
}

void TypeResolver::visit(Param& param) {
    syms->resolve_unk_type(param.type);
    param.node_type = param.type;
}

void TypeResolver::visit(FuncDecl& func) {
    if (func.impl_type) {
        func.impl_type->get()->node_type =
            syms->get_type(func.impl_type->get()->to_string());
    }

    for (auto& param : func.params) {
        param->accept(*this);
        dynamic_cast<Block*>(func.body.get())
            ->get_scope_ctxt()
            ->declare_var(param->name, param->type);
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

void TypeResolver::visit(BreakStmt& stmt) {
    if (stmt.expr) {
        stmt.expr->accept(*this);
    }

    stmt.node_type = std::make_shared<VoidType>();
}

void TypeResolver::visit(ContinueStmt& stmt) {
    stmt.node_type = std::make_shared<VoidType>();
}

void TypeResolver::visit(ForExpr& expr) {
    expr.ident->node_type = infctxt->new_type(InferType::Var);
    expr.block->get_scope_ctxt()->declare_var(expr.ident,
                                              expr.ident->node_type);

    expr.block->accept(*this);
    expr.node_type = infctxt->new_type(InferType::Var);
    infctxt->eq(expr.node_type, expr.block->node_type);

    expr.expr->accept(*this);
    if (!expr.expr->has_type()) {
        return;
    }

    if (expr.expr->node_type->is_explicit()) {
        expr.ident->node_type = expr.expr->node_type;
    } else {
        expr.ident->node_type = infctxt->new_type(InferType::Var);
        infctxt->eq(expr.ident->node_type, expr.expr->node_type);
    }
}

void TypeResolver::visit(LetStmt& stmt) {
    if (stmt.val) {
        stmt.val->accept(*this);

        if (!stmt.val->has_type()) {
            syms->declare_var(stmt.ident, std::make_shared<InvalidType>());
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
        stmt.ident->node_type = std::make_shared<VariableType>(stmt.type);
    } else if (stmt.val) {
        auto var_type =
            std::make_shared<VariableType>(infctxt->new_type(InferType::Var));
        infctxt->eq(var_type->get_type(), stmt.val->node_type);
        stmt.ident->node_type = var_type;
    } else {
        stmt.ident->node_type =
            std::make_shared<VariableType>(infctxt->new_type(InferType::Var));
    }

    syms->declare_var(stmt.ident, stmt.ident->node_type);
    stmt.node_type = std::make_shared<VoidType>();
}

void TypeResolver::visit(ReturnStmt& stmt) {
    auto& current_scope_type = syms->get_current_scope()->get_type();

    if (stmt.expr) {
        stmt.expr->accept(*this);
        if (!stmt.expr->has_type()) {
            return;
        }

        infctxt->eq(current_scope_type, stmt.expr->node_type);
    } else {
        infctxt->eq(current_scope_type, std::make_shared<VoidType>());
    }

    stmt.node_type = std::make_shared<VoidType>();
}

void TypeResolver::visit(IfExpr& expr) {
    expr.expr->accept(*this);

    expr.block->accept(*this);

    expr.node_type = infctxt->new_type(InferType::Var);
    infctxt->eq(expr.node_type, expr.block->node_type);

    if (expr.else_expr) {
        expr.else_expr->accept(*this);
        if (expr.else_expr->has_type()) {
            infctxt->eq(expr.node_type, expr.else_expr->node_type);
        }
    }
}

void TypeResolver::visit(ElseExpr& expr) {
    if (expr.if_expr) {
        expr.if_expr->accept(*this);
        if (expr.if_expr->has_type()) {
            expr.node_type = infctxt->new_type(InferType::Var);
            infctxt->eq(expr.node_type, expr.if_expr->node_type);
        }
    } else if (expr.block) {
        expr.block->accept(*this);
        expr.node_type = infctxt->new_type(InferType::Var);
        infctxt->eq(expr.node_type, expr.block->node_type);
    }
}

void TypeResolver::visit(LoopExpr& expr) {
    if (expr.expr) {
        expr.expr->accept(*this);
    }

    expr.block->accept(*this);
    expr.node_type = infctxt->new_type(InferType::Var);
    infctxt->eq(expr.node_type, expr.block->node_type);
}

void TypeResolver::visit(WhileExpr& expr) {
    expr.expr->accept(*this);

    expr.block->accept(*this);
    expr.node_type = infctxt->new_type(InferType::Var);
    infctxt->eq(expr.node_type, expr.block->node_type);
}

void TypeResolver::visit(StringExpr& expr) {
    expr.node_type = std::make_unique<StringType>();
}

void TypeResolver::visit(StructField& /* field */) {}

void TypeResolver::visit(StructDecl& decl) {
    decl.node_type = syms->get_type(decl.ident->to_string());
    decl.ident->node_type = decl.node_type;

    const auto* struct_type = dynamic_cast<StructType*>(decl.node_type.get());
    for (auto& field : decl.fields) {
        field->ident->node_type =
            struct_type->get_field_type(field->ident->to_string());
        field->node_type = field->ident->node_type;
    }
}

void TypeResolver::visit(EnumField& /* field*/) {}

void TypeResolver::visit(EnumDecl& decl) {
    decl.node_type = syms->get_type(decl.ident->to_string());
    decl.ident->node_type = syms->get_type(decl.ident->to_string());

    for (auto& field : decl.fields) {
        field->node_type =
            std::make_shared<EnumVariantType>(decl.ident->to_string());
    }
}

void TypeResolver::visit(ConstDecl& decl) {
    decl.ident->node_type = syms->get_global_var(decl.ident->to_string());

    decl.val->accept(*this);
    if (decl.val->has_type()) {
        infctxt->eq(decl.val->node_type, decl.type);
    }

    decl.node_type = std::make_shared<VoidType>();

    resolve(&decl);
    infctxt.emplace();
}

void TypeResolver::visit(StaticDecl& decl) {
    decl.ident->node_type = std::make_shared<VariableType>(decl.type);

    decl.val->accept(*this);
    if (decl.val->has_type()) {
        infctxt->eq(decl.val->node_type, decl.type);
    }

    decl.node_type = std::make_shared<VoidType>();

    resolve(&decl);
    infctxt.emplace();
}

void TypeResolver::resolve(std::shared_ptr<Type>& type) {
    type = infctxt->try_resolve(type);
    if (!type->is_explicit()) {
        const auto* infer = get_inf_type(type.get());
        if (infer->get_infer_type() == InferType::IntLiteral) {
            type = std::make_shared<IntegerType>(32, true);
        } else if (infer->get_infer_type() == InferType::FloatLiteral) {
            type = std::make_shared<FloatType>(64);
        } else if (infer->get_infer_type() == InferType::Block) {
            type = std::make_shared<VoidType>();
        } else {
            std::cerr << "UNK TYPE\n";
        }
    } else if (type->is_unknown()) {
        type = syms->get_type(
            dynamic_cast<UnknownType*>(type.get())->get_ident()->to_string());
    }
}

void TypeResolver::resolve(ASTNode* node) {
    if (auto* block = dynamic_cast<Block*>(node)) {
        for (auto& stmt : block->stmts) {
            resolve(stmt.get());
        }
        resolve(node->node_type);
        return;
    }

    if (node == nullptr)
        return;

    if (node->has_type())
        resolve(node->node_type);

    if (auto* expr = dynamic_cast<PrefixExpr*>(node)) {
        resolve(expr->expr.get());
    } else if (auto* expr = dynamic_cast<PostfixExpr*>(node)) {
        resolve(expr->expr.get());
    } else if (auto* expr = dynamic_cast<BinaryExpr*>(node)) {
        resolve(expr->lhs.get());
        resolve(expr->rhs.get());
    } else if (auto* expr = dynamic_cast<TernaryExpr*>(node)) {
        resolve(expr->lhs.get());
        resolve(expr->mhs.get());
        resolve(expr->rhs.get());
    } else if (auto* expr = dynamic_cast<CallExpr*>(node)) {
        for (auto& arg : expr->args) {
            resolve(arg.get());
        }
    } else if (auto* expr = dynamic_cast<ArrayExpr*>(node)) {
        resolve(expr->ident.get());
        resolve(expr->val.get());
    } else if (auto* expr = dynamic_cast<ArrayInitExpr*>(node)) {
        for (auto& val : expr->vals) {
            resolve(val.get());
        }
    } else if (auto* expr = dynamic_cast<StructInitExpr*>(node)) {
        resolve(expr->ident.get());
        for (auto& val : expr->vals) {
            resolve(val.get());
        }
    } else if (auto* decl = dynamic_cast<FuncDecl*>(node)) {
        resolve(decl->name.get());
        if (decl->impl_type)
            resolve(decl->impl_type->get());
        resolve(decl->body.get());
    } else if (auto* stmt = dynamic_cast<BreakStmt*>(node)) {
        if (stmt->expr)
            resolve(stmt->expr.get());
    } else if (auto* expr = dynamic_cast<ForExpr*>(node)) {
        resolve(expr->ident.get());
        resolve(expr->expr.get());
        resolve(expr->block.get());
    } else if (auto* stmt = dynamic_cast<LetStmt*>(node)) {
        resolve(stmt->ident.get());
        if (stmt->type)
            resolve(stmt->type);
        if (stmt->val)
            resolve(stmt->val.get());
    } else if (auto* stmt = dynamic_cast<ReturnStmt*>(node)) {
        if (stmt->expr)
            resolve(stmt->expr.get());
    } else if (auto* expr = dynamic_cast<ElseExpr*>(node)) {
        if (expr->if_expr)
            resolve(expr->if_expr.get());
        else
            resolve(expr->block.get());
    } else if (auto* expr = dynamic_cast<IfExpr*>(node)) {
        resolve(expr->expr.get());
        resolve(expr->block.get());
        if (expr->else_expr)
            resolve(expr->else_expr.get());
    } else if (auto* expr = dynamic_cast<LoopExpr*>(node)) {
        resolve(expr->expr.get());
        resolve(expr->block.get());
    } else if (auto* expr = dynamic_cast<WhileExpr*>(node)) {
        resolve(expr->expr.get());
        resolve(expr->block.get());
    } else if (auto* decl = dynamic_cast<ConstDecl*>(node)) {
        resolve(decl->ident.get());
        resolve(decl->val.get());
    } else if (auto* decl = dynamic_cast<StaticDecl*>(node)) {
        resolve(decl->ident.get());
        resolve(decl->val.get());
    }
}

// NOLINTEND(bugprone-unchecked-optional-access)