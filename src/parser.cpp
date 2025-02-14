#include "parser.h"
#include "token.h"
#include "ast.h"
#include <charconv>
#include <exception>
#include <stdexcept>

BinOpPrecedence get_op_precedence(TokenKind kind) {
    switch (kind) {
        case TokenKind::PlusEq:
        case TokenKind::MinusEq:
        case TokenKind::StarEq:
        case TokenKind::SlashEq:
        case TokenKind::CaretEq:
        case TokenKind::AndEq:
        case TokenKind::OrEq:
        case TokenKind::ShlEq:
        case TokenKind::ShrEq:
        case TokenKind::Eq:
            return BinOpPrecedence::Assignment;
        case TokenKind::Range:
        case TokenKind::RangeEq:
            return BinOpPrecedence::Range;
        case TokenKind::Question:
            return BinOpPrecedence::Conditional;
        case TokenKind::OrOr:
            return BinOpPrecedence::LogicalOr;
        case TokenKind::AndAnd:
            return BinOpPrecedence::LogicalAnd;
        case TokenKind::EqEq:
        case TokenKind::Ne:
        case TokenKind::Gt:
        case TokenKind::Lt:
        case TokenKind::Ge:
        case TokenKind::Le:
            return BinOpPrecedence::Equality;
        case TokenKind::Or:
            return BinOpPrecedence::Or;
        case TokenKind::Caret:
            return BinOpPrecedence::Xor;
        case TokenKind::And:
            return BinOpPrecedence::And;
        case TokenKind::Shl:
        case TokenKind::Shr:
            return BinOpPrecedence::Shift;
        case TokenKind::Plus:
        case TokenKind::Minus:
            return BinOpPrecedence::Addition;
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:
            return BinOpPrecedence::Multiplication;
        default:
            return BinOpPrecedence::Unknown;
    }
}

void Parser::parse() {
    next_token();
    parse_func_decl();
}

FuncDecl Parser::parse_func_decl() {
    assert(TokenKind::KwFn);
    consume(TokenKind::Identifier);
    auto func_ident = Identifier(tok);
    auto params = parse_func_params();
    next_token();
    auto block = parse_block();
    return FuncDecl(func_ident, params, block);
}

std::vector<Param> Parser::parse_func_params() {
    consume(TokenKind::LParen);
    
    std::vector<Param> params;
    while (!kind(TokenKind::RParen)) {
        params.push_back(parse_param_decl());
        if (!kind(TokenKind::Comma)) {
            assert(TokenKind::RParen);
            break;
        }
    }

    return params;
}

Param Parser::parse_param_decl() {
    assert(TokenKind::Identifier);
    auto name = Identifier(tok);

    consume(TokenKind::Colon);

    consume(TokenKind::Identifier);
    auto type = Identifier(tok);

    return Param(name, type);
}

Block Parser::parse_block() {
    assert(TokenKind::LBrace);
    std::vector<Stmt> stmts;
    while (!kind(TokenKind::RBrace)) {
        stmts.push_back(parse_stmt());
    }

    return Block(stmts);
}

Stmt Parser::parse_stmt() {
    Stmt stmt;
    if (tok.is(TokenKind::KwBreak)) {
        stmt = BreakStmt(tok);
        consume(TokenKind::Semi);
    } else if (tok.is(TokenKind::KwContinue)) {
        stmt = ContinueStmt(tok);
        consume(TokenKind::Semi);
    } else if (tok.is(TokenKind::KwLet)) {
        consume(TokenKind::Identifier);
        auto ident = Identifier(tok);
        consume(TokenKind::Eq);
        stmt = LetStmt(ident, parse_expr());
        consume(TokenKind::Semi);
    } else if (tok.is(TokenKind::KwReturn)) {
        try {
            auto expr = parse_expr();
            stmt = ReturnStmt(expr);
        } catch (std::exception) {
            stmt = ReturnStmt();
        }
        consume(TokenKind::Semi);
    } else {
        stmt = parse_expr();
    }

    return stmt;
}

