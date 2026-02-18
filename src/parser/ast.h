#pragma once

#include "core/colour.h"
#include "core/panic.h"
#include "core/string_pool.h"
#include "core/zctxt.h"
#include "diag/diagnostics.h"
#include "diag/src_mgr.h"
#include "lexer/token.h"
#include "sema/scope.h"
#include "sema/sym_table.h"
#include "type/type.h"
#include "type/type_ref.h"
#include <cassert>
#include <cstdint>
#include <generator>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

inline void print_indent(std::ostream& os, int indent) {
    std::print(os, "{:>{}}", "", indent);
}

namespace z::ast {

enum class BinOpPrecedence : std::uint8_t {
    Unknown,
    Assignment,
    Range,
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

enum class UnOp : std::uint8_t { Inc, Dec, Neg, BitNot, LogicNot };

static constexpr UnOp tok_kind_to_unop(TokenKind kind) {
    switch (kind) {
    case TokenKind::PlusPlus:
        return UnOp::Inc;
    case TokenKind::MinusMinus:
        return UnOp::Dec;
    case TokenKind::Minus:
        return UnOp::Neg;
    case TokenKind::Not:
        return UnOp::BitNot;
    case TokenKind::LogicalNot:
        return UnOp::LogicNot;
    default:
        panic("Cannot convert TokenKind to unary operator");
    }
}

enum class BinOp : std::uint8_t {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    BitXor,
    BitAnd,
    BitOr,
    LogicAnd,
    LogicOr,
    Shl,
    Shr,
    Range,
    RangeEq,
    AddEq,
    SubEq,
    MulEq,
    DivEq,
    ModEq,
    BitXorEq,
    BitAndEq,
    BitOrEq,
    ShlEq,
    ShrEq,
    Eq,
    EqEq,
    Ne,
    Gt,
    Lt,
    Ge,
    Le,
    ColonColon,
};

static constexpr BinOp tok_kind_to_binop(TokenKind kind) {
    switch (kind) {
    case TokenKind::Plus:
        return BinOp::Add;
    case TokenKind::Minus:
        return BinOp::Sub;
    case TokenKind::Star:
        return BinOp::Mul;
    case TokenKind::Slash:
        return BinOp::Div;
    case TokenKind::Percent:
        return BinOp::Mod;
    case TokenKind::Caret:
        return BinOp::BitXor;
    case TokenKind::And:
        return BinOp::BitAnd;
    case TokenKind::Or:
        return BinOp::BitOr;
    case TokenKind::AndAnd:
        return BinOp::LogicAnd;
    case TokenKind::OrOr:
        return BinOp::LogicOr;
    case TokenKind::Shl:
        return BinOp::Shl;
    case TokenKind::Shr:
        return BinOp::Shr;
    case TokenKind::Range:
        return BinOp::Range;
    case TokenKind::RangeEq:
        return BinOp::RangeEq;
    case TokenKind::PlusEq:
        return BinOp::AddEq;
    case TokenKind::MinusEq:
        return BinOp::SubEq;
    case TokenKind::StarEq:
        return BinOp::MulEq;
    case TokenKind::SlashEq:
        return BinOp::DivEq;
    case TokenKind::PercentEq:
        return BinOp::ModEq;
    case TokenKind::CaretEq:
        return BinOp::BitXorEq;
    case TokenKind::AndEq:
        return BinOp::BitAndEq;
    case TokenKind::OrEq:
        return BinOp::BitOrEq;
    case TokenKind::ShlEq:
        return BinOp::ShlEq;
    case TokenKind::ShrEq:
        return BinOp::ShrEq;
    case TokenKind::Eq:
        return BinOp::Eq;
    case TokenKind::EqEq:
        return BinOp::EqEq;
    case TokenKind::Ne:
        return BinOp::Ne;
    case TokenKind::Gt:
        return BinOp::Gt;
    case TokenKind::Lt:
        return BinOp::Lt;
    case TokenKind::Ge:
        return BinOp::Ge;
    case TokenKind::Le:
        return BinOp::Le;
    case TokenKind::ColonColon:
        return BinOp::ColonColon;
    default:
        panic("Cannot convert TokenKind to binary operator");
    }
}

static constexpr bool is_assignment_op(BinOp op) {
    switch (op) {
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
        return true;
    default:
        return false;
    }
}

static constexpr bool is_division_op(BinOp op) {
    switch (op) {
    case BinOp::Div:
    case BinOp::DivEq:
    case BinOp::Mod:
    case BinOp::ModEq:
        return true;
    default:
        return false;
    }
}

struct IntExpr;
struct FloatExpr;
struct BoolExpr;
struct PrefixExpr;
struct PostfixExpr;
struct BinaryExpr;
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
struct SourceFileDecl;
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
struct TraitDecl;
struct TypeAliasDecl;
struct TraitFuncDecl;

enum class ASTKind : std::uint8_t {
    Identifier,
    IntExpr,
    FloatExpr,
    BoolExpr,
    PrefixExpr,
    PostfixExpr,
    BinaryExpr,
    CallExpr,
    ArrayExpr,
    FieldExpr,
    ArrayInitExpr,
    StructExprField,
    StructInitExpr,
    TupleExpr,
    Block,
    Param,
    SourceFileDecl,
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
    StaticDecl,
    TraitDecl,
    TypeAliasDecl,
    TraitFuncDecl,
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
    virtual void visit(SourceFileDecl&) = 0;
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
    virtual void visit(TraitDecl&) = 0;
    virtual void visit(TypeAliasDecl&) = 0;
    virtual void visit(TraitFuncDecl&) = 0;
};

struct ASTNode {
    type::TypeRef node_type;
    Span span;
    const ASTKind kind;

