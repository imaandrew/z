#pragma once

#include "diagnostics.h"
#include "scope.h"
#include "src_mgr.h"
#include "sym_table.h"
#include "token.h"
#include "type.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace z::ast {

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

struct IntExpr;
struct FloatExpr;
struct BoolExpr;
struct PrefixExpr;
struct PostfixExpr;
struct BinaryExpr;
struct TernaryExpr;
struct CallExpr;
struct ArrayExpr;
struct FieldExpr;
struct ArrayInitExpr;
struct StructExprField;
struct StructInitExpr;
struct TupleExpr;
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
struct CharExpr;
struct StructField;
struct StructDecl;
struct EnumField;
struct EnumDecl;
struct ConstDecl;
struct StaticDecl;

enum class ASTKind : std::uint8_t {
    Identifier,
    IntExpr,
    FloatExpr,
    BoolExpr,
    PrefixExpr,
    PostfixExpr,
    BinaryExpr,
    TernaryExpr,
    CallExpr,
    ArrayExpr,
    FieldExpr,
    ArrayInitExpr,
    StructExprField,
    StructInitExpr,
    TupleExpr,
    Block,
    Param,
    FuncDecl,
    BreakStmt,
    ContinueStmt,
    ForExpr,
    LetStmt,
    ReturnStmt,
    ElseExpr,
    IfExpr,
    LoopExpr,
    WhileExpr,
    StringExpr,
    CharExpr,
    StructField,
    StructDecl,
    EnumField,
    EnumDecl,
    ConstDecl,
    StaticDecl
};

class ASTVisitor {
public:
    ASTVisitor() = default;
    virtual ~ASTVisitor() = default;
    ASTVisitor(const ASTVisitor&) = delete;
    ASTVisitor& operator=(const ASTVisitor&) = delete;
    ASTVisitor(ASTVisitor&&) = delete;
    ASTVisitor& operator=(ASTVisitor&&) = delete;
    virtual void visit(IntExpr&) = 0;
    virtual void visit(FloatExpr&) = 0;
    virtual void visit(BoolExpr&) = 0;
    virtual void visit(PrefixExpr&) = 0;
    virtual void visit(PostfixExpr&) = 0;
    virtual void visit(BinaryExpr&) = 0;
    virtual void visit(TernaryExpr&) = 0;
    virtual void visit(CallExpr&) = 0;
    virtual void visit(ArrayExpr&) = 0;
    virtual void visit(FieldExpr&) = 0;
    virtual void visit(ArrayInitExpr&) = 0;
    virtual void visit(StructExprField&) = 0;
    virtual void visit(StructInitExpr&) = 0;
    virtual void visit(TupleExpr&) = 0;
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
    virtual void visit(CharExpr&) = 0;
    virtual void visit(StructField&) = 0;
    virtual void visit(StructDecl&) = 0;
    virtual void visit(EnumField&) = 0;
    virtual void visit(EnumDecl&) = 0;
    virtual void visit(ConstDecl&) = 0;
    virtual void visit(StaticDecl&) = 0;
};

struct ASTNode {
    std::shared_ptr<type::Type> node_type = nullptr;
    Span span;
    const ASTKind kind;
    bool valid = true;

    ASTNode(ASTKind kind, Span span) : span(span), kind(kind) {}
    virtual ~ASTNode() = default;

    ASTNode(const ASTNode&) = delete;
    ASTNode& operator=(const ASTNode&) = delete;
    ASTNode(ASTNode&&) = delete;
    ASTNode& operator=(ASTNode&&) = delete;

    [[nodiscard]] bool is_valid() const { return valid; }
    void mark_invalid() { valid = false; }
    [[nodiscard]] bool has_type() const { return node_type != nullptr; }
    [[nodiscard]] Span get_span() const { return span; }
    [[nodiscard]] ASTKind get_kind() const { return kind; }
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

template <typename T> bool isa(const ASTNode* node) {
    return node->get_kind() == T::Kind;
}

template <typename T> T* dyn_cast(ASTNode* node) {
    if (isa<T>(node))
        return static_cast<T*>(node);
    return nullptr;
}

template <typename T> const T* dyn_cast(const ASTNode* node) {
    if (isa<T>(node))
        return static_cast<const T*>(node);
    return nullptr;
}

template <typename T> T* cast(ASTNode* node) {
    assert(isa<T>(node) && "Invalid cast");
    return static_cast<T*>(node);
}

template <typename T> const T* cast(const ASTNode* node) {
    assert(isa<T>(node) && "Invalid cast");
    return static_cast<T*>(node);
}

struct Stmt : ASTNode {
    Stmt(ASTKind kind, Span span) : ASTNode(kind, span) {};
    ~Stmt() override = default;
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;
    Stmt(Stmt&&) = delete;
    Stmt& operator=(Stmt&&) = delete;
};

struct Expr : Stmt {
    Expr(ASTKind kind, Span span) : Stmt(kind, span) {};
    ~Expr() override = default;
    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;
    Expr(Expr&&) = delete;
    Expr& operator=(Expr&&) = delete;

