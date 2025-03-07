#pragma once
#include "token.h"
#include "type.h"
#include <iostream>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

enum class BinOpPrecedence {
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

class Stmt {
public:
    virtual ~Stmt() = default;
    virtual void print(int indent) const = 0;
};

class Expr : public Stmt {
};

class IntExpr : public Expr {
    Token tok;
    long long val;

public:
    IntExpr(Token tok, long long val) : tok(tok), val(val) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "IntExpr " << val
                  << std::endl;
    }
};

class FloatExpr : public Expr {
    Token tok;
    double val;

public:
    FloatExpr(Token tok, double val) : tok(tok), val(val) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "FloatExpr " << val
                  << std::endl;
    }
};

class PrefixExpr : public Expr {
    Token op;
    std::unique_ptr<Expr> expr;

public:
    PrefixExpr(Token op, std::unique_ptr<Expr> expr)
        : op(op), expr(std::move(expr)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "PrefixExpr " << std::string(op.get_val(), op.get_len())
                  << std::endl;
        expr->print(indent + 2);
    }
};

class PostfixExpr : public Expr {
    Token op;
    std::unique_ptr<Expr> expr;

public:
    PostfixExpr(Token op, std::unique_ptr<Expr> expr)
        : op(op), expr(std::move(expr)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "PostfixExpr " << std::string(op.get_val(), op.get_len())
                  << std::endl;
        expr->print(indent + 2);
    }
};

class BinaryExpr : public Expr {
    Token op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

public:
    BinaryExpr(Token op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "BinaryExpr " << std::string(op.get_val(), op.get_len())
                  << std::endl;
        lhs->print(indent + 2);
        rhs->print(indent + 2);
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

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "TernaryExpr " << std::string(op.get_val(), op.get_len())
                  << std::endl;
        lhs->print(indent + 2);
        mhs->print(indent + 2);
        rhs->print(indent + 2);
    }
};

class CallExpr : public Expr {
    std::unique_ptr<Expr> func;
    std::vector<std::unique_ptr<Expr>> args;

public:
    CallExpr(std::unique_ptr<Expr> func, std::vector<std::unique_ptr<Expr>> args)
        : func(std::move(func)), args(std::move(args)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "CallExpr" << std::endl;
        func->print(indent + 2);
        for (auto& arg : args) {
            arg->print(indent + 2);
        }
    }
};

class ArrayExpr : public Expr {
    std::unique_ptr<Expr> ident;
    std::unique_ptr<Expr> val;

public:
    ArrayExpr(std::unique_ptr<Expr> ident) : ident(std::move(ident)) {};

    ArrayExpr(std::unique_ptr<Expr> ident, std::unique_ptr<Expr> val)
        : ident(std::move(ident)), val(std::move(val)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "ArrayExpr" << std::endl;
        ident->print(indent + 2);
        if (val)
            val->print(indent + 2);
    }
};

class ArrayInitExpr : public Expr {
    std::vector<std::unique_ptr<Expr>> vals;

public:
    ArrayInitExpr(std::vector<std::unique_ptr<Expr>> vals) : vals(std::move(vals)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "ArrayInitExpr" << std::endl;
        for (auto& val : vals) {
            val->print(indent + 2);
        }
    }
};

class StructInitExpr : public Expr {
    std::unique_ptr<Expr> ident;
    std::vector<std::unique_ptr<Expr>> vals;

public:
    StructInitExpr(std::unique_ptr<Expr> ident, std::vector<std::unique_ptr<Expr>> vals) : ident(std::move(ident)), vals(std::move(vals)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "StructInitExpr" << std::endl;
        ident->print(indent + 2);
        for (auto& val : vals) {
            val->print(indent + 2);
        }
    }
};

class Identifier : public Expr {
    Token tok;

public:
    Identifier(Token tok) : tok(tok) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "Identifier " << std::string(tok.get_val(), tok.get_len())
                  << std::endl;
    }

    std::string to_string() const {
        return std::string(tok.get_val(), tok.get_len());
    }
};