    ASTNode(ASTKind kind, Span span) : span(span), kind(kind) {}
    virtual ~ASTNode() = default;

    ASTNode(const ASTNode&) = delete;
    ASTNode& operator=(const ASTNode&) = delete;
    ASTNode(ASTNode&&) = delete;
    ASTNode& operator=(ASTNode&&) = delete;

    [[nodiscard]] bool has_type() const { return node_type.is_valid(); }
    [[nodiscard]] Span get_span() const { return span; }
    [[nodiscard]] ASTKind get_kind() const { return kind; }
    virtual void accept(ASTVisitor& visitor) = 0;

    virtual void dump(ZContext* ctxt, int indent = 0,
                      std::ostream& stream = std::cout) const = 0;

    virtual std::generator<ASTNode*> children() { co_return; }

    void dump_type(ZContext* ctxt, std::ostream& stream = std::cout) const {
        if (node_type.is_initialized()) {
            std::print(stream, " {}<{}>{}", colour::CYAN,
                       ctxt->ty->get(node_type)->basic_name(ctxt),
                       colour::RESET);
        }
        stream << '\n';
    }

    void print_header(std::ostream& stream, int indent, const char* name,
                      z::ZContext* ctxt) const {
        print_indent(stream, indent);
        std::print(stream, "{}{}{}", colour::GREEN, name, colour::RESET);
        dump_type(ctxt, stream);
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
    bool semi_terminated = false;
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
    StringID ident;

    static constexpr ASTKind Kind = ASTKind::Identifier;

    Identifier(const Token& tok, StringID ident)
        : Expr(Kind, tok.get_span()), ident(ident) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_indent(stream, indent);
        std::print(stream, "{}Identifier{} {}'{}'{}", colour::BOLD_GREEN,
                   colour::RESET, colour::YELLOW,
                   ctxt->src->get_string(get_span()), colour::RESET);
        dump_type(ctxt, stream);
    }

    [[nodiscard]] StringID get_id() const { return ident; }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    [[nodiscard]] bool is_assignable() const override { return true; }
};

struct IntExpr final : Expr {
    std::uint64_t val;
    static constexpr ASTKind Kind = ASTKind::IntExpr;

