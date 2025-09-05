#pragma once

#include "ast.h"
#include "diagnostics.h"
#include "src_mgr.h"
#include "sym_table.h"
#include "token.h"
#include "type.h"
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

class SemChecker : public ASTVisitor {
    SymbolTable* syms;
    DiagnosticsEngine diag;

public:
    SemChecker(SymbolTable* syms, SourceManager* src)
        : syms(syms), diag(src) {};
    ~SemChecker() override = default;
    SemChecker(const SemChecker& other) : syms(other.syms), diag(other.diag) {};
    SemChecker(SemChecker&& other) noexcept
        : syms(std::move(other.syms)), diag(std::move(other.diag)) {};
    SemChecker& operator=(const SemChecker& other) {
        if (this != &other) {
            syms = other.syms;
            diag = other.diag;
        }
        return *this;
    }

    SemChecker& operator=(SemChecker&& other) noexcept {
        if (this != &other) {
            syms = std::move(other.syms);
            diag = std::move(other.diag);
        }
        return *this;
    }

    void
    fill_top_level_syms(const std::vector<std::unique_ptr<Decl>>& decls) const {
        for (const auto& decl : decls) {
            decl->declare_type(syms);
        }

        for (const auto& decl : decls) {
            decl->resolve_sym(syms);
        }
    }

    void visit(InvalidStmt& stmt) override {
        stmt.node_type = std::make_unique<InvalidType>();
    }

    void visit(IntExpr& expr) override {
        if (expr.val > INT32_MAX)
            expr.node_type = std::make_unique<IntegerType>(8, false);
        else
            expr.node_type = std::make_unique<IntegerType>(4, false);
    }

    void visit(FloatExpr& expr) override {
        expr.node_type = std::make_unique<FloatType>(8);
    }

    void visit(PrefixExpr& expr) override {
        expr.expr->accept(*this);

        auto* type = expr.expr->node_type.get();
        switch (expr.op.get_kind()) {
        case TokenKind::PlusPlus:
        case TokenKind::MinusMinus:
            if (!type->is_assignable()) {
                // error: expr is not assignable
            }

            if (!type->is_integral()) {
                // error: cannot increment expr of type <type>
            }
            break;
        case TokenKind::LogicalNot:
            if (!type->is_logical()) {
                // error: expected boolean type
            }
            break;
        case TokenKind::Not:
            if (!type->is_integral()) {
                // error: expected integral type
            }
            break;
        case TokenKind::Minus:
            if (!type->is_numeric()) {
                // error: expected numerical type
            }
            break;
        default:
            std::unreachable();
        }

        expr.node_type = expr.expr->node_type;
    }

    void visit(PostfixExpr& expr) override {
        expr.expr->accept(*this);

        switch (expr.op.get_kind()) {
        case TokenKind::PlusPlus:
        case TokenKind::MinusMinus:
            if (!expr.expr->node_type->is_assignable()) {
                // error: expr is not assignable
            }

            if (!expr.expr->node_type->is_integral()) {
                // error: cannot increment expr of type <type>
            }
            break;
        default:
            std::unreachable();
        }

        expr.node_type = expr.expr->node_type;
    }

