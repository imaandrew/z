#pragma once

#include "diagnostics.h"
#include "scope.h"
#include "sym_table.h"
#include "token.h"
#include "type.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class BinOpPrecedence : std::uint8_t {
    Unknown,
    Assignment,
    Range,
    Conditional,
    LogicalOr,
    LogicalAnd,
    Equality,
    Or,
    Xor,
    And,
    Shift,
    Addition,
    Multiplication,
    Prefix,
    ScopeRes,
    Postfix,
};

struct InvalidStmt;
struct IntExpr;
struct FloatExpr;
struct PrefixExpr;
struct PostfixExpr;
struct BinaryExpr;
struct TernaryExpr;
struct CallExpr;
struct ArrayExpr;
struct ArrayInitExpr;
struct StructInitExpr;
struct Identifier;
struct Block;
struct Param;
struct FuncDecl;
struct BreakStmt;
struct ContinueStmt;
struct ForExpr;
struct LetStmt;
struct ReturnStmt;
struct IfExpr;
struct ElseExpr;
struct LoopExpr;
struct WhileExpr;
struct StringExpr;
struct StructField;
struct StructDecl;
struct EnumField;
struct EnumDecl;
struct ConstDecl;
struct StaticDecl;

class ASTVisitor {
public:
    ASTVisitor() = default;
    virtual ~ASTVisitor() = default;
    ASTVisitor(const ASTVisitor&) = delete;
    ASTVisitor& operator=(const ASTVisitor&) = delete;
    ASTVisitor(ASTVisitor&&) = delete;
    ASTVisitor& operator=(ASTVisitor&&) = delete;
    virtual void visit(InvalidStmt&) = 0;
    virtual void visit(IntExpr&) = 0;
    virtual void visit(FloatExpr&) = 0;
    virtual void visit(PrefixExpr&) = 0;
    virtual void visit(PostfixExpr&) = 0;
    virtual void visit(BinaryExpr&) = 0;
    virtual void visit(TernaryExpr&) = 0;
    virtual void visit(CallExpr&) = 0;
    virtual void visit(ArrayExpr&) = 0;
    virtual void visit(ArrayInitExpr&) = 0;
    virtual void visit(StructInitExpr&) = 0;
    virtual void visit(Identifier&) = 0;
    virtual void visit(Block&) = 0;
    virtual void visit(Param&) = 0;
    virtual void visit(FuncDecl&) = 0;
    virtual void visit(BreakStmt&) = 0;
    virtual void visit(ContinueStmt&) = 0;
    virtual void visit(ForExpr&) = 0;
    virtual void visit(LetStmt&) = 0;
    virtual void visit(ReturnStmt&) = 0;
    virtual void visit(IfExpr&) = 0;
    virtual void visit(ElseExpr&) = 0;
    virtual void visit(LoopExpr&) = 0;
    virtual void visit(WhileExpr&) = 0;
    virtual void visit(StringExpr&) = 0;
    virtual void visit(StructField&) = 0;
    virtual void visit(StructDecl&) = 0;
    virtual void visit(EnumField&) = 0;
    virtual void visit(EnumDecl&) = 0;
    virtual void visit(ConstDecl&) = 0;
    virtual void visit(StaticDecl&) = 0;
};

struct ASTNode {
    bool valid = true;
    std::shared_ptr<Type> node_type = nullptr;

    ASTNode() = default;
    virtual ~ASTNode() = default;

    ASTNode(const ASTNode&) = delete;
    ASTNode& operator=(const ASTNode&) = delete;
    ASTNode(ASTNode&&) = delete;
    ASTNode& operator=(ASTNode&&) = delete;

    [[nodiscard]] bool is_valid() const { return valid; }
    void mark_invalid() { valid = false; }
    virtual void accept(ASTVisitor& visitor) = 0;

    virtual void dump(SourceManager* source, int indent = 0,
                      std::ostream& stream = std::cout) const = 0;

    void dump_type(std::ostream& stream = std::cout) const {
        stream << " - type: ";
        if (node_type)
            node_type->dump(stream);
        else
            stream << "null";
        stream << '\n';
    }
};

struct Stmt : ASTNode {
    Stmt() = default;
    ~Stmt() override = default;
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;
    Stmt(Stmt&&) = delete;
    Stmt& operator=(Stmt&&) = delete;
};

struct InvalidStmt final : Stmt {
    InvalidStmt() { mark_invalid(); };