    IntExpr(const Token& tok, std::uint64_t val)
        : Expr(Kind, tok.get_span()), val(val) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_indent(stream, indent);
        std::print(stream, "{}IntExpr{} {}{}{}", colour::BOLD_GREEN,
                   colour::RESET, colour::YELLOW, val, colour::RESET);
        dump_type(ctxt, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct FloatExpr final : Expr {
    double val;
    static constexpr ASTKind Kind = ASTKind::FloatExpr;

    FloatExpr(const Token& tok, const double val)
        : Expr(Kind, tok.get_span()), val(val) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_indent(stream, indent);
        std::print(stream, "{}FloatExpr{} {}{}{}", colour::BOLD_GREEN,
                   colour::RESET, colour::YELLOW, val, colour::RESET);
        dump_type(ctxt, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct BoolExpr final : Expr {
    bool val;

    static constexpr ASTKind Kind = ASTKind::BoolExpr;

    BoolExpr(const Token& tok, const bool val)
        : Expr(Kind, tok.get_span()), val(val) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_indent(stream, indent);
        std::print(stream, "{}BoolExpr{} {}{}{}", colour::BOLD_GREEN,
                   colour::RESET, colour::YELLOW, val, colour::RESET);
        dump_type(ctxt, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

// NOLINTBEGIN(readability-identifier-length)

struct PrefixExpr final : Expr {
    UnOp op;
    Span op_span;
    std::unique_ptr<Expr> expr;

    static constexpr ASTKind Kind = ASTKind::PrefixExpr;

    PrefixExpr(const Token& op, std::unique_ptr<Expr> expr)
        : Expr(Kind, op.get_span() + expr->span),
          op(tok_kind_to_unop(op.get_kind())), op_span(op.get_span()),
          expr(std::move(expr)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_indent(stream, indent);
        std::print(stream, "{}PrefixExpr{} {}'{}'{}", colour::BOLD_GREEN,
                   colour::RESET, colour::YELLOW,
                   ctxt->src->get_string(op_span), colour::RESET);
        dump_type(ctxt, stream);
        expr->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override { co_yield expr.get(); }
};

struct PostfixExpr final : Expr {
    UnOp op;
    Span op_span;
    std::unique_ptr<Expr> expr;

    static constexpr ASTKind Kind = ASTKind::PostfixExpr;

    PostfixExpr(const Token& op, std::unique_ptr<Expr> expr)
        : Expr(Kind, op.get_span() + expr->span),
          op(tok_kind_to_unop(op.get_kind())), op_span(op.get_span()),
          expr(std::move(expr)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_indent(stream, indent);
        std::print(stream, "{}PostfixExpr{} {}'{}'{}", colour::BOLD_GREEN,
                   colour::RESET, colour::YELLOW,
                   ctxt->src->get_string(op_span), colour::RESET);
        dump_type(ctxt, stream);
        expr->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override { co_yield expr.get(); }
};

struct BinaryExpr final : Expr {
    BinOp op;
    Span op_span;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

    static constexpr ASTKind Kind = ASTKind::BinaryExpr;

    BinaryExpr(const Token& op, std::unique_ptr<Expr> lhs,
               std::unique_ptr<Expr> rhs)
        : Expr(Kind, lhs->span + rhs->span),
          op(tok_kind_to_binop(op.get_kind())), op_span(op.get_span()),
          lhs(std::move(lhs)), rhs(std::move(rhs)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_indent(stream, indent);
        std::print(stream, "{}BinaryExpr{} {}'{}'{}", colour::BOLD_GREEN,
                   colour::RESET, colour::YELLOW,
                   ctxt->src->get_string(op_span), colour::RESET);
        dump_type(ctxt, stream);
        lhs->dump(ctxt, indent + 2, stream);
        rhs->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield lhs.get();
        co_yield rhs.get();
    }
};

// NOLINTEND(readability-identifier-length)

struct CallExpr final : Expr {
    std::unique_ptr<Expr> ident;
    std::vector<std::unique_ptr<Expr>> args;

    static constexpr ASTKind Kind = ASTKind::CallExpr;

    CallExpr(Span span, std::unique_ptr<Expr> func,
             std::vector<std::unique_ptr<Expr>> args)
        : Expr(Kind, span), ident(std::move(func)), args(std::move(args)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "CallExpr", ctxt);
        ident->dump(ctxt, indent + 2, stream);
        for (const auto& arg : args) {
            arg->dump(ctxt, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield ident.get();
        for (const auto& arg : args)
            co_yield arg.get();
    }
};

struct ArrayExpr final : Expr {
    std::unique_ptr<Expr> ident;
    std::unique_ptr<Expr> val;

    static constexpr ASTKind Kind = ASTKind::ArrayExpr;

    ArrayExpr(Span span, std::unique_ptr<Expr> ident, std::unique_ptr<Expr> val)
        : Expr(Kind, span), ident(std::move(ident)), val(std::move(val)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "ArrayExpr", ctxt);
        ident->dump(ctxt, indent + 2, stream);
        val->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield ident.get();
        co_yield val.get();
    }

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

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "FieldExpr", ctxt);
        container->dump(ctxt, indent + 2, stream);
        field->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield container.get();
        co_yield field.get();
    }

    [[nodiscard]] bool is_assignable() const override { return true; }
};

struct ArrayInitExpr final : Expr {
    std::vector<std::unique_ptr<Expr>> vals;

    static constexpr ASTKind Kind = ASTKind::ArrayInitExpr;

    ArrayInitExpr(Span span, std::vector<std::unique_ptr<Expr>> vals)
        : Expr(Kind, span), vals(std::move(vals)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "ArrayInitExpr", ctxt);
        for (const auto& val : vals) {
            val->dump(ctxt, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        for (const auto& val : vals)
            co_yield val.get();
    }
};

struct StructExprField final : Expr {
    std::unique_ptr<Identifier> ident;
    std::unique_ptr<Expr> val;

    static constexpr ASTKind Kind = ASTKind::StructExprField;

    StructExprField(std::unique_ptr<Identifier> ident,
                    std::unique_ptr<Expr> val)
        : Expr(Kind, ident->get_span() + val->get_span()),
          ident(std::move(ident)), val(std::move(val)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "StructExprField", ctxt);
        ident->dump(ctxt, indent + 2, stream);
        val->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield ident.get();
        co_yield val.get();
    }
};

struct StructInitExpr final : Expr {
    std::unique_ptr<Expr> ident;
    std::vector<std::unique_ptr<StructExprField>> fields;

    static constexpr ASTKind Kind = ASTKind::StructInitExpr;

    StructInitExpr(Span span, std::unique_ptr<Expr> ident,
                   std::vector<std::unique_ptr<StructExprField>> fields)
        : Expr(Kind, span), ident(std::move(ident)),
          fields(std::move(fields)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "StructInitExpr", ctxt);
        ident->dump(ctxt, indent + 2, stream);
        for (const auto& field : fields) {
            field->dump(ctxt, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield ident.get();
        for (const auto& field : fields)
            co_yield field.get();
    }
};

struct TupleExpr final : Expr {
    std::unique_ptr<Expr> first;
    std::unique_ptr<Expr> second;

    static constexpr ASTKind Kind = ASTKind::TupleExpr;

    TupleExpr(Span span, std::unique_ptr<Expr> first,
              std::unique_ptr<Expr> second)
        : Expr(Kind, span), first(std::move(first)),
          second(std::move(second)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "TupleExpr", ctxt);
        first->dump(ctxt, indent + 2, stream);
        second->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield first.get();
        co_yield second.get();
    }
};

struct Block final : Expr {
    std::vector<std::unique_ptr<Stmt>> stmts;
    ScopeID scope;

    static constexpr ASTKind Kind = ASTKind::Block;

    Block(Span span, std::vector<std::unique_ptr<Stmt>> stmts, ScopeID scope)
        : Expr(Kind, span), stmts(std::move(stmts)), scope(scope) {};

    [[nodiscard]] ScopeID get_scope_id() const { return scope; }

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "Block", ctxt);
        for (const auto& stmt : stmts) {
            stmt->dump(ctxt, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        for (const auto& stmt : stmts)
            co_yield stmt.get();
    }
};

struct Param final : Expr {
    std::unique_ptr<Identifier> name;
    type::TypeRef type;

    static constexpr ASTKind Kind = ASTKind::Param;

    Param(Span span, std::unique_ptr<Identifier> name, type::TypeRef type)
        : Expr(Kind, span), name(std::move(name)), type(type) {};

    void dump(ZContext* ctxt, int indent, std::ostream& stream) const override {
        print_header(stream, indent, "Param", ctxt);
        name->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override { co_yield name.get(); }
};

struct Decl : ASTNode {
    Decl(ASTKind kind, Span span) : ASTNode(kind, span) {}
    ~Decl() override = default;
    Decl(const Decl&) = delete;
    Decl& operator=(const Decl&) = delete;
    Decl(Decl&&) = delete;
    Decl& operator=(Decl&&) = delete;

    bool valid = true;

    virtual void declare_type(ZContext* ctxt) = 0;
    virtual void resolve_sym(ZContext* ctxt) = 0;
};

struct SourceFileDecl : Decl {
    std::vector<std::unique_ptr<Decl>> decls;
    std::vector<std::unique_ptr<Decl>> const_decls;

    static constexpr ASTKind Kind = ASTKind::SourceFileDecl;

    SourceFileDecl(Span span, std::vector<std::unique_ptr<Decl>> decls,
                   std::vector<std::unique_ptr<Decl>> const_decls)
        : Decl(Kind, span), decls(std::move(decls)),
          const_decls(std::move(const_decls)) {}

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "SourceFileDecl", ctxt);

        print_indent(stream, indent + 2);
        std::println(stream, "{}Consts:{}", colour::DIM, colour::RESET);
        for (const auto& decl : const_decls) {
            decl->dump(ctxt, indent + 2, stream);
        }

        print_indent(stream, indent + 2);
        std::println(stream, "{}Decls:{}", colour::DIM, colour::RESET);
        for (const auto& decl : decls) {
            decl->dump(ctxt, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        for (const auto& decl : const_decls)
            co_yield decl.get();

        for (const auto& decl : decls)
            co_yield decl.get();
    }

    void declare_type(ZContext* /*ctxt*/) override {}
    void resolve_sym(ZContext* /*ctxt*/) override {}
};

struct FuncDecl final : Decl {
    std::unique_ptr<Identifier> name;
    std::vector<std::unique_ptr<Param>> params;
    type::TypeRef ret;
    std::unique_ptr<Block> body;

    static constexpr ASTKind Kind = ASTKind::FuncDecl;

    FuncDecl(Span span, std::unique_ptr<Identifier> name,
             std::vector<std::unique_ptr<Param>> params, type::TypeRef ret,
             std::unique_ptr<Block> body)
        : Decl(Kind, span), name(std::move(name)), params(std::move(params)),
          ret(ret), body(std::move(body)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "FuncDecl", ctxt);
        name->dump(ctxt, indent + 2, stream);
        body->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield name.get();

        for (const auto& param : params)
            co_yield param.get();

        co_yield body.get();
    }

    void declare_type(ZContext* ctxt) override {
        valid = ctxt->syms->declare_func(
            name, ctxt->ty->make<type::TempType>(name->get_id(),
                                                 type::TypeKind::Function));
    }

    void resolve_sym(ZContext* ctxt) override {
        if (!valid)
            return;

        auto t = ctxt->syms->get_func(name->get_id());
        if (!t)
            return;

        auto param_types = std::vector<type::TypeRef>();
        for (const auto& param : params) {
            if (!ctxt->resolve_unk_type(param->type)) {
                return;
            }
            param_types.push_back(param->type);
        }

        if (!ctxt->resolve_unk_type(ret)) {
            return;
        }

        ctxt->ty->replace<type::FunctionType>(*t, param_types, ret);
    }
};

struct BreakStmt final : Stmt {
    std::unique_ptr<Expr> expr;

    static constexpr ASTKind Kind = ASTKind::BreakStmt;

    explicit BreakStmt(Span span) : Stmt(Kind, span) {};
    BreakStmt(Span span, std::unique_ptr<Expr> expr)
        : Stmt(Kind, span), expr(std::move(expr)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "BreakStmt", ctxt);
        if (expr)
            expr->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        if (expr)
            co_yield expr.get();
    }
};

struct ContinueStmt final : Stmt {
    static constexpr ASTKind Kind = ASTKind::ContinueStmt;

    explicit ContinueStmt(Span span) : Stmt(Kind, span) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "ContinueStmt", ctxt);
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

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "ForExpr", ctxt);
        expr->dump(ctxt, indent + 2, stream);
        block->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield ident.get();
        co_yield expr.get();
        co_yield block.get();
    }
};

struct LetStmt final : Stmt {
    std::unique_ptr<Identifier> ident;
    type::TypeRef type;
    std::unique_ptr<Expr> val;
    std::optional<Span> eq;

    static constexpr ASTKind Kind = ASTKind::LetStmt;

    LetStmt(Span span, std::unique_ptr<Identifier> ident)
        : Stmt(Kind, span), ident(std::move(ident)) {};

    LetStmt(Span span, std::unique_ptr<Identifier> ident, type::TypeRef type)
        : Stmt(Kind, span), ident(std::move(ident)), type(type) {};

    LetStmt(Span span, std::unique_ptr<Identifier> ident,
            std::unique_ptr<Expr> val, Span eq)
        : Stmt(Kind, span), ident(std::move(ident)), val(std::move(val)),
          eq(eq) {};

    LetStmt(Span span, std::unique_ptr<Identifier> ident, type::TypeRef type,
            std::unique_ptr<Expr> val, Span eq)
        : Stmt(Kind, span), ident(std::move(ident)), type(type),
          val(std::move(val)), eq(eq) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "LetStmt", ctxt);

        ident->dump(ctxt, indent + 2, stream);
        if (val)
            val->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield ident.get();
        if (val)
            co_yield val.get();
    }
};

struct ReturnStmt final : Stmt {
    std::unique_ptr<Expr> expr;