    [[nodiscard]] virtual bool is_assignable() const { return false; }
};

struct Identifier final : Expr {
    Token tok;
    std::string_view ident;

    static constexpr ASTKind Kind = ASTKind::Identifier;

    Identifier(const Token& tok, std::string_view ident)
        : Expr(Kind, tok.get_span()), tok(tok), ident(ident) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "Identifier "
               << source->get_string(tok.get_span());
        dump_type(stream);
    }

    [[nodiscard]] std::string to_string() const { return std::string(ident); }

    [[nodiscard]] std::string_view get_ident() const { return ident; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    [[nodiscard]] bool is_assignable() const override { return true; }
};

struct IntExpr final : Expr {
    Token tok;
    unsigned long long val;
    static constexpr ASTKind Kind = ASTKind::IntExpr;

    IntExpr(const Token& tok, const unsigned long long val)
        : Expr(Kind, tok.get_span()), tok(tok), val(val) {};

    void dump(SourceManager* /* source */, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "IntExpr " << val;
        dump_type(stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct FloatExpr final : Expr {
    Token tok;
    double val;
    static constexpr ASTKind Kind = ASTKind::FloatExpr;

    FloatExpr(const Token& tok, const double val)
        : Expr(Kind, tok.get_span()), tok(tok), val(val) {};

    void dump(SourceManager* /* source */, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "FloatExpr " << val;
        dump_type(stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct BoolExpr final : Expr {
    Token tok;
    bool val;

    static constexpr ASTKind Kind = ASTKind::BoolExpr;

    BoolExpr(const Token& tok, const bool val)
        : Expr(Kind, tok.get_span()), tok(tok), val(val) {};

    void dump(SourceManager* /* source */, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "BoolExpr "
               << (val ? "true" : "false");
        dump_type(stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

// NOLINTBEGIN(readability-identifier-length)

struct PrefixExpr final : Expr {
    Token op;
    std::shared_ptr<Expr> expr;

    static constexpr ASTKind Kind = ASTKind::PrefixExpr;

    PrefixExpr(const Token& op, std::unique_ptr<Expr> expr)
        : Expr(Kind, op.get_span() + expr->span), op(op),
          expr(std::move(expr)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "PrefixExpr "
               << source->get_string(op.get_span());
        dump_type(stream);
        expr->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct PostfixExpr final : Expr {
    Token op;
    std::unique_ptr<Expr> expr;

    static constexpr ASTKind Kind = ASTKind::PostfixExpr;

    PostfixExpr(const Token& op, std::unique_ptr<Expr> expr)
        : Expr(Kind, op.get_span() + expr->span), op(op),
          expr(std::move(expr)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "PostfixExpr "
               << source->get_string(op.get_span());
        dump_type(stream);
        expr->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct BinaryExpr final : Expr {
    Token op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

    static constexpr ASTKind Kind = ASTKind::BinaryExpr;

    BinaryExpr(const Token& op, std::unique_ptr<Expr> lhs,
               std::unique_ptr<Expr> rhs)
        : Expr(Kind, lhs->span + rhs->span), op(op), lhs(std::move(lhs)),
          rhs(std::move(rhs)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "BinaryExpr "
               << source->get_string(op.get_span());
        dump_type(stream);
        lhs->dump(source, indent + 2, stream);
        rhs->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct TernaryExpr final : Expr {
    Token op;
    Token op2;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> mhs;
    std::unique_ptr<Expr> rhs;

    static constexpr ASTKind Kind = ASTKind::TernaryExpr;

    TernaryExpr(const Token& op, const Token& op2, std::unique_ptr<Expr> lhs,
                std::unique_ptr<Expr> mhs, std::unique_ptr<Expr> rhs)
        : Expr(Kind, lhs->span + rhs->span), op(op), op2(op2),
          lhs(std::move(lhs)), mhs(std::move(mhs)), rhs(std::move(rhs)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "TernaryExpr "
               << source->get_string(op.get_span());
        dump_type(stream);
        lhs->dump(source, indent + 2, stream);
        mhs->dump(source, indent + 2, stream);
        rhs->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

// NOLINTEND(readability-identifier-length)

struct CallExpr final : Expr {
    std::unique_ptr<Expr> ident;
    std::vector<std::unique_ptr<Expr>> args;

    static constexpr ASTKind Kind = ASTKind::CallExpr;

    CallExpr(Span span, std::unique_ptr<Expr> func,
             std::vector<std::unique_ptr<Expr>> args)
        : Expr(Kind, span), ident(std::move(func)), args(std::move(args)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "CallExpr";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        for (const auto& arg : args) {
            arg->dump(source, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct ArrayExpr final : Expr {
    std::unique_ptr<Expr> ident;
    std::unique_ptr<Expr> val;

    static constexpr ASTKind Kind = ASTKind::ArrayExpr;

    ArrayExpr(Span span, std::unique_ptr<Expr> ident)
        : Expr(Kind, span), ident(std::move(ident)) {};

    ArrayExpr(Span span, std::unique_ptr<Expr> ident, std::unique_ptr<Expr> val)
        : Expr(Kind, span), ident(std::move(ident)), val(std::move(val)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ArrayExpr";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        if (val)
            val->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    [[nodiscard]] bool is_assignable() const override { return true; }
};

struct FieldExpr final : Expr {
    std::unique_ptr<Expr> container;
    std::unique_ptr<Identifier> field;

    static constexpr ASTKind Kind = ASTKind::FieldExpr;

    FieldExpr(std::unique_ptr<Expr> container,
              std::unique_ptr<Identifier> field)
        : Expr(Kind, container->get_span() + field->get_span()),
          container(std::move(container)), field(std::move(field)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "FieldExpr";
        dump_type(stream);
        container->dump(source, indent + 2, stream);
        field->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    [[nodiscard]] bool is_assignable() const override { return true; }
};

struct ArrayInitExpr final : Expr {
    std::vector<std::unique_ptr<Expr>> vals;

    static constexpr ASTKind Kind = ASTKind::ArrayInitExpr;

    ArrayInitExpr(Span span, std::vector<std::unique_ptr<Expr>> vals)
        : Expr(Kind, span), vals(std::move(vals)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ArrayInitExpr";
        dump_type(stream);
        for (const auto& val : vals) {
            val->dump(source, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct StructExprField final : Expr {
    std::unique_ptr<Identifier> ident;
    std::unique_ptr<Expr> val;

    static constexpr ASTKind Kind = ASTKind::StructExprField;

    StructExprField(std::unique_ptr<Identifier> ident,
                    std::unique_ptr<Expr> val)
        : Expr(Kind, ident->get_span() + val->get_span()),
          ident(std::move(ident)), val(std::move(val)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "StructExprField";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        val->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct StructInitExpr final : Expr {
    std::unique_ptr<Expr> ident;
    std::vector<std::unique_ptr<StructExprField>> fields;

    static constexpr ASTKind Kind = ASTKind::StructInitExpr;

    StructInitExpr(Span span, std::unique_ptr<Expr> ident,
                   std::vector<std::unique_ptr<StructExprField>> fields)
        : Expr(Kind, span), ident(std::move(ident)),
          fields(std::move(fields)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "StructInitExpr";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        for (const auto& field : fields) {
            field->dump(source, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct TupleExpr final : Expr {
    std::unique_ptr<Expr> first;
    std::unique_ptr<Expr> second;

    static constexpr ASTKind Kind = ASTKind::TupleExpr;

    TupleExpr(Span span, std::unique_ptr<Expr> first,
              std::unique_ptr<Expr> second)
        : Expr(Kind, span), first(std::move(first)),
          second(std::move(second)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "TupleExpr";
        dump_type(stream);
        first->dump(source, indent + 2, stream);
        second->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct Block final : Expr {
    std::vector<std::unique_ptr<Stmt>> stmts;
    ScopeContext ctxt;

    static constexpr ASTKind Kind = ASTKind::Block;

    Block(Span span, std::vector<std::unique_ptr<Stmt>> stmts)
        : Expr(Kind, span), stmts(std::move(stmts)) {};

    ScopeContext* get_scope_ctxt() { return &ctxt; }

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "Block";
        dump_type(stream);
        for (const auto& stmt : stmts) {
            stmt->dump(source, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct Param final : Expr {
    std::unique_ptr<Identifier> name;
    std::shared_ptr<type::Type> type;

    static constexpr ASTKind Kind = ASTKind::Param;

    Param(Span span, std::unique_ptr<Identifier> name,
          std::shared_ptr<type::Type> type)
        : Expr(Kind, span), name(std::move(name)), type(std::move(type)) {};

    void dump(SourceManager* source, int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "Param";
        dump_type(stream);
        name->dump(source, indent + 2, stream);
        type->dump(stream);
        stream << '\n';
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct Decl : ASTNode {
    Decl(ASTKind kind, Span span) : ASTNode(kind, span) {}
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
    std::shared_ptr<type::Type> ret;
    std::unique_ptr<Block> body;

    static constexpr ASTKind Kind = ASTKind::FuncDecl;

    FuncDecl(Span span, std::unique_ptr<Identifier> name,
             std::optional<std::shared_ptr<Identifier>> impl_type,
             std::vector<std::unique_ptr<Param>> params,
             std::shared_ptr<type::Type> ret, std::unique_ptr<Block> body)
        : Decl(Kind, span), name(std::move(name)),
          impl_type(std::move(impl_type)), params(std::move(params)),
          ret(std::move(ret)), body(std::move(body)) {};

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

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    void declare_type(SymbolTable* syms) override {
        auto param_types = std::vector<std::shared_ptr<type::Type>>();
        for (const auto& param : params) {
            param_types.push_back(param->type);
        }

        valid = syms->declare_func(
            get_abs_name(), name->tok,
            std::make_shared<type::FunctionType>(param_types, ret));
    }

    void resolve_sym(SymbolTable* syms) override {
        auto* func_type =
            cast<type::FunctionType>(syms->get_func(get_abs_name()).get());

        for (size_t i = 0; i < params.size(); i++) {
            if (params[i]->type->is_unknown()) {
                if (syms->resolve_unk_type(params[i]->type)) {
                    func_type->get_params()[i] = params[i]->type;
                } else {
                    valid = false;
                }
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

    static constexpr ASTKind Kind = ASTKind::BreakStmt;

    BreakStmt(Span span, const Token& tok) : Stmt(Kind, span), tok(tok) {};
    BreakStmt(Span span, const Token& tok, std::unique_ptr<Expr> expr)
        : Stmt(Kind, span), tok(tok), expr(std::move(expr)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "BreakStmt";
        dump_type(stream);
        if (expr)
            expr->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct ContinueStmt final : Stmt {
    Token tok;

    static constexpr ASTKind Kind = ASTKind::ContinueStmt;

    ContinueStmt(Span span, const Token& tok) : Stmt(Kind, span), tok(tok) {};

    void dump(SourceManager* /* source */, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ContinueStmt";
        dump_type(stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct ForExpr final : Expr {
    std::unique_ptr<Identifier> ident;
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;

    static constexpr ASTKind Kind = ASTKind::ForExpr;

    ForExpr(Span span, std::unique_ptr<Identifier> ident,
            std::unique_ptr<Expr> expr, std::unique_ptr<Block> block)
        : Expr(Kind, span), ident(std::move(ident)), expr(std::move(expr)),
          block(std::move(block)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ForExpr";
        dump_type(stream);
        expr->dump(source, indent + 2, stream);
        block->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct LetStmt final : Stmt {
    std::unique_ptr<Identifier> ident;
    std::shared_ptr<type::Type> type;
    std::shared_ptr<Expr> val;
    std::optional<Span> eq;

    static constexpr ASTKind Kind = ASTKind::LetStmt;

    LetStmt(Span span, std::unique_ptr<Identifier> ident)
        : Stmt(Kind, span), ident(std::move(ident)) {};

    LetStmt(Span span, std::unique_ptr<Identifier> ident,
            std::shared_ptr<type::Type> type)
        : Stmt(Kind, span), ident(std::move(ident)), type(std::move(type)) {};

    LetStmt(Span span, std::unique_ptr<Identifier> ident,
            std::shared_ptr<Expr> val, Span eq)
        : Stmt(Kind, span), ident(std::move(ident)), val(std::move(val)),
          eq(eq) {};

    LetStmt(Span span, std::unique_ptr<Identifier> ident,
            std::shared_ptr<type::Type> type, std::shared_ptr<Expr> val,
            Span eq)
        : Stmt(Kind, span), ident(std::move(ident)), type(std::move(type)),
          val(std::move(val)), eq(eq) {};

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

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct ReturnStmt final : Stmt {
    std::unique_ptr<Expr> expr;

    static constexpr ASTKind Kind = ASTKind::ReturnStmt;

    explicit ReturnStmt(Span span) : Stmt(Kind, span) {}
    ReturnStmt(Span span, std::unique_ptr<Expr> expr)
        : Stmt(Kind, span), expr(std::move(expr)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ReturnStmt";
        dump_type(stream);
        if (expr)
            expr->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct ElseExpr final : Expr {
    std::unique_ptr<Expr> if_expr;
    std::unique_ptr<Block> block;

    static constexpr ASTKind Kind = ASTKind::ElseExpr;

    ElseExpr(Span span, std::unique_ptr<Block> block)
        : Expr(Kind, span), block(std::move(block)) {};
    ElseExpr(Span span, std::unique_ptr<Expr> if_expr)
        : Expr(Kind, span), if_expr(std::move(if_expr)) {};

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

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct IfExpr final : Expr {
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;
    std::unique_ptr<ElseExpr> else_expr;

    static constexpr ASTKind Kind = ASTKind::IfExpr;

    IfExpr(Span span, std::unique_ptr<Expr> expr, std::unique_ptr<Block> block)
        : Expr(Kind, span), expr(std::move(expr)), block(std::move(block)) {};
    IfExpr(Span span, std::unique_ptr<Expr> expr, std::unique_ptr<Block> block,
           std::unique_ptr<ElseExpr> else_expr)
        : Expr(Kind, span), expr(std::move(expr)), block(std::move(block)),
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

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct LoopExpr final : Expr {
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;

    static constexpr ASTKind Kind = ASTKind::LoopExpr;

    LoopExpr(Span span, std::unique_ptr<Expr> expr,
             std::unique_ptr<Block> block)
        : Expr(Kind, span), expr(std::move(expr)), block(std::move(block)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "LoopExpr";
        dump_type(stream);
        if (expr) {
            expr->dump(source, indent + 2, stream);
        }
        block->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct WhileExpr final : Expr {
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;

    static constexpr ASTKind Kind = ASTKind::WhileExpr;

    WhileExpr(Span span, std::unique_ptr<Expr> expr,
              std::unique_ptr<Block> block)
        : Expr(Kind, span), expr(std::move(expr)), block(std::move(block)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "WhileExpr";
        dump_type(stream);
        expr->dump(source, indent + 2, stream);
        block->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct StringExpr final : Expr {
    Span span;

    static constexpr ASTKind Kind = ASTKind::StringExpr;

    explicit StringExpr(Span span) : Expr(Kind, span), span(span) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "String '"
               << source->get_string(span) << '\'';
        dump_type(stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct CharExpr final : Expr {
    Span span;

    static constexpr ASTKind Kind = ASTKind::CharExpr;

    explicit CharExpr(Span span) : Expr(Kind, span), span(span) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "Char '"
               << source->get_string(span) << '\'';
        dump_type(stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct StructField final : ASTNode {
    std::shared_ptr<Identifier> ident;
    std::shared_ptr<type::Type> type;

    static constexpr ASTKind Kind = ASTKind::StructField;

    StructField(Span span, std::shared_ptr<Identifier> ident,
                std::shared_ptr<type::Type> type)
        : ASTNode(Kind, span), ident(std::move(ident)),
          type(std::move(type)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "StructField";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        stream << std::string(indent + 2, ' ');
        type->dump(stream);
        stream << '\n';
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct StructDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    std::vector<std::unique_ptr<StructField>> fields;

    static constexpr ASTKind Kind = ASTKind::StructDecl;

    StructDecl(Span span, std::unique_ptr<Identifier> ident,
               std::vector<std::unique_ptr<StructField>> fields)
        : Decl(Kind, span), ident(std::move(ident)),
          fields(std::move(fields)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "StructDecl";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        for (const auto& field : fields)
            field->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    void declare_type(SymbolTable* syms) override {
        const auto name = ident->get_ident();

        if (!syms->declare_type(ident,
                                std::make_shared<type::StructType>(ident))) {
            valid = false;
            return;
        }

        auto* struct_type = cast<type::StructType>(syms->get_type(name).get());

        for (const auto& field : fields) {
            const bool is_unique = struct_type->define_field(
                field->ident->get_ident(), field->type);
            if (!is_unique) {
                syms->diag.emit(field->ident->tok.get_span(),
                                DiagnosticKind::DuplicateField,
                                ident->to_string(), field->ident->to_string());
                valid = false;
            }
        }
    }

    void resolve_sym(SymbolTable* syms) override {
        if (!is_valid())
            return;

        auto* struct_type =
            cast<type::StructType>(syms->get_type(ident->get_ident()).get());

        for (const auto& field : fields) {
            if (field->type->is_unknown()) {
                if (syms->resolve_unk_type(field->type)) {
                    struct_type->replace_field_type(field->ident->get_ident(),
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
    std::vector<std::shared_ptr<type::Type>> types;

    static constexpr ASTKind Kind = ASTKind::EnumField;

    EnumField(Span span, std::unique_ptr<Identifier> ident)
        : ASTNode(Kind, span), ident(std::move(ident)) {};
    EnumField(Span span, std::unique_ptr<Identifier> ident,
              std::vector<std::shared_ptr<type::Type>> types)
        : ASTNode(Kind, span), ident(std::move(ident)),
          types(std::move(types)) {};

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

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct EnumDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    std::vector<std::unique_ptr<EnumField>> fields;

    static constexpr ASTKind Kind = ASTKind::EnumDecl;

    EnumDecl(Span span, std::unique_ptr<Identifier> ident,
             std::vector<std::unique_ptr<EnumField>> fields)
        : Decl(Kind, span), ident(std::move(ident)),
          fields(std::move(fields)) {};

    void dump(SourceManager* source, const int indent,
              std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "EnumDecl";
        dump_type(stream);
        ident->dump(source, indent + 2, stream);
        for (const auto& field : fields)
            field->dump(source, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    void declare_type(SymbolTable* syms) override {
        const auto name = ident->get_ident();

        const bool is_unique =
            syms->declare_type(ident, std::make_shared<type::EnumType>(ident));
        if (!is_unique) {
            valid = false;
            return;
        }

        auto* enum_type = cast<type::EnumType>(syms->get_type(name).get());

        for (const auto& field : fields) {
            if (!enum_type->define_field(field->ident->get_ident(),
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
    std::shared_ptr<type::Type> type;
    std::unique_ptr<Expr> val;

    static constexpr ASTKind Kind = ASTKind::ConstDecl;

    ConstDecl(Span span, std::unique_ptr<Identifier> ident,
              std::shared_ptr<type::Type> type, std::unique_ptr<Expr> val)
        : Decl(Kind, span), ident(std::move(ident)), type(std::move(type)),
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

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    void declare_type(SymbolTable* syms) override {
        valid = syms->declare_var(ident, type);
    }

    void resolve_sym(SymbolTable* syms) override {
        if (!type->is_unknown()) {
            return;
        }

        if (!syms->resolve_unk_type(type)) {
            valid = false;
            return;
        }
    }
};

struct StaticDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    std::shared_ptr<type::Type> type;
    std::unique_ptr<Expr> val;

    static constexpr ASTKind Kind = ASTKind::StaticDecl;

    StaticDecl(Span span, std::unique_ptr<Identifier> ident,
               std::shared_ptr<type::Type> type, std::unique_ptr<Expr> val)
        : Decl(Kind, span), ident(std::move(ident)), type(std::move(type)),
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

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    void declare_type(SymbolTable* syms) override {
        valid = syms->declare_var(ident, type);
    }

    void resolve_sym(SymbolTable* syms) override {
        if (!type->is_unknown()) {
            return;
        }

        if (!syms->resolve_unk_type(type)) {
            valid = false;
            return;
        }
    }
};
} // namespace z::ast
