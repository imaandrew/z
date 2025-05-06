#pragma once

#include "token.h"
#include "type.h"
#include <iostream>
#include <memory>
#include <optional>
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

class ASTNode {
    bool valid = true;
public:
    ASTNode() = default;
    virtual ~ASTNode() = default;

    ASTNode(const ASTNode&) = delete;
    ASTNode& operator=(const ASTNode&) = delete;
    ASTNode(ASTNode&&) = delete;
    ASTNode& operator=(ASTNode&&) = delete;
    [[nodiscard]] bool is_valid() const { return valid; }

    void mark_invalid() { valid = false; }

    virtual void dump(int indent = 0,
                      std::ostream& stream = std::cout) const = 0;
};

class Stmt : public ASTNode {
public:
    Stmt() = default;
    ~Stmt() override = default;
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;
    Stmt(Stmt&&) = delete;
    Stmt& operator=(Stmt&&) = delete;
};

class InvalidStmt : public Stmt {
public:
    InvalidStmt() { mark_invalid(); };

    void dump(int /*indent*/, std::ostream& stream) const override {
        stream << "InvalidStmt" << '\n';
    }
};

class Expr : public Stmt {
public:
    Expr() = default;
    ~Expr() override = default;
    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;
    Expr(Expr&&) = delete;
    Expr& operator=(Expr&&) = delete;
};

class IntExpr : public Expr {
    Token tok;
    long long val;

public:
    IntExpr(Token tok, long long val) : tok(tok), val(val) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "IntExpr " << val << '\n';
    }
};

class FloatExpr : public Expr {
    Token tok;
    double val;

public:
    FloatExpr(Token tok, double val) : tok(tok), val(val) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "FloatExpr " << val << '\n';
    }
};

// NOLINTBEGIN(readability-identifier-length)

class PrefixExpr : public Expr {
    Token op;
    std::unique_ptr<Expr> expr;

public:
    PrefixExpr(Token op, std::unique_ptr<Expr> expr)
        : op(op), expr(std::move(expr)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "PrefixExpr "
               << std::string(op.get_val(), op.get_len()) << '\n';
        expr->dump(indent + 2, stream);
    }
};

class PostfixExpr : public Expr {
    Token op;
    std::unique_ptr<Expr> expr;

public:
    PostfixExpr(Token op, std::unique_ptr<Expr> expr)
        : op(op), expr(std::move(expr)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "PostfixExpr "
               << std::string(op.get_val(), op.get_len()) << '\n';
        expr->dump(indent + 2, stream);
    }
};

class BinaryExpr : public Expr {
    Token op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

public:
    BinaryExpr(Token op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "BinaryExpr "
               << std::string(op.get_val(), op.get_len()) << '\n';
        lhs->dump(indent + 2, stream);
        rhs->dump(indent + 2, stream);
    }
};

class TernaryExpr : public Expr {
    Token op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> mhs;
    std::unique_ptr<Expr> rhs;

public:
    TernaryExpr(Token op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> mhs,
                std::unique_ptr<Expr> rhs)
        : op(op), lhs(std::move(lhs)), mhs(std::move(mhs)),
          rhs(std::move(rhs)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "TernaryExpr "
               << std::string(op.get_val(), op.get_len()) << '\n';
        lhs->dump(indent + 2, stream);
        mhs->dump(indent + 2, stream);
        rhs->dump(indent + 2, stream);
    }
};

// NOLINTEND(readability-identifier-length)

class CallExpr : public Expr {
    std::unique_ptr<Expr> func;
    std::vector<std::unique_ptr<Expr>> args;

public:
    CallExpr(std::unique_ptr<Expr> func, std::vector<std::unique_ptr<Expr>> args)
        : func(std::move(func)), args(std::move(args)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "CallExpr" << '\n';
        func->dump(indent + 2, stream);
        for (const auto& arg : args) {
            arg->dump(indent + 2, stream);
        }
    }
};

class ArrayExpr : public Expr {
    std::unique_ptr<Expr> ident;
    std::unique_ptr<Expr> val;

public:
    explicit ArrayExpr(std::unique_ptr<Expr> ident)
        : ident(std::move(ident)) {};

    ArrayExpr(std::unique_ptr<Expr> ident, std::unique_ptr<Expr> val)
        : ident(std::move(ident)), val(std::move(val)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ArrayExpr" << '\n';
        ident->dump(indent + 2, stream);
        if (val)
            val->dump(indent + 2, stream);
    }
};

class ArrayInitExpr : public Expr {
    std::vector<std::unique_ptr<Expr>> vals;

public:
    explicit ArrayInitExpr(std::vector<std::unique_ptr<Expr>> vals)
        : vals(std::move(vals)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ArrayInitExpr" << '\n';
        for (const auto& val : vals) {
            val->dump(indent + 2, stream);
        }
    }
};

