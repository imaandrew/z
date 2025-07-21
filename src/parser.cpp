#include "parser.h"
#include "ast.h"
#include "error.h"
#include "token.h"
#include "type.h"
#include <charconv>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

BinOpPrecedence get_op_precedence(const TokenKind kind) {
    switch (kind) {
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
        case TokenKind::Colon:
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
        case TokenKind::ColonColon:
            return BinOpPrecedence::ScopeRes;
        case TokenKind::LParen:
        case TokenKind::LBracket:
        case TokenKind::PlusPlus:
        case TokenKind::MinusMinus:
        case TokenKind::Dot:
            return BinOpPrecedence::Postfix;
        default:
            return BinOpPrecedence::Unknown;
    }
}

} // namespace

std::vector<std::unique_ptr<Decl>> Parser::parse() {
    next_token();

    std::vector<std::unique_ptr<Decl>> decls;

    DeclResult decl(false);
    while (!tok.is(TokenKind::Eof)) {
        switch (tok.get_kind()) {
            case TokenKind::KwStruct:
                decl = parse_struct_decl();
                break;
            case TokenKind::KwEnum:
                decl = parse_enum_decl();
                break;
            case TokenKind::KwConst:
                decl = parse_const_decl();
                break;
            case TokenKind::KwStatic:
                decl = parse_static_decl();
                break;
            case TokenKind::KwFn:
                decl = parse_func_decl();
                break;
            default:
                diag.emit(tok, ErrorKind::ExpectedDecl,
                          tok_kind_to_string(tok.get_kind()));
                recover_decl();
                break;
            }

            if (decl.is_valid()) {
                decls.push_back(decl.take());
            } else {
                recover_decl();
            }
    }

    return decls;
}

DeclResult Parser::parse_struct_decl() {
    assert(TokenKind::KwStruct);

    if (!consume(TokenKind::Identifier)) {
        return DeclError();
    }

    auto ident = std::make_unique<Identifier>(tok);

    if (!consume(TokenKind::LBrace)) {
        return DeclError();
    }

    std::vector<std::unique_ptr<StructField>> fields;
    next_token();
    while (!tok.is(TokenKind::RBrace)) {
        auto struct_field = parse_struct_field();

        if (!struct_field.is_valid()) {
            return DeclError();
        }

        fields.push_back(struct_field.take());

        if (!tok.is(TokenKind::RBrace)) {
            assert(TokenKind::Comma);
            next_token();
        }
    }

    next_token();

    return DeclResult(
        std::make_unique<StructDecl>(std::move(ident), std::move(fields)));
}

Result<std::unique_ptr<StructField>> Parser::parse_struct_field() {
    if (!assert(TokenKind::Identifier)) {
        return Result<std::unique_ptr<StructField>>(false);
    }
    auto ident = std::make_unique<Identifier>(tok);

    if (!consume(TokenKind::Colon)) {
        return Result<std::unique_ptr<StructField>>(false);
    }

    auto type = prime_parse_type();
    if (!type.is_valid()) {
        return Result<std::unique_ptr<StructField>>(false);
    }

    return Result(std::make_unique<StructField>(std::move(ident), type.take()));
}

DeclResult Parser::parse_enum_decl() {
    assert(TokenKind::KwEnum);

    if (!consume(TokenKind::Identifier)) {
        return DeclError();
    }

    auto ident = std::make_unique<Identifier>(tok);

    if (!consume(TokenKind::LBrace)) {
        return DeclError();
    }

    std::vector<std::unique_ptr<EnumField>> fields;
    while (!kind(TokenKind::RBrace)) {
        auto enum_field = parse_enum_field();
        if (!enum_field.is_valid()) {
            return DeclError();
        }

        fields.push_back(enum_field.take());

        if (!tok.is(TokenKind::RBrace)) {
            assert(TokenKind::Comma);
        }
    }

    next_token();

    return DeclResult(
        std::make_unique<EnumDecl>(std::move(ident), std::move(fields)));
}

Result<std::unique_ptr<EnumField>> Parser::parse_enum_field() {
    if (!assert(TokenKind::Identifier)) {
        return Result<std::unique_ptr<EnumField>>(false);
    }

    auto ident = std::make_unique<Identifier>(tok);

    if (kind(TokenKind::LParen)) {
        std::vector<std::shared_ptr<Type>> types;
        while (!consume(TokenKind::RParen)) {
            auto type = parse_type();

            if (!type.is_valid()) {
                return Result<std::unique_ptr<EnumField>>(false);
            }

            types.push_back(type.take());

            if (!tok.is(TokenKind::RParen) && !assert(TokenKind::Comma)) {
                return Result<std::unique_ptr<EnumField>>(false);
            }
        }

        return Result(
            std::make_unique<EnumField>(std::move(ident), std::move(types)));
    }

    return Result(std::make_unique<EnumField>(std::move(ident)));
}