    static constexpr ASTKind Kind = ASTKind::ReturnStmt;

    explicit ReturnStmt(Span span) : Stmt(Kind, span) {}
    ReturnStmt(Span span, std::unique_ptr<Expr> expr)
        : Stmt(Kind, span), expr(std::move(expr)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "ReturnStmt", ctxt);
        if (expr)
            expr->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        if (expr)
            co_yield expr.get();
    }
};

struct ElseExpr final : Expr {
    std::unique_ptr<Expr> if_expr;
    std::unique_ptr<Block> block;

    static constexpr ASTKind Kind = ASTKind::ElseExpr;

    ElseExpr(Span span, std::unique_ptr<Block> block)
        : Expr(Kind, span), block(std::move(block)) {};
    ElseExpr(Span span, std::unique_ptr<Expr> if_expr)
        : Expr(Kind, span), if_expr(std::move(if_expr)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "ElseExpr", ctxt);

        if (if_expr) {
            if_expr->dump(ctxt, indent + 2, stream);
        } else if (block) {
            block->dump(ctxt, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        if (if_expr)
            co_yield if_expr.get();
        else if (block)
            co_yield block.get();
    }
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

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "IfExpr", ctxt);
        expr->dump(ctxt, indent + 2, stream);
        block->dump(ctxt, indent + 2, stream);
        if (else_expr) {
            else_expr->dump(ctxt, indent + 2, stream);
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield expr.get();
        co_yield block.get();
        if (else_expr)
            co_yield else_expr.get();
    }
};

struct LoopExpr final : Expr {
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;