class Block : public Stmt {
    std::vector<std::unique_ptr<Stmt>> stmts;

public:
    Block(std::vector<std::unique_ptr<Stmt>> stmts)
        : stmts(std::move(stmts)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "Block" << std::endl;
        for (auto& stmt : stmts) {
            stmt->print(indent + 2);
        }
    }
};

class Param {
    Identifier name;
    Type type;

public:
    Param(Identifier name, Type type) : name(name), type(type) {};
};

class Decl {
public:
    virtual ~Decl() = default;
    virtual void print(int indent) const = 0;
};

class FuncDecl : public Decl {
    Identifier name;
    std::optional<Identifier> impl_type;
    std::vector<Param> params;
    std::optional<Type> ret;
    Block body;

public:
    FuncDecl(Identifier name, std::optional<Identifier> impl_type, std::vector<Param> params, std::optional<Type> ret, Block body)
        : name(name), impl_type(impl_type), params(std::move(params)), ret(ret), body(std::move(body)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "FuncDecl" << std::endl;
        name.print(indent + 2);
        if (impl_type)
            impl_type->print(indent + 2);
        body.print(indent + 2);
    }
};

class BreakStmt : public Stmt {
    Token tok;
    std::unique_ptr<Expr> expr;

public:
    BreakStmt(Token tok) : tok(tok) {};
    BreakStmt(Token tok, std::unique_ptr<Expr> expr) : tok(tok), expr(std::move(expr)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "BreakStmt" << std::endl;
        if (expr)
            expr->print(indent + 2);
    }
};

class ContinueStmt : public Stmt {
    Token tok;

public:
    ContinueStmt(Token tok) : tok(tok) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "ContinueStmt" << std::endl;
    }
};

class ForExpr : public Expr {
    Identifier ident;
    std::unique_ptr<Expr> expr;
    Block block;

public:
    ForExpr(Identifier ident, std::unique_ptr<Expr> expr, Block block)
        : ident(ident), expr(std::move(expr)), block(std::move(block)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "ForExpr" << std::endl;
        expr->print(indent + 2);
        block.print(indent + 2);
    }
};

class LetStmt : public Stmt {
    Identifier ident;
    std::optional<Type> type;
    std::unique_ptr<Expr> val;

public:
    LetStmt(Identifier ident) : ident(ident) {};

    LetStmt(Identifier ident, std::unique_ptr<Expr> val)
        : ident(ident), val(std::move(val)) {};

    LetStmt(Identifier ident, Type type, std::unique_ptr<Expr> val)
        : ident(ident), type(type), val(std::move(val)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "LetStmt" << std::endl;
        ident.print(indent + 2);
        //if (type)
            //type.value()->print(indent + 2);
        if (val)
            val->print(indent + 2);
    }
};

class ReturnStmt : public Stmt {
    std::unique_ptr<Expr> expr;

public:
    ReturnStmt() {}
    ReturnStmt(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "ReturnStmt" << std::endl;
        if (expr)
            expr->print(indent + 2);
    }
};

class IfExpr;

class ElseExpr : public Expr {
    std::variant<std::unique_ptr<IfExpr>, Block> expr;

public:
    ElseExpr(std::unique_ptr<IfExpr> expr) : expr(std::move(expr)) {};
    ElseExpr(Block block) : expr(std::move(block)) {};

    void print(int indent) const override;
};

class IfExpr : public Expr {
    std::unique_ptr<Expr> expr;
    Block block;
    std::unique_ptr<ElseExpr> else_expr;

public:
    IfExpr(std::unique_ptr<Expr> expr, Block block)
        : expr(std::move(expr)), block(std::move(block)) {};
    IfExpr(std::unique_ptr<Expr> expr, Block block,
           std::unique_ptr<ElseExpr> else_expr)
        : expr(std::move(expr)), block(std::move(block)),
          else_expr(std::move(else_expr)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "IfExpr" << std::endl;
        expr->print(indent + 2);
        block.print(indent + 2);
        if (else_expr) {
            else_expr->print(indent + 2);
        }
    }
};