DeclResult Parser::parse_const_decl() {
    assert(TokenKind::KwConst);

    if (!consume(TokenKind::Identifier))
        return DeclError();

    auto ident = std::make_unique<Identifier>(tok);

    if (!consume(TokenKind::Colon))
        return DeclError();

    auto type = prime_parse_type();
    if (!type.is_valid())
        return DeclError();

    if (!assert(TokenKind::Eq))
        return DeclError();

    auto expr = prime_parse_expr();
    if (!expr.is_valid())
        return DeclError();

    if (assert(TokenKind::Semi))
        next_token();

    return DeclResult(std::make_unique<ConstDecl>(std::move(ident), type.take(),
                                                  expr.take()));
}

DeclResult Parser::parse_static_decl() {
    assert(TokenKind::KwStatic);

    if (!consume(TokenKind::Identifier))
        return DeclError();

    auto ident = std::make_unique<Identifier>(tok);

    if (!consume(TokenKind::Colon))
        return DeclError();

    auto type = prime_parse_type();
    if (!type.is_valid())
        return DeclError();

    if (!assert(TokenKind::Eq))
        return DeclError();

    auto expr = prime_parse_expr();
    if (!expr.is_valid())
        return DeclError();

    if (assert(TokenKind::Semi))
        next_token();

    return DeclResult(std::make_unique<StaticDecl>(std::move(ident),
                                                   type.take(), expr.take()));
}

DeclResult Parser::parse_func_decl() {
    assert(TokenKind::KwFn);

    if (!consume(TokenKind::Identifier))
        return DeclError();

    const Token t = tok;
    std::unique_ptr<Identifier> func_ident;

    std::optional<std::unique_ptr<Identifier>> impl_type;
    if (kind(TokenKind::ColonColon)) {
        if (!consume(TokenKind::Identifier))
            return DeclError();

        impl_type = std::make_unique<Identifier>(t);
        func_ident = std::make_unique<Identifier>(tok);
        next_token();
    } else {
        func_ident = std::make_unique<Identifier>(t);
    }

    auto params = parse_func_params();
    if (!params.is_valid())
        return DeclError();

    std::unique_ptr<Type> ret;
    if (tok.is(TokenKind::Arrow)) {
        auto type = prime_parse_type();
        if (!type.is_valid())
            return DeclError();

        ret = type.take();
    } else {
        ret = std::make_unique<VoidType>();
    }

    auto block = parse_block();
    if (!block.is_valid())
        return DeclError();

    return DeclResult(std::make_unique<FuncDecl>(
        std::move(func_ident), std::move(impl_type), params.take(),
        std::move(ret), block.take()));
}

Result<std::vector<std::unique_ptr<Param>>> Parser::parse_func_params() {
    assert(TokenKind::LParen);

    std::vector<std::unique_ptr<Param>> params;
    next_token();
    while (!tok.is(TokenKind::RParen)) {
        auto param_decl = parse_param_decl();
        if (!param_decl.is_valid())
            return Result<std::vector<std::unique_ptr<Param>>>(false);

        params.push_back(param_decl.take());

        if (!tok.is(TokenKind::RParen) && !assert(TokenKind::Comma)) {
            return Result<std::vector<std::unique_ptr<Param>>>(false);
        }
    }

    next_token();

    return Result(std::move(params));
}

Result<std::unique_ptr<Param>> Parser::parse_param_decl() {
    if (!assert(TokenKind::Identifier))
        return Result<std::unique_ptr<Param>>(false);

    auto name = std::make_unique<Identifier>(tok);

    if (!consume(TokenKind::Colon))
        return Result<std::unique_ptr<Param>>(false);

    auto type = prime_parse_type();
    if (!type.is_valid())
        return Result<std::unique_ptr<Param>>(false);

    return Result(std::make_unique<Param>(std::move(name), type.take()));
}