class StructInitExpr : public Expr {
    std::unique_ptr<Expr> ident;
    std::vector<std::unique_ptr<Expr>> vals;

public:
    StructInitExpr(std::unique_ptr<Expr> ident, std::vector<std::unique_ptr<Expr>> vals) : ident(std::move(ident)), vals(std::move(vals)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "StructInitExpr" << '\n';
        ident->dump(indent + 2, stream);
        for (const auto& val : vals) {
            val->dump(indent + 2, stream);
        }
    }
};

class Identifier : public Expr {
    Token tok;

public:
    explicit Identifier(Token tok) : tok(tok) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "Identifier "
               << std::string(tok.get_val(), tok.get_len()) << '\n';
    }

    [[nodiscard]] std::string to_string() const {
        return std::string(tok.get_val(), tok.get_len());
    }
};

class Block : public Stmt {
    std::vector<std::unique_ptr<Stmt>> stmts;

public:
    explicit Block(std::vector<std::unique_ptr<Stmt>> stmts)
        : stmts(std::move(stmts)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "Block" << '\n';
        for (const auto& stmt : stmts) {
            stmt->dump(indent + 2, stream);
        }
    }
};

class Param : public Expr {
    std::unique_ptr<Identifier> name;
    std::unique_ptr<Type> type;

public:
    Param(std::unique_ptr<Identifier> name, std::unique_ptr<Type> type)
        : name(std::move(name)), type(std::move(type)) {};

    void dump(int indent, std::ostream& stream) const override {
        // TODO
    }
};

class Decl : public ASTNode {
public:
    Decl() = default;
    ~Decl() override = default;
    Decl(const Decl&) = delete;
    Decl& operator=(const Decl&) = delete;
    Decl(Decl&&) = delete;
    Decl& operator=(Decl&&) = delete;
};

class FuncDecl : public Decl {
    std::unique_ptr<Identifier> name;
    std::optional<std::unique_ptr<Identifier>> impl_type;
    std::vector<std::unique_ptr<Expr>> params;
    std::optional<std::unique_ptr<Type>> ret;
    std::unique_ptr<Block> body;

public:
    FuncDecl(std::unique_ptr<Identifier> name,
             std::optional<std::unique_ptr<Identifier>> impl_type,
             std::vector<std::unique_ptr<Expr>> params,
             std::optional<std::unique_ptr<Type>> ret,
             std::unique_ptr<Block> body)
        : name(std::move(name)), impl_type(std::move(impl_type)),
          params(std::move(params)), ret(std::move(ret)),
          body(std::move(body)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "FuncDecl" << '\n';
        name->dump(indent + 2, stream);
        if (impl_type && impl_type.has_value())
            impl_type.value()->dump(indent + 2, stream);
        body->dump(indent + 2, stream);
    }
};

class BreakStmt : public Stmt {
    Token tok;
    std::unique_ptr<Expr> expr;

public:
    explicit BreakStmt(Token tok) : tok(tok) {};
    BreakStmt(Token tok, std::unique_ptr<Expr> expr) : tok(tok), expr(std::move(expr)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "BreakStmt" << '\n';
        if (expr)
            expr->dump(indent + 2, stream);
    }
};

class ContinueStmt : public Stmt {
    Token tok;

public:
    explicit ContinueStmt(Token tok) : tok(tok) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ContinueStmt" << '\n';
    }
};

class ForExpr : public Expr {
    std::unique_ptr<Identifier> ident;
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;

public:
    ForExpr(std::unique_ptr<Identifier> ident, std::unique_ptr<Expr> expr,
            std::unique_ptr<Block> block)
        : ident(std::move(ident)), expr(std::move(expr)),
          block(std::move(block)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ForExpr" << '\n';
        expr->dump(indent + 2, stream);
        block->dump(indent + 2, stream);
    }
};

class LetStmt : public Stmt {
    std::unique_ptr<Identifier> ident;
    std::unique_ptr<Type> type;
    std::unique_ptr<Expr> val;

public:
    explicit LetStmt(std::unique_ptr<Identifier> ident)
        : ident(std::move(ident)) {};

    LetStmt(std::unique_ptr<Identifier> ident, std::unique_ptr<Expr> val)
        : ident(std::move(ident)), val(std::move(val)) {};

    LetStmt(std::unique_ptr<Identifier> ident, std::unique_ptr<Type> type,
            std::unique_ptr<Expr> val)
        : ident(std::move(ident)), type(std::move(type)),
          val(std::move(val)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "LetStmt" << '\n';
        ident->dump(indent + 2, stream);
        //if (type)
            //type.value()->print(indent + 2);
        if (val)
            val->dump(indent + 2, stream);
    }
};

class ReturnStmt : public Stmt {
    std::unique_ptr<Expr> expr;

public:
    ReturnStmt() = default;
    explicit ReturnStmt(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ReturnStmt" << '\n';
        if (expr)
            expr->dump(indent + 2, stream);
    }
};

class IfExpr;

class ElseExpr : public Expr {
    std::unique_ptr<Expr> if_expr;
    std::unique_ptr<Block> block;

public:
    explicit ElseExpr(std::unique_ptr<Block> block)
        : block(std::move(block)) {};
    explicit ElseExpr(std::unique_ptr<Expr> if_expr)
        : if_expr(std::move(if_expr)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ElseExpr" << '\n';

        if (if_expr) {
            if_expr->dump(indent + 2, stream);
        } else if (block) {
            block->dump(indent + 2, stream);
        }
    }
};