    static constexpr ASTKind Kind = ASTKind::LoopExpr;

    LoopExpr(Span span, std::unique_ptr<Expr> expr,
             std::unique_ptr<Block> block)
        : Expr(Kind, span), expr(std::move(expr)), block(std::move(block)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "LoopExpr", ctxt);
        if (expr) {
            expr->dump(ctxt, indent + 2, stream);
        }
        block->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        if (expr)
            co_yield expr.get();
        co_yield block.get();
    }
};

struct WhileExpr final : Expr {
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;

    static constexpr ASTKind Kind = ASTKind::WhileExpr;

    WhileExpr(Span span, std::unique_ptr<Expr> expr,
              std::unique_ptr<Block> block)
        : Expr(Kind, span), expr(std::move(expr)), block(std::move(block)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "WhileExpr", ctxt);
        expr->dump(ctxt, indent + 2, stream);
        block->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield expr.get();
        co_yield block.get();
    }
};

struct StringExpr final : Expr {
    StringID string;

    static constexpr ASTKind Kind = ASTKind::StringExpr;

    explicit StringExpr(Span span, StringID string)
        : Expr(Kind, span), string(string) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_indent(stream, indent);
        stream << colour::BOLD_GREEN << "StringExpr" << colour::RESET << " "
               << colour::YELLOW << "'" << ctxt->strings->get_string(string)
               << "'" << colour::RESET;
        dump_type(ctxt, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct CharExpr final : Expr {
    unsigned char c;

    static constexpr ASTKind Kind = ASTKind::CharExpr;

    explicit CharExpr(Span span, unsigned char c) : Expr(Kind, span), c(c) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_indent(stream, indent);
        stream << colour::BOLD_GREEN << "CharExpr" << colour::RESET << " "
               << colour::YELLOW << "'" << static_cast<unsigned int>(c) << "'"
               << colour::RESET;
        dump_type(ctxt, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

struct StructField final : ASTNode {
    std::unique_ptr<Identifier> ident;
    type::TypeRef type;

    static constexpr ASTKind Kind = ASTKind::StructField;

    StructField(Span span, std::unique_ptr<Identifier> ident,
                type::TypeRef type)
        : ASTNode(Kind, span), ident(std::move(ident)), type(type) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "StructField", ctxt);
        ident->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override { co_yield ident.get(); }
};

struct StructDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    std::vector<std::unique_ptr<StructField>> fields;
    std::vector<std::unique_ptr<Decl>> funcs;
    ScopeID scope;

    static constexpr ASTKind Kind = ASTKind::StructDecl;

    StructDecl(Span span, std::unique_ptr<Identifier> ident,
               std::vector<std::unique_ptr<StructField>> fields,
               std::vector<std::unique_ptr<Decl>> funcs, ScopeID scope)
        : Decl(Kind, span), ident(std::move(ident)), fields(std::move(fields)),
          funcs(std::move(funcs)), scope(scope) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "StructDecl", ctxt);
        ident->dump(ctxt, indent + 2, stream);
        for (const auto& field : fields)
            field->dump(ctxt, indent + 2, stream);
        for (const auto& func : funcs)
            func->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield ident.get();
        for (const auto& field : fields)
            co_yield field.get();

        for (const auto& func : funcs)
            co_yield func.get();
    }