inline void ElseExpr::print(int indent) const {
    std::cout << std::string(indent, ' ') << "ElseExpr" << std::endl;
    
    if (auto e = std::get_if<std::unique_ptr<IfExpr>>(&expr)) {
        e->get()->print(indent + 2);
    } else if (auto b = std::get_if<Block>(&expr)) {
        b->print(indent + 2);
    }
}

class LoopExpr : public Expr {
    std::unique_ptr<Expr> expr;
    Block block;

public:
    LoopExpr(std::unique_ptr<Expr> expr, Block block)
        : expr(std::move(expr)), block(std::move(block)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "LoopExpr" << std::endl;
        if (expr) {
            expr->print(indent + 2);
        }
        block.print(indent + 2);
    }
};

class WhileExpr : public Expr {
    std::unique_ptr<Expr> expr;
    Block block;

public:
    WhileExpr(std::unique_ptr<Expr> expr, Block block)
        : expr(std::move(expr)), block(std::move(block)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "WhileExpr" << std::endl;
        expr->print(indent + 2);
        block.print(indent + 2);
    }
};

class StringExpr : public Expr {
    const char* start;
    size_t len;

public:
    StringExpr(const char* start, size_t len) : start(start), len(len) {};

    void print(int indent) const {
        std::cout << std::string(indent, ' ') << "String " << std::string(start, len) << std::endl;
    }
};

class StructField {
    Identifier ident;
    Type type;

public:
    StructField(Identifier ident, Type type) : ident(ident), type(type) {};

    void print(int indent) const {
        std::cout << std::string(indent, ' ') << "StructField" << std::endl;
        ident.print(indent + 2);
        //type->print(indent + 2);
    }
};

class StructDecl : public Decl {
    Identifier ident;
    std::vector<StructField> fields;

public:
    StructDecl(Identifier ident, std::vector<StructField> fields) : ident(ident), fields(fields) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "StructDecl" << std::endl;
        ident.print(indent + 2);
        for (const auto& f : fields)
            f.print(indent + 2);
    }
};

class EnumField {
    Identifier ident;
    std::vector<Type> types;

public:
    EnumField(Identifier ident) : ident(ident) {};
    EnumField(Identifier ident, std::vector<Type> types) : ident(ident), types(types) {};

    void print(int indent) const {
        std::cout << std::string(indent, ' ') << "EnumField" << std::endl;
        ident.print(indent + 2);
        //for (const auto& t : types)
            //t->print(indent + 2);
    }
};

class EnumDecl : public Decl {
    Identifier ident;
    std::vector<EnumField> fields;

public:
EnumDecl(Identifier ident, std::vector<EnumField> fields) : ident(ident), fields(fields) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "EnumDecl" << std::endl;
        ident.print(indent + 2);
        for (const auto& f : fields)
            f.print(indent + 2);
    }
};

class ConstDecl : public Decl {
    Identifier ident;
    Type type;
    std::unique_ptr<Expr> val;

public:
    ConstDecl(Identifier ident, Type type, std::unique_ptr<Expr> val) : ident(ident), type(type), val(std::move(val)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "ConstDecl" << std::endl;
        ident.print(indent + 2);
        // print type
        if (val)
            val->print(indent + 2);
    }
};

class StaticDecl : public Decl {
    Identifier ident;
    Type type;
    std::unique_ptr<Expr> val;

public:
    StaticDecl(Identifier ident, Type type, std::unique_ptr<Expr> val) : ident(ident), type(type), val(std::move(val)) {};

    void print(int indent) const override {
        std::cout << std::string(indent, ' ') << "StaticDecl" << std::endl;
        ident.print(indent + 2);
        // print type
        if (val)
            val->print(indent + 2);
    }
};