Result<std::unique_ptr<Block>> Parser::parse_block(const bool implicit_return) {
    assert(TokenKind::LBrace);
    next_token();

    std::vector<std::unique_ptr<Stmt>> stmts;
    while (!tok.is(TokenKind::RBrace)) {
        if (tok.is(TokenKind::Semi)) {
            next_token();
            continue;
        }

        auto res = parse_stmt();
        if (res.is_valid()) {
            stmts.push_back(res.take());
        } else {
            recover_stmt();
            continue;
        }

        if (implicit_return && tok.is(TokenKind::RBrace))
            break;

        if (required_semi) {
            if (assert(TokenKind::Semi))
                next_token();
        } else {
            required_semi = true;
            next_token();
        }
    }

    next_token();

    return Result(std::make_unique<Block>(std::move(stmts)));
}

StmtResult Parser::parse_stmt() {
    std::unique_ptr<Stmt> stmt;

    if (tok.is(TokenKind::KwBreak)) {
        next_token();
        if (can_be_expr()) {
            auto expr = parse_expr();
            if (!expr.is_valid())
                return StmtError();

            stmt = std::make_unique<BreakStmt>(tok, expr.take());
        } else {
            stmt = std::make_unique<BreakStmt>(tok);
        }
    } else if (tok.is(TokenKind::KwContinue)) {
        stmt = std::make_unique<ContinueStmt>(tok);
        next_token();
    } else if (tok.is(TokenKind::KwLet)) {
        if (!consume(TokenKind::Identifier))
            return StmtError();

        auto ident = std::make_unique<Identifier>(tok);

        if (kind(TokenKind::Colon)) {
            auto type = prime_parse_type();
            if (!type.is_valid())
                return StmtError();

            if (!assert(TokenKind::Eq))
                return StmtError();

            auto expr = prime_parse_expr();
            if (!expr.is_valid())
                return StmtError();

            stmt = std::make_unique<LetStmt>(std::move(ident), type.take(),
                                             expr.take());
        } else if (tok.is(TokenKind::Eq)) {
            auto expr = prime_parse_expr();
            if (!expr.is_valid())
                return StmtError();

            stmt = std::make_unique<LetStmt>(std::move(ident), expr.take());
        } else {
            stmt = std::make_unique<LetStmt>(std::move(ident));
        }
    } else if (tok.is(TokenKind::KwReturn)) {
        next_token();

        if (can_be_expr()) {
            auto expr = prime_parse_expr();
            if (!expr.is_valid())
                return StmtError();

            stmt = std::make_unique<ReturnStmt>(expr.take());
        } else {
            stmt = std::make_unique<ReturnStmt>();
        }
    } else {
        const auto expr_no_semi =
            tok.is(TokenKind::KwIf) || tok.is(TokenKind::KwFor) ||
            tok.is(TokenKind::KwLoop) || tok.is(TokenKind::KwWhile);
        auto expr = parse_expr();
        if (!expr.is_valid())
            return StmtError();

        stmt = expr.take();
        required_semi = !expr_no_semi;
    }
    
    return Result(std::move(stmt));
}

ExprResult Parser::prime_parse_expr(const int precedence) {
    next_token();
    return parse_expr(precedence);
}

