#pragma once
#include "token.h"
#include <memory>
#include <variant>
#include <optional>
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
    Postfix
};

class Stmt {

};

class Expr : public Stmt {

};

class IntExpr : public Expr {
    Token tok;
    long long val;

public:
    IntExpr(Token tok, long long val) : tok(tok), val(val) {};
};

class FloatExpr : public Expr {
    Token tok;
    double val;

public:
    FloatExpr(Token tok, double val) : tok(tok), val(val) {};
};

class PrefixExpr : public Expr {
    Token op;
    Expr expr;

public:
    PrefixExpr(Token op, Expr expr) : op(op), expr(expr) {};
};

class PostfixExpr : public Expr {
    Token op;
    Expr expr;

public:
    PostfixExpr(Token op, Expr expr) : op(op), expr(expr) {};
};

class BinaryExpr : public Expr {
    Token op;
    Expr lhs;
    Expr rhs;

public:
    BinaryExpr(Token op, Expr lhs, Expr rhs) : op(op), lhs(lhs), rhs(rhs) {};
};

class TernaryExpr : public Expr {
    Token op;
    Expr lhs;
    Expr mhs;
    Expr rhs;

public:
    TernaryExpr(Token op, Expr lhs, Expr mhs, Expr rhs) : op(op), lhs(lhs), mhs(mhs), rhs(rhs) {};
};

class CallExpr : public Expr {
    Expr func;
    std::vector<Expr> args;

public:
    CallExpr(Expr func, std::vector<Expr> args) : func(func), args(args) {};
};

class ArrayExpr : public Expr {
    Expr ident;
    Expr val;

public:
    ArrayExpr(Expr ident, Expr val) : ident(ident), val(val) {};
};


class Identifier : public Expr {
    Token tok;

public:
    Identifier(Token tok) : tok(tok) {};
};


class Block {
    std::vector<Stmt> stmts;
public:
    Block(std::vector<Stmt> stmts) : stmts(stmts) {};
};


class Param {
    Identifier name;
    Identifier type;

public:
    Param(Identifier name, Identifier type) : name(name), type(type) {};
};


class FuncDecl {
    Identifier name;
    std::vector<Param> params;
    Block body;

public:
    FuncDecl(Identifier name, std::vector<Param> params, Block body) : name(name), params(params), body(body) {};
};


class BreakStmt : public Stmt {
    Token tok;
public:
    BreakStmt(Token tok) : tok(tok) {};
};


class ContinueStmt : public Stmt {
    Token tok;
public:
    ContinueStmt(Token tok) : tok(tok) {};
};

class ForExpr : public Expr {
    Identifier ident;
    Expr expr;
    Block block;

public:
    ForExpr(Identifier ident, Expr expr, Block block) : ident(ident), expr(expr), block(block) {};
};

class LetStmt : public Stmt {
    Identifier ident;
    Expr val;

public:
    LetStmt(Identifier ident, Expr val) : ident(ident), val(val) {};
};

class ReturnStmt : public Stmt {
    std::optional<Expr> expr;

public:
    ReturnStmt() {}
    ReturnStmt(Expr expr) : expr(expr) {};
};

class IfExpr;

class ElseExpr : public Expr {
    std::variant<std::unique_ptr<IfExpr>, Block> expr;

public:
    ElseExpr(std::unique_ptr<IfExpr> expr) : expr(std::move(expr)) {};
    ElseExpr(Block block) : expr(block) {};
};

class IfExpr : public Expr {
    Expr expr;
    Block block;
    std::unique_ptr<ElseExpr> else_expr;

public:
    IfExpr(Expr expr, Block block) : expr(expr), block(block) {};
    IfExpr(Expr expr, Block block, std::unique_ptr<ElseExpr> else_expr) : expr(expr), block(block), else_expr(std::move(else_expr)) {};
};

class LoopExpr : public Expr {
    std::optional<Expr> expr;
    Block block;

public:
    LoopExpr(std::optional<Expr> expr, Block block) : expr(expr), block(block) {};
};

class WhileExpr : public Expr {
    Expr expr;
    Block block;

public:
    WhileExpr(Expr expr, Block block) : expr(expr), block(block) {};
};