class IfExpr : public Expr {
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;
    std::unique_ptr<ElseExpr> else_expr;

public:
    IfExpr(std::unique_ptr<Expr> expr, std::unique_ptr<Block> block)
        : expr(std::move(expr)), block(std::move(block)) {};
    IfExpr(std::unique_ptr<Expr> expr, std::unique_ptr<Block> block,
           std::unique_ptr<ElseExpr> else_expr)
        : expr(std::move(expr)), block(std::move(block)),
          else_expr(std::move(else_expr)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "IfExpr" << '\n';
        expr->dump(indent + 2, stream);
        block->dump(indent + 2, stream);
        if (else_expr) {
            else_expr->dump(indent + 2, stream);
        }
    }
};

class LoopExpr : public Expr {
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;

public:
    LoopExpr(std::unique_ptr<Expr> expr, std::unique_ptr<Block> block)
        : expr(std::move(expr)), block(std::move(block)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "LoopExpr" << '\n';
        if (expr) {
            expr->dump(indent + 2, stream);
        }
        block->dump(indent + 2, stream);
    }
};

class WhileExpr : public Expr {
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Block> block;

public:
    WhileExpr(std::unique_ptr<Expr> expr, std::unique_ptr<Block> block)
        : expr(std::move(expr)), block(std::move(block)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "WhileExpr" << '\n';
        expr->dump(indent + 2, stream);
        block->dump(indent + 2, stream);
    }
};

class StringExpr : public Expr {
    const char* start;
    size_t len;

public:
    StringExpr(const char* start, size_t len) : start(start), len(len) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "String "
               << std::string(start, len) << '\n';
    }
};

class StructField : public Decl {
    std::unique_ptr<Identifier> ident;
    std::unique_ptr<Type> type;

public:
    StructField(std::unique_ptr<Identifier> ident, std::unique_ptr<Type> type)
        : ident(std::move(ident)), type(std::move(type)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "StructField" << '\n';
        ident->dump(indent + 2, stream);
        //type->print(indent + 2);
    }
};

class StructDecl : public Decl {
    std::unique_ptr<Identifier> ident;
    std::vector<std::unique_ptr<StructField>> fields;

public:
    StructDecl(std::unique_ptr<Identifier> ident,
               std::vector<std::unique_ptr<StructField>> fields)
        : ident(std::move(ident)), fields(std::move(fields)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "StructDecl" << '\n';
        ident->dump(indent + 2, stream);
        for (const auto& field : fields)
            field->dump(indent + 2, stream);
    }
};

class EnumField : public Decl {
    std::unique_ptr<Identifier> ident;
    std::vector<std::unique_ptr<Type>> types;

public:
    explicit EnumField(std::unique_ptr<Identifier> ident)
        : ident(std::move(ident)) {};
    EnumField(std::unique_ptr<Identifier> ident,
              std::vector<std::unique_ptr<Type>> types)
        : ident(std::move(ident)), types(std::move(types)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "EnumField" << '\n';
        ident->dump(indent + 2, stream);
        //for (const auto& t : types)
            //t->print(indent + 2);
    }
};

class EnumDecl : public Decl {
    std::unique_ptr<Identifier> ident;
    std::vector<std::unique_ptr<EnumField>> fields;

public:
    EnumDecl(std::unique_ptr<Identifier> ident,
             std::vector<std::unique_ptr<EnumField>> fields)
        : ident(std::move(ident)), fields(std::move(fields)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "EnumDecl" << '\n';
        ident->dump(indent + 2, stream);
        for (const auto& field : fields)
            field->dump(indent + 2, stream);
    }
};

class ConstDecl : public Decl {
    std::unique_ptr<Identifier> ident;
    std::unique_ptr<Type> type;
    std::unique_ptr<Expr> val;

public:
    ConstDecl(std::unique_ptr<Identifier> ident, std::unique_ptr<Type> type,
              std::unique_ptr<Expr> val)
        : ident(std::move(ident)), type(std::move(type)),
          val(std::move(val)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "ConstDecl" << '\n';
        ident->dump(indent + 2, stream);
        // print type
        if (val)
            val->dump(indent + 2, stream);
    }
};

class StaticDecl : public Decl {
    std::unique_ptr<Identifier> ident;
    std::unique_ptr<Type> type;
    std::unique_ptr<Expr> val;

public:
    StaticDecl(std::unique_ptr<Identifier> ident, std::unique_ptr<Type> type,
               std::unique_ptr<Expr> val)
        : ident(std::move(ident)), type(std::move(type)),
          val(std::move(val)) {};

    void dump(int indent, std::ostream& stream) const override {
        stream << std::string(indent, ' ') << "StaticDecl" << '\n';
        ident->dump(indent + 2, stream);
        // print type
        if (val)
            val->dump(indent + 2, stream);
    }
};