ExprResult Parser::parse_expr(const int precedence) {
    std::unique_ptr<Expr> lhs;
    switch (tok.get_kind()) {
        case TokenKind::Number:
            lhs = parse_num();
            next_token();
            break;
        case TokenKind::Identifier:
            lhs = std::make_unique<Identifier>(tok);
            next_token();
            break;
        case TokenKind::String: {
            lhs = std::make_unique<StringExpr>(tok.get_val(), tok.get_len());
            next_token();
            break;
        }
        case TokenKind::PlusPlus:
        case TokenKind::MinusMinus:
        case TokenKind::LogicalNot:
        case TokenKind::Not:
        case TokenKind::Minus: {
            auto prefix_tok = tok;
            auto expr =
                prime_parse_expr(static_cast<int>(BinOpPrecedence::Prefix));
            if (!expr.is_valid())
                return ExprError();

            lhs = std::make_unique<PrefixExpr>(prefix_tok, expr.take());
            break;
        }
        case TokenKind::LParen: {
            auto expr = prime_parse_expr();
            if (!expr.is_valid())
                return ExprError();

            lhs = expr.take();
            if (!assert(TokenKind::RParen))
                return ExprError();

            next_token();
            break;
        }
        case TokenKind::LBrace: {
            std::vector<std::unique_ptr<Expr>> vals;

            while (!kind(TokenKind::RBrace)) {
                auto expr = parse_expr();
                if (!expr.is_valid())
                    return ExprError();

                vals.push_back(expr.take());

                if (!tok.is(TokenKind::Comma)) {
                    if (!assert(TokenKind::RBrace))
                        return ExprError();
                    break;
                }
            }

            next_token();
            lhs = std::make_unique<ArrayInitExpr>(std::move(vals));
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
            return ExprError();
    }

    while (precedence < static_cast<int>(get_op_precedence(tok.get_kind()))) {
        switch (tok.get_kind()) {
            case TokenKind::PlusPlus:
            case TokenKind::MinusMinus:
                lhs = std::make_unique<PostfixExpr>(tok, std::move(lhs));
                next_token();
                break;
            case TokenKind::Question: {
                auto operator_tok = tok;

                auto then_expr = prime_parse_expr();
                if (!then_expr.is_valid())
                    return ExprError();

                if (!consume(TokenKind::Colon))
                    return ExprError();

                auto else_expr = parse_expr(static_cast<int>(BinOpPrecedence::Prefix) - 1);
                if (!else_expr.is_valid())
                    return ExprError();

                lhs = std::make_unique<TernaryExpr>(
                    operator_tok, std::move(lhs), then_expr.take(),
                    else_expr.take());
                break;
            }
            case TokenKind::LParen: {
                std::vector<std::unique_ptr<Expr>> args;

                while (!kind(TokenKind::RParen)) {
                    auto expr = parse_expr();
                    if (!expr.is_valid())
                        return ExprError();

                    args.push_back(expr.take());

                    if (!tok.is(TokenKind::Comma)) {
                        if (!assert(TokenKind::RParen))
                            return ExprError();
                        break;
                    }
                }

                next_token();
                lhs =
                    std::make_unique<CallExpr>(std::move(lhs), std::move(args));
                break;
            }
            case TokenKind::LBracket:
                if (kind(TokenKind::RBracket)) {
                    lhs = std::make_unique<ArrayExpr>(std::move(lhs));
                } else {
                    auto expr = parse_expr();
                    if (!expr.is_valid())
                        return ExprError();

                    lhs = std::make_unique<ArrayExpr>(std::move(lhs),
                                                      expr.take());
                    if (!assert(TokenKind::RBracket))
                        return ExprError();
                }
                next_token();
                break;
            case TokenKind::LBrace: {
                std::vector<std::unique_ptr<Expr>> vals;

                while (!kind(TokenKind::RBrace)) {
                    auto expr = parse_expr();
                    if (!expr.is_valid())
                        return ExprError();

                    vals.push_back(expr.take());

                    if (!tok.is(TokenKind::Comma)) {
                        if (!assert(TokenKind::RBrace))
                            return ExprError();
                        break;
                    }
                }

                next_token();
                lhs = std::make_unique<StructInitExpr>(std::move(lhs),
                                                       std::move(vals));
                break;
            }
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
            case TokenKind::Colon: {
                auto operator_tok = tok;
                auto expr = prime_parse_expr(
                    static_cast<int>(get_op_precedence(tok.get_kind())) - 1);
                if (!expr.is_valid())
                    return ExprError();

                lhs = std::make_unique<BinaryExpr>(operator_tok, std::move(lhs),
                                                   expr.take());
                break;
            }
            default: {
                auto prec = static_cast<int>(get_op_precedence(tok.get_kind()));
                if (prec == 0)
                    throw std::runtime_error("invalid operator");

                auto operator_tok = tok;
                auto expr = prime_parse_expr(prec);
                if (!expr.is_valid())
                    return ExprError();

                lhs = std::make_unique<BinaryExpr>(operator_tok, std::move(lhs),
                                                   expr.take());
                break;
            }
        }
    }
    return Result(std::move(lhs));
}

std::unique_ptr<Expr> Parser::parse_num() const {
    const auto val = std::span(tok.get_val(), tok.get_len());
    const auto len = tok.get_len();

    for (size_t i = 0; i < len; i++) {
        if (val[i] == '.') {
            double num = NAN;
            std::from_chars(val.data(), val.data() + val.size_bytes(), num);
            return std::make_unique<FloatExpr>(tok, num);
        }
    }

    auto base = 10;
    if (len >= 3 && val.front() == '0') {
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
            default:
                base = 10;
        }
    }

    unsigned long long num = 0;
    if (base != 10) {
        std::from_chars(val.data() + 2, val.data() + val.size(), num, base);
    } else {
        std::from_chars(val.data(), val.data() + val.size(), num);
    }

    return std::make_unique<IntExpr>(tok, num);
}