    void declare_type(ZContext* ctxt) override {
        valid = ctxt->syms->declare_type(
            ident, ctxt->ty->make<type::TempType>(ident->get_id(),
                                                  type::TypeKind::Struct));
    }

    void resolve_sym(ZContext* ctxt) override {
        if (!valid)
            return;

        auto t = ctxt->syms->get_type(ident->get_id());
        if (!t)
            return;

        std::unordered_map<StringID, std::pair<type::TypeRef, std::uint32_t>>
            field_types;
        std::unordered_map<StringID, std::pair<type::TypeRef, std::uint32_t>>
            func_types;

        std::uint32_t field_num = 0;

        ctxt->syms->enter_scope(scope);

        for (const auto& field : fields) {
            if (!ctxt->resolve_unk_type(field->type))
                return;

            const auto is_unique =
                field_types
                    .insert(std::make_pair(
                        field->ident->get_id(),
                        std::make_pair(field->type, field_num++)))
                    .second;
            if (!is_unique) {
                auto err = ctxt->diag.error(
                    field->ident->get_span(), DiagnosticKind::DuplicateField,
                    ctxt->strings->get_string(ident->get_id()),
                    ctxt->strings->get_string(field->ident->get_id()));
                err.add_primary_note("defined again here");
                for (const auto& f : fields) {
                    if (f->ident->get_id() == field->ident->get_id() &&
                        f.get() != field.get()) {
                        err.add_note(f->ident->get_span(),
                                     "first defined here");
                        break;
                    }
                }
            } else {
                ctxt->syms->declare_var(field->ident, field->type, false, true);
            }
        }

        for (const auto& func : funcs) {
            auto* func_decl = cast<FuncDecl>(func.get());
            auto param_types = std::vector<type::TypeRef>();
            for (const auto& param : func_decl->params) {
                if (!ctxt->resolve_unk_type(param->type))
                    return;
                param_types.push_back(param->type);
            }

            if (!ctxt->resolve_unk_type(func_decl->ret))
                return;

            const auto func_type =
                ctxt->ty->make<type::FunctionType>(param_types, func_decl->ret);
            const auto is_unique =
                func_types
                    .insert(
                        std::make_pair(func_decl->name->get_id(),
                                       std::make_pair(func_type, field_num++)))
                    .second;
            if (!is_unique) {
                auto err = ctxt->diag.error(
                    func_decl->name->get_span(), DiagnosticKind::DuplicateField,
                    ctxt->strings->get_string(ident->get_id()),
                    ctxt->strings->get_string(func_decl->name->get_id()));
                err.add_primary_note("defined again here");
                for (const auto& f : funcs) {
                    if (f.get() == func.get()) {
                        break;
                    }
                    auto* fd = cast<FuncDecl>(f.get());
                    if (fd->name->get_id() == func_decl->name->get_id()) {
                        err.add_note(fd->name->get_span(),
                                     "first defined here");
                        break;
                    }
                }
            } else {
                ctxt->syms->declare_func(func_decl->name, func_type);
            }
        }

        ctxt->syms->exit_scope();
        ctxt->ty->replace<type::StructType>(*t, ident->get_id(), field_types,
                                            func_types);
    }
};