    void dump(SourceManager* /* source */, int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "InvalidStmt";
        dump_type(stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct Expr : Stmt {
    Expr() = default;
    ~Expr() override = default;
    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;
    Expr(Expr&&) = delete;
    Expr& operator=(Expr&&) = delete;
};

struct Identifier final : Expr {
    Token tok;
    std::string_view ident;

    explicit Identifier(const Token& tok, std::string_view ident)
        : tok(tok), ident(ident) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "Identifier "
               << source->get_string(tok.get_span());
        dump_type(stream);
    }

    [[nodiscard]] std::string to_string() const { return std::string(ident); }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct IntExpr final : Expr {
    Token tok;
    unsigned long long val;

    IntExpr(const Token& tok, const unsigned long long val)
        : tok(tok), val(val) {};

    void dump(SourceManager* /* source */, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "IntExpr " << val;
        dump_type(stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct FloatExpr final : Expr {
    Token tok;
    double val;

    FloatExpr(const Token& tok, const double val) : tok(tok), val(val) {};

    void dump(SourceManager* /* source */, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "FloatExpr " << val;
        dump_type(stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

// NOLINTBEGIN(readability-identifier-length)

struct PrefixExpr final : Expr {
    Token op;
    std::shared_ptr<Expr> expr;

    PrefixExpr(const Token& op, std::unique_ptr<Expr> expr)
        : op(op), expr(std::move(expr)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "PrefixExpr "
               << source->get_string(op.get_span());
        dump_type(stream);
        expr->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct PostfixExpr final : Expr {
    Token op;
    std::unique_ptr<Expr> expr;

    PostfixExpr(const Token& op, std::unique_ptr<Expr> expr)
        : op(op), expr(std::move(expr)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "PostfixExpr "
               << source->get_string(op.get_span());
        dump_type(stream);
        expr->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct BinaryExpr final : Expr {
    Token op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

    BinaryExpr(const Token& op, std::unique_ptr<Expr> lhs,
               std::unique_ptr<Expr> rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "BinaryExpr "
               << source->get_string(op.get_span());
        dump_type(stream);
        lhs->dump(source, indent + 2, stream);
        rhs->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct TernaryExpr final : Expr {
    Token op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> mhs;
    std::unique_ptr<Expr> rhs;

    TernaryExpr(const Token& op, std::unique_ptr<Expr> lhs,
                std::unique_ptr<Expr> mhs, std::unique_ptr<Expr> rhs)
        : op(op), lhs(std::move(lhs)), mhs(std::move(mhs)),
          rhs(std::move(rhs)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "TernaryExpr "
               << source->get_string(op.get_span());
        dump_type(stream);
        lhs->dump(source, indent + 2, stream);
        mhs->dump(source, indent + 2, stream);
        rhs->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

// NOLINTEND(readability-identifier-length)

struct CallExpr final : Expr {
    std::unique_ptr<Expr> ident;
    std::vector<std::unique_ptr<Expr>> args;

    CallExpr(std::unique_ptr<Expr> func,
             std::vector<std::unique_ptr<Expr>> args)
        : ident(std::move(func)), args(std::move(args)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "CallExpr";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        for (const auto& arg : args) {
            arg->dump(source, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct ArrayExpr final : Expr {
    std::unique_ptr<Expr> ident;
    std::unique_ptr<Expr> val;

    explicit ArrayExpr(std::unique_ptr<Expr> ident)
        : ident(std::move(ident)) {};

    ArrayExpr(std::unique_ptr<Expr> ident, std::unique_ptr<Expr> val)
        : ident(std::move(ident)), val(std::move(val)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ArrayExpr";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        if (val)
            val->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct ArrayInitExpr final : Expr {
    std::vector<std::unique_ptr<Expr>> vals;

    explicit ArrayInitExpr(std::vector<std::unique_ptr<Expr>> vals)
        : vals(std::move(vals)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ArrayInitExpr";
        dump_type(stream);
        for (const auto& val : vals) {
            val->dump(source, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct StructInitExpr final : Expr {
    std::unique_ptr<Expr> ident;
    std::vector<std::unique_ptr<Expr>> vals;

    StructInitExpr(std::unique_ptr<Expr> ident,
                   std::vector<std::unique_ptr<Expr>> vals)
        : ident(std::move(ident)), vals(std::move(vals)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "StructInitExpr";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        for (const auto& val : vals) {
            val->dump(source, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct Block final : Stmt {
    std::vector<std::unique_ptr<Stmt>> stmts;
    ScopeContext ctxt;

    explicit Block(std::vector<std::unique_ptr<Stmt>> stmts)
        : stmts(std::move(stmts)) {};

    ScopeContext* get_scope_ctxt() { return &ctxt; }

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "Block";
        dump_type(stream);
        for (const auto& stmt : stmts) {
            stmt->dump(source, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct Param final : Expr {
    std::unique_ptr<Identifier> name;
    std::shared_ptr<Type> type;

    Param(std::unique_ptr<Identifier> name, std::shared_ptr<Type> type)
        : name(std::move(name)), type(std::move(type)) {};

    void dump(SourceManager* source, int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "Param";
        dump_type(stream);
        name->dump(source, indent + 2, stream);
        type->dump(stream);
        stream << '\n';
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct Decl : ASTNode {
    Decl() = default;
    ~Decl() override = default;
    Decl(const Decl&) = delete;
    Decl& operator=(const Decl&) = delete;
    Decl(Decl&&) = delete;
    Decl& operator=(Decl&&) = delete;

    virtual void declare_type(SymbolTable* syms) = 0;
    virtual void resolve_sym(SymbolTable* syms) = 0;
};

struct FuncDecl final : Decl {
    std::unique_ptr<Identifier> name;
    std::optional<std::shared_ptr<Identifier>> impl_type;
    std::vector<std::unique_ptr<Param>> params;
    std::shared_ptr<Type> ret;
    std::unique_ptr<Block> body;

    FuncDecl(std::unique_ptr<Identifier> name,
             std::optional<std::shared_ptr<Identifier>> impl_type,
             std::vector<std::unique_ptr<Param>> params,
             std::shared_ptr<Type> ret, std::unique_ptr<Block> body)
        : name(std::move(name)), impl_type(std::move(impl_type)),
          params(std::move(params)), ret(std::move(ret)),
          body(std::move(body)) {};

    [[nodiscard]] std::string get_abs_name() const {
        if (impl_type)
            return impl_type->get()->to_string() + "::" + name->to_string();
        return name->to_string();
    }

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "FuncDecl";
        dump_type(stream);
        name->dump(source, indent + 2, stream);
        if (impl_type && impl_type.has_value())
            impl_type.value()->dump(source, indent + 2, stream);
        body->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }

    void declare_type(SymbolTable* syms) override {
        auto param_types = std::vector<std::shared_ptr<Type>>();
        for (const auto& param : params) {
            param_types.push_back(param->type);
        }

        valid = syms->declare_func(
            get_abs_name(), name->tok,
            std::make_shared<FunctionType>(param_types, ret));
    }

    void resolve_sym(SymbolTable* syms) override {
        const auto func_type = syms->get_func(get_abs_name());

        for (const auto& param : params) {
            if (param->type->is_unknown() &&
                !syms->resolve_unk_type(param->type)) {
                valid = false;
            }
        }

        if (ret->is_unknown()) {
            if (syms->resolve_unk_type(ret)) {
                func_type->set_return_val(ret);
            } else {
                valid = false;
            }
        }
    }
};

struct BreakStmt final : Stmt {
    Token tok;
    std::unique_ptr<Expr> expr;

    explicit BreakStmt(const Token& tok) : tok(tok) {};
    BreakStmt(const Token& tok, std::unique_ptr<Expr> expr)
        : tok(tok), expr(std::move(expr)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "BreakStmt";
        dump_type(stream);
        if (expr)
            expr->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct ContinueStmt final : Stmt {
    Token tok;

    explicit ContinueStmt(const Token& tok) : tok(tok) {};

    void dump(SourceManager* /* source */, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ContinueStmt";
        dump_type(stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct ForExpr final : Expr {
    std::unique_ptr<Identifier> ident;
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;

    ForExpr(std::unique_ptr<Identifier> ident, std::unique_ptr<Expr> expr,
            std::unique_ptr<Block> block)
        : ident(std::move(ident)), expr(std::move(expr)),
          block(std::move(block)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ForExpr";
        dump_type(stream);
        expr->dump(source, indent + 2, stream);
        block->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct LetStmt final : Stmt {
    std::unique_ptr<Identifier> ident;
    std::shared_ptr<Type> type;
    std::shared_ptr<Expr> val;

    explicit LetStmt(std::unique_ptr<Identifier> ident)
        : ident(std::move(ident)) {};

    LetStmt(std::unique_ptr<Identifier> ident, std::shared_ptr<Expr> val)
        : ident(std::move(ident)), val(std::move(val)) {};

    LetStmt(std::unique_ptr<Identifier> ident, std::shared_ptr<Type> type,
            std::shared_ptr<Expr> val)
        : ident(std::move(ident)), type(std::move(type)),
          val(std::move(val)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "LetStmt";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        if (type) {
            stream << std::string(indent + 2, ' ');
            type->dump(stream);
            stream << '\n';
        }
        if (val)
            val->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct ReturnStmt final : Stmt {
    std::unique_ptr<Expr> expr;

    ReturnStmt() = default;
    explicit ReturnStmt(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ReturnStmt";
        dump_type(stream);
        if (expr)
            expr->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct ElseExpr final : Expr {
    std::unique_ptr<Expr> if_expr;
    std::unique_ptr<Block> block;

    explicit ElseExpr(std::unique_ptr<Block> block)
        : block(std::move(block)) {};
    explicit ElseExpr(std::unique_ptr<Expr> if_expr)
        : if_expr(std::move(if_expr)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ElseExpr";
        dump_type(stream);

        if (if_expr) {
            if_expr->dump(source, indent + 2, stream);
        } else if (block) {
            block->dump(source, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct IfExpr final : Expr {
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;
    std::unique_ptr<ElseExpr> else_expr;

    IfExpr(std::unique_ptr<Expr> expr, std::unique_ptr<Block> block)
        : expr(std::move(expr)), block(std::move(block)) {};
    IfExpr(std::unique_ptr<Expr> expr, std::unique_ptr<Block> block,
           std::unique_ptr<ElseExpr> else_expr)
        : expr(std::move(expr)), block(std::move(block)),
          else_expr(std::move(else_expr)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "IfExpr";
        dump_type(stream);
        expr->dump(source, indent + 2, stream);
        block->dump(source, indent + 2, stream);
        if (else_expr) {
            else_expr->dump(source, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct LoopExpr final : Expr {
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;

    LoopExpr(std::unique_ptr<Expr> expr, std::unique_ptr<Block> block)
        : expr(std::move(expr)), block(std::move(block)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "LoopExpr";
        dump_type(stream);
        if (expr) {
            expr->dump(source, indent + 2, stream);
        }
        block->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct WhileExpr final : Expr {
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;

    WhileExpr(std::unique_ptr<Expr> expr, std::unique_ptr<Block> block)
        : expr(std::move(expr)), block(std::move(block)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "WhileExpr";
        dump_type(stream);
        expr->dump(source, indent + 2, stream);
        block->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct StringExpr final : Expr {
    Span span;

    explicit StringExpr(Span span) : span(span) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "String '"
               << source->get_string(span) << '\'';
        dump_type(stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct StructField final : ASTNode {
    std::shared_ptr<Identifier> ident;
    std::shared_ptr<Type> type;

    StructField(std::shared_ptr<Identifier> ident, std::shared_ptr<Type> type)
        : ident(std::move(ident)), type(std::move(type)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "StructField";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        stream << std::string(indent + 2, ' ');
        type->dump(stream);
        stream << '\n';
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct StructDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    std::vector<std::unique_ptr<StructField>> fields;

    StructDecl(std::unique_ptr<Identifier> ident,
               std::vector<std::unique_ptr<StructField>> fields)
        : ident(std::move(ident)), fields(std::move(fields)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "StructDecl";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        for (const auto& field : fields)
            field->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }

    void declare_type(SymbolTable* syms) override {
        const auto name = ident->to_string();

        if (!syms->declare_type(ident, std::make_shared<StructType>(ident))) {
            valid = false;
            return;
        }

        auto* struct_type =
            dynamic_cast<StructType*>(syms->get_type(name).get());

        for (const auto& field : fields) {
            const bool is_unique = struct_type->define_field(
                field->ident->to_string(), field->type);
            if (!is_unique) {
                syms->diag.emit(field->ident->tok.get_span(),
                                DiagnosticKind::DuplicateField,
                                ident->to_string(), field->ident->to_string());
                valid = false;
            }
        }
    }

    void resolve_sym(SymbolTable* syms) override {
        auto* struct_type =
            dynamic_cast<StructType*>(syms->get_type(ident->to_string()).get());

        for (const auto& field : fields) {
            if (field->type->is_unknown()) {
                if (syms->resolve_unk_type(field->type)) {
                    struct_type->replace_field_type(field->ident->to_string(),
                                                    field->type);
                } else {
                    valid = false;
                }
            }
        }
    }
};

struct EnumField final : ASTNode {
    std::unique_ptr<Identifier> ident;
    std::vector<std::shared_ptr<Type>> types;

    explicit EnumField(std::unique_ptr<Identifier> ident)
        : ident(std::move(ident)) {};
    EnumField(std::unique_ptr<Identifier> ident,
              std::vector<std::shared_ptr<Type>> types)
        : ident(std::move(ident)), types(std::move(types)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "EnumField";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        for (const auto& t : types) {
            stream << std::string(indent + 2, ' ');
            t->dump(stream);
            stream << '\n';
        }
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }
};

struct EnumDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    std::vector<std::unique_ptr<EnumField>> fields;

    EnumDecl(std::unique_ptr<Identifier> ident,
             std::vector<std::unique_ptr<EnumField>> fields)
        : ident(std::move(ident)), fields(std::move(fields)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "EnumDecl";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        for (const auto& field : fields)
            field->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }

    void declare_type(SymbolTable* syms) override {
        const auto name = ident->to_string();

        const bool is_unique =
            syms->declare_type(ident, std::make_shared<EnumType>(ident));
        if (!is_unique) {
            valid = false;
            return;
        }

        auto* enum_type = dynamic_cast<EnumType*>(syms->get_type(name).get());

        for (const auto& field : fields) {
            if (!enum_type->define_field(field->ident->to_string(),
                                         field->types)) {
                syms->diag.emit(field->ident->tok.get_span(),
                                DiagnosticKind::DuplicateField,
                                ident->to_string(), field->ident->to_string());
                valid = false;
            }
        }
    }

    void resolve_sym(SymbolTable* syms) override {
        for (const auto& field : fields) {
            for (auto& field_type : field->types) {
                if (field_type->is_unknown() &&
                    !syms->resolve_unk_type(field_type)) {
                    valid = false;
                }
            }
        }
    }
};

struct ConstDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    std::shared_ptr<Type> type;
    std::unique_ptr<Expr> val;

    ConstDecl(std::unique_ptr<Identifier> ident, std::shared_ptr<Type> type,
              std::unique_ptr<Expr> val)
        : ident(std::move(ident)), type(std::move(type)),
          val(std::move(val)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ConstDecl";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        stream << std::string(indent + 2, ' ');
        type->dump(stream);
        stream << '\n';
        if (val)
            val->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }

    void declare_type(SymbolTable* syms) override {
        valid = syms->declare_global_var(
            ident, std::make_shared<VariableType>(type, true));
    }

    void resolve_sym(SymbolTable* syms) override {
        if (!type->is_unknown()) {
            return;
        }

        if (!syms->resolve_unk_type(type)) {
            valid = false;
            return;
        }

        auto* const_type = dynamic_cast<VariableType*>(
            syms->get_global_var(ident->to_string()).get());
        const_type->replace_type(type);
    }
};

struct StaticDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    std::shared_ptr<Type> type;
    std::unique_ptr<Expr> val;

    StaticDecl(std::unique_ptr<Identifier> ident, std::shared_ptr<Type> type,
               std::unique_ptr<Expr> val)
        : ident(std::move(ident)), type(std::move(type)),
          val(std::move(val)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "StaticDecl";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        stream << std::string(indent + 2, ' ');
        type->dump(stream);
        stream << '\n';
        if (val)
            val->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override {
        if (is_valid()) {
            visitor.visit(*this);
        }
    }

    void declare_type(SymbolTable* syms) override {
        valid = syms->declare_global_var(
            ident, std::make_shared<VariableType>(type, false, true));
    }

    void resolve_sym(SymbolTable* syms) override {
        if (!type->is_unknown()) {
            return;
        }

        if (!syms->resolve_unk_type(type)) {
            valid = false;
            return;
        }

        auto* static_type = dynamic_cast<VariableType*>(
            syms->get_global_var(ident->to_string()).get());
        static_type->replace_type(type);
    }
};