Expr Parser::parse_expr(int precedence) {
    Expr lhs;
    switch (tok.get_kind()) {
        case TokenKind::Number:
            lhs = parse_num();
            next_token();
            break;
        case TokenKind::Identifier:
            lhs = Identifier(tok);
            next_token();
            break;
        case TokenKind::PlusPlus:
        case TokenKind::MinusMinus:
        case TokenKind::LogicalNot:
        case TokenKind::Not:
        case TokenKind::Minus:
            lhs = parse_expr(static_cast<int>(BinOpPrecedence::Prefix));
            break;
        case TokenKind::LParen: {
            next_token();    
            auto expr = parse_expr();
            consume(TokenKind::RParen);
            next_token();
            lhs = expr;
            break;
        }
        case TokenKind::KwFor:
            return parse_for_expr();
        case TokenKind::KwIf:
            return parse_if_expr();
        case TokenKind::KwLoop:
            return parse_loop_expr();
        case TokenKind::KwWhile:
            return parse_while_expr();
        default:
            throw std::runtime_error(std::format("invalid expr {}", tok_kind_to_string(tok.get_kind())));
    }

    while (precedence < static_cast<int>(get_op_precedence(tok.get_kind()))) {
        switch (tok.get_kind()) {
            case TokenKind::PlusPlus:
            case TokenKind::MinusMinus:
                lhs = PostfixExpr(tok, lhs);
                next_token();
                break;
            case TokenKind::Question: {
                auto then_expr = parse_expr();
                consume(TokenKind::Colon);
                auto else_expr = parse_expr(static_cast<int>(BinOpPrecedence::Prefix) - 1);
                lhs = TernaryExpr(tok, lhs, then_expr, else_expr);
                break;
            }
            case TokenKind::LParen: {
                std::vector<Expr> args;

                if (!kind(TokenKind::RParen)) {
                    do {
                        args.push_back(parse_expr());
                    } while (kind(TokenKind::Comma));
                    consume(TokenKind::RParen);
                    next_token();
                }

                lhs = CallExpr(lhs, args);
                break;
            }
            case TokenKind::LBracket:
                lhs = ArrayExpr(lhs, parse_expr());
                break;
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
                lhs = BinaryExpr(tok, lhs, parse_expr(static_cast<int>(get_op_precedence(tok.get_kind())) - 1));
                break;
            case TokenKind::RParen:
            case TokenKind::RBracket:
            case TokenKind::Comma:
                goto end;
            default:
                auto prec = static_cast<int>(get_op_precedence(tok.get_kind()));
                if (prec == 0)
                    throw std::runtime_error("invalid operator");

                lhs = BinaryExpr(tok, lhs, parse_expr(static_cast<int>(prec)));
                break;
        }
    }
    end:
    return lhs;
}

Expr Parser::parse_num() {
    auto val = tok.get_val();
    auto len = tok.get_len();

    for (int i = 0; i < len; i++) {
        if (val[i] == '.') {
            double num;
            std::from_chars(val, val + len, num);
            return FloatExpr(tok, num);
        }
    }

    auto base = 10;
    if (len >= 3 && *val == '0') {
        switch (val[1]) {
            case 'b':
            case 'B':
                base = 2;
                break;
            case 'o':
            case 'O':
                base = 8;
                break;
            case 'x':
            case 'X':
                base = 16;
                break;
        }
    }
    
    long long num;
    if (base != 10) {
        std::from_chars(val + 2, val + len, num, base);
    } else {
        std::from_chars(val, val + len, num);
    }

    return IntExpr(tok, num);
}

ForExpr Parser::parse_for_expr() {
    assert(TokenKind::KwFor);

    consume(TokenKind::Identifier);
    auto ident = Identifier(tok);

    consume(TokenKind::KwIn);
    next_token();

    auto expr = parse_expr();
    auto block = parse_block();

    return ForExpr(ident, expr, block);
}

IfExpr Parser::parse_if_expr() {
    assert(TokenKind::KwIf);

    auto expr = parse_expr();
    auto block = parse_block();

    if (kind(TokenKind::KwElse)) {
        std::unique_ptr<ElseExpr> else_expr;
        if (kind(TokenKind::KwIf)) {
            auto if_expr = parse_if_expr();
            else_expr = std::make_unique<ElseExpr>(std::make_unique<IfExpr>(std::move(if_expr)));
        } else {
            auto block = parse_block();
            else_expr = std::make_unique<ElseExpr>(block);
        }

        return IfExpr(expr, block, std::move(else_expr));
    }
    return IfExpr(expr, block);
}

LoopExpr Parser::parse_loop_expr() {
    assert(TokenKind::KwLoop);

    std::optional<Expr> expr = std::nullopt;
    if (!kind(TokenKind::LBrace)) {
        expr = std::make_optional(parse_expr());
    }

    auto block = parse_block();
    return LoopExpr(expr, block);
}

WhileExpr Parser::parse_while_expr() {
    assert(TokenKind::KwWhile);

    auto expr = parse_expr();
    auto block = parse_block();
    return WhileExpr(expr, block);
}

void Parser::next_token() {
    tok = lexer.lex_token();
}

void Parser::consume(TokenKind kind) {
    next_token();
    assert(kind);
}

bool Parser::kind(TokenKind kind) {
    next_token();
    return tok.is(kind);
}

void Parser::assert(TokenKind kind) {
    if (!tok.is(kind))
        throw std::runtime_error(std::format("unexpected token {}, wanted {}", tok_kind_to_string(tok.get_kind()), tok_kind_to_string(kind)));
}