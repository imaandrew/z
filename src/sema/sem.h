#pragma once

#include "core/zctxt.h"
#include "diag/src_mgr.h"
#include "parser/ast.h"
#include "type/type.h"
#include "type/type_ref.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace z {

class SemChecker : public ast::ASTVisitor {
    enum class ReachableStatus : std::uint8_t {
        Reachable,
        Unreachable,
        WarningEmitted
    };

    struct LoopContext {
        type::TypeRef expected_type;
        Span first_break{};
        bool has_break = false;
    };

    ZContext* ctxt;
    int loop_depth = 0;
    ast::Identifier* current_func_name = nullptr;
    std::optional<type::TypeRef> current_return_type;
    ReachableStatus is_stmt_reachable = ReachableStatus::Reachable;
    std::vector<LoopContext> loop_stack;

public:
    explicit SemChecker(ZContext& ctxt) : ctxt(&ctxt) {};
    ~SemChecker() override = default;
    SemChecker(const SemChecker& other) = delete;
    SemChecker(SemChecker&& other) = delete;
    SemChecker& operator=(const SemChecker& other) = delete;
    SemChecker& operator=(SemChecker&& other) = delete;

    void visit(ast::Identifier& ident) override;
    void visit(ast::IntExpr& expr) override;
    void visit(ast::FloatExpr& expr) override;
    void visit(ast::BoolExpr& expr) override;
    void visit(ast::PrefixExpr& expr) override;
    void visit(ast::PostfixExpr& expr) override;
    void visit(ast::BinaryExpr& expr) override;
    void visit(ast::CallExpr& expr) override;
    void visit(ast::ArrayExpr& expr) override;
    void visit(ast::FieldExpr& expr) override;
    void visit(ast::ArrayInitExpr& expr) override;
    void visit(ast::StructExprField& expr) override;
    void visit(ast::StructInitExpr& expr) override;
    void visit(ast::TupleExpr& expr) override;
    void visit(ast::Block& block) override;
    void visit(ast::Param& param) override;
    void visit(ast::SourceFileDecl& file) override;
    void visit(ast::FuncDecl& func) override;
    void visit(ast::BreakStmt& stmt) override;
    void visit(ast::ContinueStmt& stmt) override;
    void visit(ast::ForExpr& expr) override;
    void visit(ast::LetStmt& stmt) override;
    void visit(ast::ReturnStmt& stmt) override;
    void visit(ast::IfExpr& expr) override;
    void visit(ast::ElseExpr& expr) override;
    void visit(ast::LoopExpr& expr) override;
    void visit(ast::WhileExpr& expr) override;
    void visit(ast::StringExpr& expr) override;
    void visit(ast::CharExpr& expr) override;
    void visit(ast::StructField& field) override;
    void visit(ast::StructDecl& decl) override;
    void visit(ast::EnumField& field) override;
    void visit(ast::EnumDecl& decl) override;
    void visit(ast::ConstDecl& decl) override;
    void visit(ast::StaticDecl& decl) override;
    void visit(ast::TraitDecl& decl) override;
    void visit(ast::TypeAliasDecl& decl) override;
    void visit(ast::TraitFuncDecl& decl) override;

    void check_expr_assignable(ast::Expr& expr) const;
    bool is_recursive_struct(const type::StructType* s,
                             type::TypeRef orig) const;
};
} // namespace z