struct TraitDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    std::vector<std::unique_ptr<Decl>> consts;
    std::vector<std::unique_ptr<Decl>> types;
    std::vector<std::unique_ptr<Decl>> funcs;

    ScopeID scope;

    static constexpr ASTKind Kind = ASTKind::TraitDecl;

    TraitDecl(Span span, std::unique_ptr<Identifier> ident,
              std::vector<std::unique_ptr<Decl>> consts,
              std::vector<std::unique_ptr<Decl>> types,
              std::vector<std::unique_ptr<Decl>> funcs, ScopeID scope)
        : Decl(Kind, span), ident(std::move(ident)), consts(std::move(consts)),
          types(std::move(types)), funcs(std::move(funcs)), scope(scope) {}

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "TraitDecl", ctxt);
        ident->dump(ctxt, indent + 2, stream);
        for (const auto& c : consts)
            c->dump(ctxt, indent + 2, stream);
        for (const auto& type : types)
            type->dump(ctxt, indent + 2, stream);
        for (const auto& func : funcs)
            func->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield ident.get();
        for (const auto& c : consts)
            co_yield c.get();

        for (const auto& type : types)
            co_yield type.get();

        for (const auto& func : funcs)
            co_yield func.get();
    }

    void declare_type(ZContext* ctxt) override {
        ctxt->syms->enter_scope(scope);

        for (const auto& c : consts)
            c->declare_type(ctxt);

        for (const auto& type : types)
            type->declare_type(ctxt);

        for (const auto& func : funcs)
            func->declare_type(ctxt);

        ctxt->syms->exit_scope();
    }

    void resolve_sym(ZContext* ctxt) override {
        ctxt->syms->enter_scope(scope);

        for (const auto& c : consts)
            c->resolve_sym(ctxt);

        for (const auto& type : types)
            type->resolve_sym(ctxt);

        for (const auto& func : funcs)
            func->resolve_sym(ctxt);

        ctxt->syms->exit_scope();
    }
};

struct TypeAliasDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    type::TypeRef type;

    static constexpr ASTKind Kind = ASTKind::TypeAliasDecl;

    TypeAliasDecl(Span span, std::unique_ptr<Identifier> ident,
                  type::TypeRef type)
        : Decl(Kind, span), ident(std::move(ident)), type(type) {}

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "TypeAliasDecl", ctxt);
        ident->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override { co_yield ident.get(); }

    void declare_type(ZContext* ctxt) override {
        valid = ctxt->syms->declare_type(ident, type);
    }

    void resolve_sym(ZContext* ctxt) override {
        if (!valid)
            return;

        ctxt->resolve_unk_type(type);
    }
};

struct TraitFuncDecl final : Decl {
    std::unique_ptr<Identifier> name;
    std::vector<std::unique_ptr<Param>> params;
    type::TypeRef ret;
    std::unique_ptr<Block> body;

    static constexpr ASTKind Kind = ASTKind::TraitFuncDecl;

    TraitFuncDecl(Span span, std::unique_ptr<Identifier> name,
                  std::vector<std::unique_ptr<Param>> params, type::TypeRef ret,
                  std::unique_ptr<Block> body)
        : Decl(Kind, span), name(std::move(name)), params(std::move(params)),
          ret(ret), body(std::move(body)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "TraitFuncDecl", ctxt);
        name->dump(ctxt, indent + 2, stream);
        if (body)
            body->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield name.get();

        for (const auto& param : params)
            co_yield param.get();

        if (body)
            co_yield body.get();
    }

    void declare_type(ZContext* ctxt) override {
        valid = ctxt->syms->declare_func(
            name, ctxt->ty->make<type::TempType>(name->get_id(),
                                                 type::TypeKind::Function));
    }

    void resolve_sym(ZContext* ctxt) override {
        if (!valid)
            return;

        auto t = ctxt->syms->get_func(name->get_id());
        if (!t)
            return;

        auto param_types = std::vector<type::TypeRef>();
        for (const auto& param : params) {
            if (!ctxt->resolve_unk_type(param->type)) {
                return;
            }
            param_types.push_back(param->type);
        }

        if (!ctxt->resolve_unk_type(ret)) {
            return;
        }

        ctxt->ty->replace<type::FunctionType>(*t, param_types, ret);
    }
};