ExprResult Parser::parse_for_expr() {
    assert(TokenKind::KwFor);

    if (!consume(TokenKind::Identifier))
        return ExprError();

    auto ident = std::make_unique<Identifier>(tok);

    if (!consume(TokenKind::KwIn))
        return ExprError();

    auto expr = prime_parse_expr();
    if (!expr.is_valid())
        return ExprError();

    auto block = parse_block(false);
    if (!block.is_valid())
        return ExprError();

    return ExprResult(
        std::make_unique<ForExpr>(std::move(ident), expr.take(), block.take()));
}

ExprResult Parser::parse_if_expr() {
    assert(TokenKind::KwIf);

    auto expr = prime_parse_expr();
    if (!expr.is_valid())
        return ExprError();

    auto block = parse_block();
    if (!block.is_valid())
        return ExprError();

    if (tok.is(TokenKind::KwElse)) {
        std::unique_ptr<ElseExpr> else_expr;

        if (kind(TokenKind::KwIf)) {
            auto if_expr = parse_if_expr();
            if (!if_expr.is_valid())
                return ExprError();

            else_expr = std::make_unique<ElseExpr>(if_expr.take());
        } else {
            auto else_block = parse_block();
            if (!else_block.is_valid())
                return ExprError();

            else_expr = std::make_unique<ElseExpr>(else_block.take());
        }

        return ExprResult(std::make_unique<IfExpr>(expr.take(), block.take(),
                                                   std::move(else_expr)));
    }
    return ExprResult(std::make_unique<IfExpr>(expr.take(), block.take()));
}

ExprResult Parser::parse_loop_expr() {
    assert(TokenKind::KwLoop);

    ExprResult expr;

    if (!kind(TokenKind::LBrace)) {
        expr = parse_expr();
        if (!expr.is_valid())
            return ExprError();
    }

    auto block = parse_block(false);
    if (!block.is_valid())
        return ExprError();

    return ExprResult(std::make_unique<LoopExpr>(expr.take(), block.take()));
}

ExprResult Parser::parse_while_expr() {
    assert(TokenKind::KwWhile);

    auto expr = prime_parse_expr();
    if (!expr.is_valid())
        return ExprError();

    auto block = parse_block(false);
    if (!block.is_valid())
        return ExprError();

    return ExprResult(std::make_unique<WhileExpr>(expr.take(), block.take()));
}

TypeResult Parser::prime_parse_type() {
    next_token();
    return parse_type();
}

TypeResult Parser::parse_type() {
    std::unique_ptr<Type> type;
    if (tok.is(TokenKind::Identifier)) {

        if (const auto val_str = std::string(tok.get_val(), tok.get_len());
            val_str.at(0) == 'u') {
            if (val_str == "u8") {
                type = std::make_unique<IntegerType>(8, false);
            } else if (val_str == "u16") {
                type = std::make_unique<IntegerType>(16, false);
            } else if (val_str == "u32") {
                type = std::make_unique<IntegerType>(32, false);
            } else if (val_str == "u64") {
                type = std::make_unique<IntegerType>(64, false);
            }
        } else if (val_str.at(0) == 'i') {
            if (val_str == "i8") {
                type = std::make_unique<IntegerType>(8, true);
            } else if (val_str == "i16") {
                type = std::make_unique<IntegerType>(16, true);
            } else if (val_str == "i32") {
                type = std::make_unique<IntegerType>(32, true);
            } else if (val_str == "i64") {
                type = std::make_unique<IntegerType>(64, true);
            }
        } else if (val_str.at(0) == 'f') {
            if (val_str == "f32") {
                type = std::make_unique<FloatType>(32);
            } else if (val_str == "f64") {
                type = std::make_unique<FloatType>(64);
            }
        } else if (val_str == "bool") {
            type = std::make_unique<BooleanType>();
        } else if (val_str == "str") {
            type = std::make_unique<StringType>();
        } else if (val_str == "char") {
            type = std::make_unique<CharType>();
        } else {
            type = std::make_unique<UnknownType>(
                std::make_unique<Identifier>(tok));
        }

        while (kind(TokenKind::Star)) {
            type = std::make_unique<PointerType>(std::move(type));
        }
    } else if (tok.is(TokenKind::LBracket)) {
        auto array_type = prime_parse_type();
        if (!array_type.is_valid())
            return TypeError();

        if (tok.is(TokenKind::Semi)) {
            auto size = prime_parse_expr();
            if (!size.is_valid())
                return TypeError();

            type = std::make_unique<ArrayType>(array_type.take(), size.take());
        } else {
            type = std::make_unique<ArrayType>(array_type.take());
        }
        assert(TokenKind::RBracket);
        next_token();
    } else if (tok.is(TokenKind::LParen)) {
        std::vector<std::unique_ptr<Type>> types;
        // TODO: don't think this is ever reached

        while (!kind(TokenKind::RParen)) {
            auto elem_type = parse_type();
            if (!elem_type.is_valid())
                return TypeError();

            types.push_back(elem_type.take());

            if (!tok.is(TokenKind::Comma)) {
                if (!assert(TokenKind::RParen))
                    return TypeError();

                next_token();
                break;
            }
        }
    } else {
        return TypeError();
    }

    return TypeResult(std::move(type));
}