    void visit(BinaryExpr& expr) override {
        expr.lhs->accept(*this);
        expr.rhs->accept(*this);

        auto* l_type = expr.lhs->node_type.get();
        auto* r_type = expr.rhs->node_type.get();

        switch (expr.op.get_kind()) {
        case TokenKind::PlusEq:
        case TokenKind::MinusEq:
        case TokenKind::StarEq:
        case TokenKind::SlashEq:
            if (!l_type->is_assignable()) {
                // error
            }

            if (!l_type->is_numeric()) {
                // error
            }

            if (!r_type->is_numeric()) {
                // error
            }

            expr.node_type = std::make_shared<VoidType>();
            break;
        case TokenKind::PercentEq:
        case TokenKind::CaretEq:
        case TokenKind::AndEq:
        case TokenKind::OrEq:
        case TokenKind::ShlEq:
        case TokenKind::ShrEq:
            if (!l_type->is_assignable()) {
                // error
            }

            if (!l_type->is_integral()) {
                // error
            }

            if (!r_type->is_integral()) {
                // error
            }

            expr.node_type = std::make_shared<VoidType>();
            break;
        case TokenKind::Eq:
            if (!l_type->is_assignable()) {
                // error
            }

            if (!l_type->is_assignment_compatible(r_type)) {
                // error
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
            if (!l_type->is_logical()) {
                // error
            }

            if (!r_type->is_logical()) {
                // error
            }

            expr.node_type = expr.lhs->node_type;
            break;
        case TokenKind::Range:
        case TokenKind::RangeEq:
        case TokenKind::Or:
        case TokenKind::Caret:
        case TokenKind::And:
        case TokenKind::Shl:
        case TokenKind::Shr:
        case TokenKind::Percent:
            if (!l_type->is_integral()) {
                // error
            }

            if (!r_type->is_integral()) {
                // error
            }

            if (!l_type->is_arithmetic_compatible(r_type)) {
                // error
            }

            expr.node_type = expr.lhs->node_type;
            break;
        case TokenKind::Plus:
        case TokenKind::Minus:
        case TokenKind::Star:
        case TokenKind::Slash:
            if (!l_type->is_numeric()) {
                // error
            }

            if (!r_type->is_numeric()) {
                // error
            }

            if (!l_type->is_arithmetic_compatible(r_type)) {
                // error
            }

            expr.node_type = expr.lhs->node_type;
            break;
        case TokenKind::ColonColon:

            break;
        case TokenKind::Dot: {
            if (l_type->is_unknown()) {
                // error
            }

            if (!l_type->is_variable()) {
            }

            if (!r_type->is_variable()) {
                // error
            }

            auto* struct_var = dynamic_cast<VariableType*>(expr.lhs.get());
            auto* field_var = dynamic_cast<VariableType*>(expr.rhs.get());
            break;
        }
        default:
            std::unreachable();
        }
    }

    void visit(TernaryExpr& expr) override {
        // expr.lhs->accept(*this);
        // expr.rhs->accept(*this);
        // expr.mhs->accept(*this);
    }

    void visit(CallExpr& expr) override {
        // expr.ident->accept(*this);
        // for (auto& arg : expr.args) {
        //     arg->accept(*this);
        // }
    }

    void visit(ArrayExpr& expr) override {
        // expr.ident->accept(*this);
        // expr.val->accept(*this);
    }

    void visit(ArrayInitExpr& expr) override {
        // for (auto& val : expr.vals) {
        //     val->accept(*this);
        // }
    }

    void visit(StructInitExpr& expr) override {
        expr.ident->accept(*this);
        for (auto& val : expr.vals) {
            val->accept(*this);
        }
    }

    void visit(Identifier& ident) override {
        ident.node_type = std::make_unique<InvalidType>();
    }

    void visit(Block& block) override {
        for (auto& stmt : block.stmts) {
            stmt->accept(*this);
        }
    }

    void visit(Param& param) override {
        syms->resolve_unk_type(param.type);
        syms->declare_var(param.name, param.type);
    }

    void visit(FuncDecl& func) override {
        func.name->accept(*this);

        if (func.impl_type)
            func.impl_type->get()->accept(*this);

        syms->enter_scope(func.body->get_scope_ctxt());

        for (auto& param : func.params) {
            param->accept(*this);
        }

        func.body->accept(*this);

        syms->exit_scope();
    }

    void visit(BreakStmt& stmt) override {
        if (stmt.expr)
            stmt.expr->accept(*this);
    }

    void visit(ContinueStmt& stmt) override {}

    void visit(ForExpr& expr) override {
        expr.ident->accept(*this);
        expr.expr->accept(*this);
        expr.expr->accept(*this);
    }

    void visit(LetStmt& stmt) override {
        stmt.ident->accept(*this);
        stmt.val->accept(*this);
    }

    void visit(ReturnStmt& stmt) override {
        if (stmt.expr)
            stmt.expr->accept(*this);
    }

    void visit(IfExpr& expr) override {
        expr.expr->accept(*this);
        expr.block->accept(*this);
        if (expr.else_expr)
            expr.else_expr->accept(*this);
    }

    void visit(ElseExpr& expr) override {
        if (expr.if_expr)
            expr.if_expr->accept(*this);
        if (expr.block)
            expr.block->accept(*this);
    }

    void visit(LoopExpr& expr) override {
        if (expr.expr)
            expr.expr->accept(*this);

        expr.block->accept(*this);
    }

    void visit(WhileExpr& expr) override {
        expr.expr->accept(*this);
        expr.block->accept(*this);
    }

    void visit(StringExpr& expr) override {
        expr.node_type = std::make_unique<StringType>();
    }

    void visit(StructField& field) override {
        field.ident->accept(*this);
        auto type = field.type;
        // if (auto type = syms->get_type(field->))
    }

    void visit(StructDecl& decl) override {
        decl.ident->accept(*this);
        for (auto& field : decl.fields)
            field->accept(*this);
    }

    void visit(EnumField& field) override { field.ident->accept(*this); }

    void visit(EnumDecl& decl) override {
        decl.ident->accept(*this);
        for (auto& field : decl.fields)
            field->accept(*this);
    }

    void visit(ConstDecl& decl) override {
        decl.ident->accept(*this);
        decl.val->accept(*this);
    }

    void visit(StaticDecl& decl) override {
        decl.ident->accept(*this);
        decl.val->accept(*this);
    }
};