struct EnumField final : ASTNode {
    std::unique_ptr<Identifier> ident;
    std::vector<type::TypeRef> types;

    static constexpr ASTKind Kind = ASTKind::EnumField;

    EnumField(Span span, std::unique_ptr<Identifier> ident)
        : ASTNode(Kind, span), ident(std::move(ident)) {};
    EnumField(Span span, std::unique_ptr<Identifier> ident,
              std::vector<type::TypeRef> types)
        : ASTNode(Kind, span), ident(std::move(ident)),
          types(std::move(types)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "EnumField", ctxt);
        ident->dump(ctxt, indent + 2, stream);
        for (const auto& t : types) {
            print_indent(stream, indent + 2);
            stream << colour::CYAN << "<" << ctxt->ty->get(t)->basic_name(ctxt)
                   << ">" << colour::RESET << '\n';
        }
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override { co_yield ident.get(); }
};

struct EnumDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    std::vector<std::unique_ptr<EnumField>> fields;

    static constexpr ASTKind Kind = ASTKind::EnumDecl;

    EnumDecl(Span span, std::unique_ptr<Identifier> ident,
             std::vector<std::unique_ptr<EnumField>> fields)
        : Decl(Kind, span), ident(std::move(ident)),
          fields(std::move(fields)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "EnumDecl", ctxt);
        ident->dump(ctxt, indent + 2, stream);
        for (const auto& field : fields)
            field->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield ident.get();
        for (const auto& field : fields)
            co_yield field.get();
    }

    void declare_type(ZContext* ctxt) override {
        valid = ctxt->syms->declare_type(
            ident, ctxt->ty->make<type::TempType>(ident->get_id(),
                                                  type::TypeKind::Enum));
    }

    void resolve_sym(ZContext* ctxt) override {
        if (!valid)
            return;

        auto t = ctxt->syms->get_type(ident->get_id());
        if (!t)
            return;

        std::unordered_map<StringID, std::vector<type::TypeRef>&> field_types;

        for (const auto& field : fields) {
            for (auto& type : field->types) {
                if (!ctxt->resolve_unk_type(type)) {
                    return;
                }
            }

            const auto is_unique =
                field_types.insert({field->ident->get_id(), field->types})
                    .second;
            if (!is_unique) {
                auto err = ctxt->diag.error(
                    field->ident->get_span(), DiagnosticKind::DuplicateField,
                    ctxt->strings->get_string(ident->get_id()),
                    ctxt->strings->get_string(field->ident->get_id()));
                err.add_primary_note("defined again here");
                for (const auto& f : fields) {
                    if (f->ident->get_id() == field->ident->get_id() &&
                        f.get() != field.get()) {
                        err.add_note(f->ident->get_span(),
                                     "first defined here");
                        break;
                    }
                }
            }
        }

        ctxt->ty->replace<type::EnumType>(*t, ident->get_id(), field_types);
    }
};

struct ConstDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    type::TypeRef type;
    std::unique_ptr<Expr> val;

    static constexpr ASTKind Kind = ASTKind::ConstDecl;

    ConstDecl(Span span, std::unique_ptr<Identifier> ident, type::TypeRef type,
              std::unique_ptr<Expr> val)
        : Decl(Kind, span), ident(std::move(ident)), type(type),
          val(std::move(val)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "ConstDecl", ctxt);
        ident->dump(ctxt, indent + 2, stream);
        if (val)
            val->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield ident.get();
        co_yield val.get();
    }

    void declare_type(ZContext* ctxt) override {
        valid = ctxt->syms->declare_var(ident, type, true, true);
    }

    void resolve_sym(ZContext* ctxt) override {
        if (!valid)
            return;

        if (!ctxt->resolve_unk_type(type)) {
            return;
        }
    }
};

struct StaticDecl final : Decl {
    std::unique_ptr<Identifier> ident;
    type::TypeRef type;
    std::unique_ptr<Expr> val;

    static constexpr ASTKind Kind = ASTKind::StaticDecl;

    StaticDecl(Span span, std::unique_ptr<Identifier> ident, type::TypeRef type,
               std::unique_ptr<Expr> val)
        : Decl(Kind, span), ident(std::move(ident)), type(type),
          val(std::move(val)) {};

    void dump(ZContext* ctxt, const int indent,
              std::ostream& stream) const override {
        print_header(stream, indent, "StaticDecl", ctxt);
        ident->dump(ctxt, indent + 2, stream);
        if (val)
            val->dump(ctxt, indent + 2, stream);
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }

    std::generator<ASTNode*> children() override {
        co_yield ident.get();
        co_yield val.get();
    }

    void declare_type(ZContext* ctxt) override {
        valid = ctxt->syms->declare_var(ident, type, false, true);
    }

    void resolve_sym(ZContext* ctxt) override {
        if (!valid)
            return;

        if (!ctxt->resolve_unk_type(type)) {
            return;
        }
    }
};
} // namespace z::ast