void Parser::next_token() {
    prev_tok = tok;
    tok = lexer.lex_token();
}

bool Parser::consume(const TokenKind kind) {
    next_token();
    return assert(kind);
}

bool Parser::kind(const TokenKind kind) {
    next_token();
    return tok.is(kind);
}

bool Parser::assert(const TokenKind kind) {
    if (!tok.is(kind)) {
        if (kind == TokenKind::Semi) {
            const auto semi_tok = Token(
                TokenKind::Semi, prev_tok.get_val(), prev_tok.get_pos() + 1,
                prev_tok.get_line(), prev_tok.get_col() + 1);
            diag.emit(semi_tok, ErrorKind::ExpectedSemi,
                      tok_kind_to_string(kind),
                      tok_kind_to_string(semi_tok.get_kind()));
        } else {
            diag.emit(tok, ErrorKind::ExpectedToken, tok_kind_to_string(kind),
                      tok_kind_to_string(tok.get_kind()));
        }
        return false;
    }

    return true;
}

bool Parser::sync(const TokenKind kind, const SyncFlags flags) {
    return sync(std::vector<TokenKind>{kind}, flags);
}

bool Parser::sync(std::vector<TokenKind> const& kinds, const SyncFlags flags) {
    int braces = 0;
    while (!tok.is(TokenKind::Eof)) {
        for (const auto kind : kinds) {
            if (tok.is(kind) && braces <= 0) {
                if ((flags & SyncFlags::BreakBefore) != SyncFlags::BreakBefore)
                    next_token();
                return true;
            }
        }

        if (tok.is(TokenKind::Semi) &&
            (flags & SyncFlags::StopAtSemi) == SyncFlags::StopAtSemi) {
            if ((flags & SyncFlags::BreakBefore) != SyncFlags::BreakBefore)
                next_token();
            return true;
        }

        switch (tok.get_kind()) {
        case TokenKind::LBrace:
            braces += 1;
            break;
        case TokenKind::RBrace:
            if (braces <= 1) {
                next_token();
                return false;
            }

            braces -= 1;
        default:
            break;
        }

        next_token();
    }

    return false;
}

void Parser::recover_decl() {
    while (!tok.is(TokenKind::Eof)) {
        switch (tok.get_kind()) {
        case TokenKind::Semi:
            next_token();
            return;
        case TokenKind::LBrace:
            next_token();
            sync(TokenKind::RBrace);
            return;
        default:
            next_token();
            break;
        }
    }
}

void Parser::recover_stmt() {
    int braces = 0;

    while (!tok.is(TokenKind::Eof)) {
        switch (tok.get_kind()) {
        case TokenKind::LBrace:
            braces += 1;
            break;
        case TokenKind::RBrace:
            if (braces == 0)
                return;

            braces -= 1;
            break;
        default:
            break;
        }
        next_token();
    }
}

bool Parser::can_be_expr() const {
    switch (tok.get_kind()) {
    case TokenKind::Number:
    case TokenKind::Identifier:
    case TokenKind::String:
    case TokenKind::PlusPlus:
    case TokenKind::MinusMinus:
    case TokenKind::LogicalNot:
    case TokenKind::Not:
    case TokenKind::Minus:
    case TokenKind::LParen:
    case TokenKind::LBrace:
    case TokenKind::KwFor:
    case TokenKind::KwIf:
    case TokenKind::KwLoop:
    case TokenKind::KwWhile:
        return true;
    default:
        return false;
    }
}