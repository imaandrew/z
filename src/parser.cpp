#include "parser.h"
#include "ast.h"
#include "diagnostics.h"
#include "src_mgr.h"
#include "token.h"
#include "type.h"
#include <charconv>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace z {

namespace {

ast::BinOpPrecedence get_op_precedence(const TokenKind kind) {
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
        return ast::BinOpPrecedence::Assignment;
    case TokenKind::Range:
    case TokenKind::RangeEq:
        return ast::BinOpPrecedence::Range;
    case TokenKind::Question:
        return ast::BinOpPrecedence::Conditional;
    case TokenKind::OrOr:
        return ast::BinOpPrecedence::LogicalOr;
    case TokenKind::AndAnd:
        return ast::BinOpPrecedence::LogicalAnd;
    case TokenKind::EqEq:
    case TokenKind::Ne:
    case TokenKind::Gt:
    case TokenKind::Lt:
    case TokenKind::Ge:
    case TokenKind::Le:
        return ast::BinOpPrecedence::Equality;
    case TokenKind::Or:
        return ast::BinOpPrecedence::Or;
    case TokenKind::Caret:
        return ast::BinOpPrecedence::Xor;
    case TokenKind::And:
        return ast::BinOpPrecedence::And;
    case TokenKind::Shl:
    case TokenKind::Shr:
        return ast::BinOpPrecedence::Shift;
    case TokenKind::Plus:
    case TokenKind::Minus:
        return ast::BinOpPrecedence::Addition;
    case TokenKind::Star:
    case TokenKind::Slash:
    case TokenKind::Percent:
        return ast::BinOpPrecedence::Multiplication;
    case TokenKind::ColonColon:
        return ast::BinOpPrecedence::ScopeRes;
    case TokenKind::LParen:
    case TokenKind::LBracket:
    case TokenKind::LBrace:
    case TokenKind::PlusPlus:
    case TokenKind::MinusMinus:
    case TokenKind::Dot:
        return ast::BinOpPrecedence::Postfix;
    default:
        return ast::BinOpPrecedence::Unknown;
    }
}
} // namespace

std::unique_ptr<ast::SourceFileDecl> Parser::parse() {
    next_token();
    auto span = tok.get_span();

    std::vector<std::unique_ptr<ast::Decl>> decls;

    DeclResult decl;
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
        case TokenKind::KwTrait:
            decl = parse_trait_decl();
            break;
        case TokenKind::KwLet:
            decl = parse_type_alias_decl();
            break;
        case TokenKind::KwFn:
            decl = parse_func_decl();
            break;
        default:
            diag.emit(tok.get_span(), DiagnosticKind::ExpectedDecl,
                      tok_kind_to_string(tok.get_kind()));
            recover_decl();
            continue;
        }

        if (decl.is_valid()) {
            decls.push_back(decl.take());
        } else {
            recover_decl();
        }
    }

    span += tok.get_span();
    return std::make_unique<ast::SourceFileDecl>(span, std::move(decls));
}

DeclResult Parser::parse_struct_decl() {
    tok_assert(TokenKind::KwStruct);
    Span span = tok.get_span();

    if (!consume(TokenKind::Identifier)) {
        return DeclError();
    }

    auto ident = parse_ident_unchecked();

    if (!consume(TokenKind::LBrace)) {
        return DeclError();
    }

    std::vector<std::unique_ptr<ast::StructField>> fields;
    std::vector<std::unique_ptr<ast::Decl>> funcs;

    next_token();
    while (!tok.is(TokenKind::RBrace)) {
        if (tok.is(TokenKind::KwFn)) {
            auto fn = parse_func_decl();
            if (!fn.is_valid())
                return DeclError();

            funcs.push_back(fn.take());
            continue;
        }

        auto struct_field = parse_struct_field();

        if (!struct_field.is_valid()) {
            return DeclError();
        }

        fields.push_back(struct_field.take());

        if (!tok.is(TokenKind::RBrace) && !tok.is(TokenKind::KwFn)) {
            tok_assert(TokenKind::Comma);
            next_token();
        }
    }

    span += tok.get_span();
    next_token();

    return DeclResult(std::make_unique<ast::StructDecl>(
        span, std::move(ident), std::move(fields), std::move(funcs)));
}

Result<std::unique_ptr<ast::StructField>> Parser::parse_struct_field() {
    if (!tok_assert(TokenKind::Identifier)) {
        return Result<std::unique_ptr<ast::StructField>>();
    }
    Span span = tok.get_span();
    auto ident = parse_ident_unchecked();

    if (!consume(TokenKind::Colon)) {
        return Result<std::unique_ptr<ast::StructField>>();
    }

    auto type = prime_parse_type();
    if (!type.is_valid()) {
        return Result<std::unique_ptr<ast::StructField>>();
    }

    span += prev_tok.get_span();
    return Result(std::make_unique<ast::StructField>(span, std::move(ident),
                                                     type.take()));
}

DeclResult Parser::parse_enum_decl() {
    tok_assert(TokenKind::KwEnum);
    Span span = tok.get_span();

    if (!consume(TokenKind::Identifier)) {
        return DeclError();
    }

    auto ident = parse_ident_unchecked();

    if (!consume(TokenKind::LBrace)) {
        return DeclError();
    }
    next_token();

    std::vector<std::unique_ptr<ast::EnumField>> fields;
    while (!tok.is(TokenKind::RBrace)) {
        auto enum_field = parse_enum_field();
        if (!enum_field.is_valid()) {
            return DeclError();
        }

        fields.push_back(enum_field.take());

        if (!tok.is(TokenKind::RBrace)) {
            tok_assert(TokenKind::Comma);
            next_token();
        }
    }

    span += tok.get_span();
    next_token();

    return DeclResult(std::make_unique<ast::EnumDecl>(span, std::move(ident),
                                                      std::move(fields)));
}

Result<std::unique_ptr<ast::EnumField>> Parser::parse_enum_field() {
    if (!tok_assert(TokenKind::Identifier)) {
        return Result<std::unique_ptr<ast::EnumField>>();
    }

    Span span = tok.get_span();
    auto ident = parse_ident_unchecked();

    if (kind(TokenKind::LParen)) {
        std::vector<std::shared_ptr<type::Type>> types;
        next_token();
        while (!tok.is(TokenKind::RParen)) {
            auto type = parse_type();

            if (!type.is_valid()) {
                return Result<std::unique_ptr<ast::EnumField>>();
            }

            types.push_back(type.take());

            if (!tok.is(TokenKind::RParen)) {
                if (!tok_assert(TokenKind::Comma))
                    return Result<std::unique_ptr<ast::EnumField>>();

                next_token();
            }
        }

        next_token();
        span += prev_tok.get_span();
        return Result(std::make_unique<ast::EnumField>(span, std::move(ident),
                                                       std::move(types)));
    }

    return Result(std::make_unique<ast::EnumField>(span, std::move(ident)));
}

DeclResult Parser::parse_const_decl() {
    tok_assert(TokenKind::KwConst);
    Span span = tok.get_span();

    if (!consume(TokenKind::Identifier))
        return DeclError();

    auto ident = parse_ident_unchecked();

    if (!consume(TokenKind::Colon))
        return DeclError();

    auto type = prime_parse_type();
    if (!type.is_valid())
        return DeclError();

    if (!tok_assert(TokenKind::Eq))
        return DeclError();

    auto expr = prime_parse_expr();
    if (!expr.is_valid())
        return DeclError();

    span += tok.get_span();
    if (tok_assert(TokenKind::Semi))
        next_token();

    return DeclResult(std::make_unique<ast::ConstDecl>(
        span, std::move(ident), type.take(), expr.take()));
}

DeclResult Parser::parse_static_decl() {
    tok_assert(TokenKind::KwStatic);
    Span span = tok.get_span();

    if (!consume(TokenKind::Identifier))
        return DeclError();

    auto ident = parse_ident_unchecked();

    if (!consume(TokenKind::Colon))
        return DeclError();

    auto type = prime_parse_type();
    if (!type.is_valid())
        return DeclError();

    if (!tok_assert(TokenKind::Eq))
        return DeclError();

    auto expr = prime_parse_expr();
    if (!expr.is_valid())
        return DeclError();

    span += tok.get_span();
    if (tok_assert(TokenKind::Semi))
        next_token();

    return DeclResult(std::make_unique<ast::StaticDecl>(
        span, std::move(ident), type.take(), expr.take()));
}

DeclResult Parser::parse_trait_decl() {
    tok_assert(TokenKind::KwTrait);
    Span span = tok.get_span();

    if (!consume(TokenKind::Identifier))
        return DeclError();

    auto ident = parse_ident_unchecked();

    if (!consume(TokenKind::LBrace))
        return DeclError();

    next_token();

    std::vector<std::unique_ptr<ast::Decl>> consts;
    std::vector<std::unique_ptr<ast::Decl>> types;
    std::vector<std::unique_ptr<ast::Decl>> funcs;
    while (!tok.is(TokenKind::RBrace)) {
        if (tok.is(TokenKind::KwConst)) {
            auto c = parse_const_decl();
            if (!c.is_valid())
                return DeclError();
            consts.push_back(c.take());
        } else if (tok.is(TokenKind::KwLet)) {
            auto t = parse_type_alias_decl();
            if (!t.is_valid())
                return DeclError();
            types.push_back(t.take());
        } else if (tok.is(TokenKind::KwFn)) {
            auto f = parse_trait_func_decl();
            if (!f.is_valid())
                return DeclError();
            funcs.push_back(f.take());
        } else {
            return DeclError();
        }
    }

    span += tok.get_span();
    next_token();
    return DeclResult(std::make_unique<ast::TraitDecl>(
        span, std::move(ident), std::move(consts), std::move(types),
        std::move(funcs), syms->new_scope()));
}

DeclResult Parser::parse_type_alias_decl() {
    tok_assert(TokenKind::KwLet);
    Span span = tok.get_span();

    if (!consume(TokenKind::Identifier))
        return DeclError();

    auto ident = parse_ident_unchecked();
    if (!ident->is_valid())
        return DeclError();

    if (!consume(TokenKind::Eq))
        return DeclError();

    auto type = prime_parse_type();
    if (!type.is_valid())
        return DeclError();

    span += tok.get_span();
    if (tok_assert(TokenKind::Semi))
        next_token();

    return DeclResult(std::make_unique<ast::TypeAliasDecl>(
        span, std::move(ident), type.take()));
}

DeclResult Parser::parse_trait_func_decl() {
    tok_assert(TokenKind::KwFn);
    Span span = tok.get_span();

    if (!consume(TokenKind::Identifier))
        return DeclError();

    auto func_ident = parse_ident_unchecked();
    next_token();

    auto params = parse_func_params();
    if (!params.is_valid())
        return DeclError();

    std::unique_ptr<type::Type> ret;
    if (tok.is(TokenKind::Arrow)) {
        auto type = prime_parse_type();
        if (!type.is_valid())
            return DeclError();

        ret = type.take();
    } else {
        ret = std::make_unique<type::VoidType>();
    }

    if (tok.is(TokenKind::LBrace)) {
        auto block = parse_block();
        if (!block.is_valid())
            return DeclError();

        span += prev_tok.get_span();
        return DeclResult(std::make_unique<ast::TraitFuncDecl>(
            span, std::move(func_ident), params.take(), std::move(ret),
            block.take()));
    }

    tok_assert(TokenKind::Semi);
    span += tok.get_span();
    next_token();
    return DeclResult(std::make_unique<ast::TraitFuncDecl>(
        span, std::move(func_ident), params.take(), std::move(ret), nullptr));
}

DeclResult Parser::parse_func_decl() {
    tok_assert(TokenKind::KwFn);
    Span span = tok.get_span();

    if (!consume(TokenKind::Identifier))
        return DeclError();

    auto func_ident = parse_ident_unchecked();
    next_token();

    auto params = parse_func_params();
    if (!params.is_valid())
        return DeclError();

    std::unique_ptr<type::Type> ret;
    if (tok.is(TokenKind::Arrow)) {
        auto type = prime_parse_type();
        if (!type.is_valid())
            return DeclError();

        ret = type.take();
    } else {
        ret = std::make_unique<type::VoidType>();
    }

    auto block = parse_block();
    if (!block.is_valid())
        return DeclError();

    span += prev_tok.get_span();
    return DeclResult(std::make_unique<ast::FuncDecl>(
        span, std::move(func_ident), params.take(), std::move(ret),
        block.take()));
}

Result<std::vector<std::unique_ptr<ast::Param>>> Parser::parse_func_params() {
    tok_assert(TokenKind::LParen);

    std::vector<std::unique_ptr<ast::Param>> params;
    next_token();
    while (!tok.is(TokenKind::RParen)) {
        auto param_decl = parse_param_decl();
        if (!param_decl.is_valid())
            return Result<std::vector<std::unique_ptr<ast::Param>>>();

        params.push_back(param_decl.take());

        if (!tok.is(TokenKind::RParen)) {
            if (!tok_assert(TokenKind::Comma))
                return Result<std::vector<std::unique_ptr<ast::Param>>>();
            next_token();
        }
    }

    next_token();

    return Result(std::move(params));
}

Result<std::unique_ptr<ast::Param>> Parser::parse_param_decl() {
    if (!tok_assert(TokenKind::Identifier))
        return Result<std::unique_ptr<ast::Param>>();

    Span span = tok.get_span();
    auto name = parse_ident_unchecked();

    if (!consume(TokenKind::Colon))
        return Result<std::unique_ptr<ast::Param>>();

    auto type = prime_parse_type();
    if (!type.is_valid())
        return Result<std::unique_ptr<ast::Param>>();

    span += prev_tok.get_span();
    return Result(
        std::make_unique<ast::Param>(span, std::move(name), type.take()));
}

Result<std::unique_ptr<ast::Block>>
Parser::parse_block(const bool implicit_return) {
    bool is_valid = true;
    tok_assert(TokenKind::LBrace);
    Span span = tok.get_span();
    next_token();

    std::vector<std::unique_ptr<ast::Stmt>> stmts;
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
            if (tok.is(TokenKind::Eof))
                return Result<std::unique_ptr<ast::Block>>();
            continue;
        }

        if (implicit_return && tok.is(TokenKind::RBrace))
            break;

        if (required_semi) {
            if (tok_assert(TokenKind::Semi))
                next_token();
            else
                is_valid = false;
        } else {
            required_semi = true;
            next_token();
        }
    }

    span = span + tok.get_span();

    next_token();

    if (!is_valid)
        return Result<std::unique_ptr<ast::Block>>();
    return Result(std::make_unique<ast::Block>(span, std::move(stmts),
                                               syms->new_scope()));
}

StmtResult Parser::parse_stmt() {
    std::unique_ptr<ast::Stmt> stmt;
    Span span = tok.get_span();

    if (tok.is(TokenKind::KwBreak)) {
        next_token();
        if (can_be_expr()) {
            auto expr = parse_expr();
            if (!expr.is_valid())
                return StmtError();

            span += prev_tok.get_span();
            stmt = std::make_unique<ast::BreakStmt>(span, tok, expr.take());
        } else {
            stmt = std::make_unique<ast::BreakStmt>(span, tok);
        }
    } else if (tok.is(TokenKind::KwContinue)) {
        stmt = std::make_unique<ast::ContinueStmt>(span, tok);
        next_token();
    } else if (tok.is(TokenKind::KwLet)) {
        auto let = parse_let_stmt();
        if (!let.is_valid())
            return StmtError();
        stmt = let.take();
    } else if (tok.is(TokenKind::KwReturn)) {
        next_token();

        if (can_be_expr()) {
            auto expr = parse_expr();
            if (!expr.is_valid())
                return StmtError();

            span += prev_tok.get_span();
            stmt = std::make_unique<ast::ReturnStmt>(span, expr.take());
        } else {
            stmt = std::make_unique<ast::ReturnStmt>(span);
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

StmtResult Parser::parse_let_stmt() {
    tok_assert(TokenKind::KwLet);
    Span span = tok.get_span();

    if (!consume(TokenKind::Identifier))
        return StmtError();

    auto ident = parse_ident_unchecked();

    if (kind(TokenKind::Colon)) {
        auto type = prime_parse_type();
        if (!type.is_valid())
            return StmtError();

        if (tok.is(TokenKind::Eq)) {
            auto eq = tok.get_span();
            if (!tok_assert(TokenKind::Eq))
                return StmtError();

            auto expr = prime_parse_expr();
            if (!expr.is_valid())
                return StmtError();

            span += prev_tok.get_span();
            return StmtResult(std::make_unique<ast::LetStmt>(
                span, std::move(ident), type.take(), expr.take(), eq));
        }

        span += prev_tok.get_span();
        return StmtResult(std::make_unique<ast::LetStmt>(span, std::move(ident),
                                                         type.take()));
    }

    if (tok.is(TokenKind::Eq)) {
        auto expr = prime_parse_expr();
        if (!expr.is_valid())
            return StmtError();

        span += prev_tok.get_span();
        return StmtResult(std::make_unique<ast::LetStmt>(
            span, std::move(ident), expr.take(), tok.get_span()));
    }

    span += prev_tok.get_span();
    return StmtResult(std::make_unique<ast::LetStmt>(span, std::move(ident)));
}

ExprResult Parser::prime_parse_expr(const int precedence,
                                    const std::optional<TokenKind> ignore) {
    next_token();
    return parse_expr(precedence, ignore);
}

ExprResult Parser::parse_expr(const int precedence,
                              const std::optional<TokenKind> ignore) {
    std::unique_ptr<ast::Expr> lhs;
    Span span = tok.get_span();
    switch (tok.get_kind()) {
    case TokenKind::Number:
        lhs = parse_num();
        next_token();
        break;
    case TokenKind::KwTrue:
        lhs = std::make_unique<ast::BoolExpr>(tok, true);
        next_token();
        break;
    case TokenKind::KwFalse:
        lhs = std::make_unique<ast::BoolExpr>(tok, false);
        next_token();
        break;
    case TokenKind::Identifier:
        lhs = parse_ident_unchecked();
        next_token();
        break;
    case TokenKind::String: {
        lhs = std::make_unique<ast::StringExpr>(tok.get_span());
        next_token();
        break;
    }
    case TokenKind::Char:
        lhs = std::make_unique<ast::CharExpr>(tok.get_span());
        next_token();
        break;
    case TokenKind::PlusPlus:
    case TokenKind::MinusMinus:
    case TokenKind::LogicalNot:
    case TokenKind::Not:
    case TokenKind::Minus: {
        auto prefix_tok = tok;
        auto expr = prime_parse_expr(
            static_cast<int>(ast::BinOpPrecedence::Prefix), ignore);
        if (!expr.is_valid())
            return ExprError();

        lhs = std::make_unique<ast::PrefixExpr>(prefix_tok, expr.take());
        break;
    }
    case TokenKind::LParen: {
        auto expr = prime_parse_expr(0, ignore);
        if (!expr.is_valid())
            return ExprError();

        lhs = expr.take();
        if (tok.is(TokenKind::Comma)) {
            auto second = prime_parse_expr(0, ignore);
            if (!second.is_valid())
                return ExprError();

            span += tok.get_span();

            lhs = std::make_unique<ast::TupleExpr>(span, std::move(lhs),
                                                   second.take());
        }

        if (!tok_assert(TokenKind::RParen))
            return ExprError();

        next_token();
        break;
    }
    case TokenKind::LBracket: {
        std::vector<std::unique_ptr<ast::Expr>> vals;

        while (!kind(TokenKind::RBracket)) {
            auto expr = parse_expr();
            if (!expr.is_valid())
                return ExprError();

            vals.push_back(expr.take());

            if (!tok.is(TokenKind::Comma)) {
                if (!tok_assert(TokenKind::RBracket))
                    return ExprError();
                break;
            }
        }

        span += tok.get_span();
        next_token();
        lhs = std::make_unique<ast::ArrayInitExpr>(span, std::move(vals));
        break;
    }
    case TokenKind::LBrace: {
        auto block = parse_block();
        if (!block.is_valid())
            return ExprError();

        span += prev_tok.get_span();
        lhs = block.take();
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
        if (ignore && ignore.value() == tok.get_kind())
            break;

        switch (tok.get_kind()) {
        case TokenKind::PlusPlus:
        case TokenKind::MinusMinus:
            lhs = std::make_unique<ast::PostfixExpr>(tok, std::move(lhs));
            next_token();
            break;
        case TokenKind::Question: {
            auto operator_tok = tok;

            auto then_expr = prime_parse_expr();
            if (!then_expr.is_valid())
                return ExprError();

            if (!tok_assert(TokenKind::Colon))
                return ExprError();
            auto colon_tok = tok;

            auto else_expr = prime_parse_expr(
                static_cast<int>(ast::BinOpPrecedence::Prefix) - 1);
            if (!else_expr.is_valid())
                return ExprError();

            lhs = std::make_unique<ast::TernaryExpr>(
                operator_tok, colon_tok, std::move(lhs), then_expr.take(),
                else_expr.take());
            break;
        }
        case TokenKind::LParen: {
            std::vector<std::unique_ptr<ast::Expr>> args;

            while (!kind(TokenKind::RParen)) {
                auto expr = parse_expr();
                if (!expr.is_valid())
                    return ExprError();

                args.push_back(expr.take());

                if (!tok.is(TokenKind::Comma)) {
                    if (!tok_assert(TokenKind::RParen))
                        return ExprError();
                    break;
                }
            }

            span += tok.get_span();
            next_token();
            lhs = std::make_unique<ast::CallExpr>(span, std::move(lhs),
                                                  std::move(args));
            break;
        }
        case TokenKind::LBracket: {
            auto expr = prime_parse_expr(0, ignore);
            if (!expr.is_valid())
                return ExprError();

            span += tok.get_span();
            lhs = std::make_unique<ast::ArrayExpr>(span, std::move(lhs),
                                                   expr.take());
            if (!tok_assert(TokenKind::RBracket))
                return ExprError();
            next_token();
            break;
        }
        case TokenKind::LBrace: {
            std::vector<std::unique_ptr<ast::StructExprField>> vals;

            while (!kind(TokenKind::RBrace)) {
                auto field = parse_struct_expr_field();
                if (!field.is_valid())
                    return ExprError();

                vals.push_back(field.take());

                if (!tok.is(TokenKind::Comma)) {
                    if (!tok_assert(TokenKind::RBrace))
                        return ExprError();
                    break;
                }
            }

            span = tok.get_span();
            next_token();
            lhs = std::make_unique<ast::StructInitExpr>(span, std::move(lhs),
                                                        std::move(vals));
            break;
        }
        case TokenKind::Dot:
            if (!consume(TokenKind::Identifier))
                return ExprError();

            lhs = std::make_unique<ast::FieldExpr>(std::move(lhs),
                                                   parse_ident_unchecked());
            next_token();
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
        case TokenKind::Eq: {
            auto operator_tok = tok;
            auto expr = prime_parse_expr(
                static_cast<int>(get_op_precedence(tok.get_kind())) - 1);
            if (!expr.is_valid())
                return ExprError();

            lhs = std::make_unique<ast::BinaryExpr>(
                operator_tok, std::move(lhs), expr.take());
            break;
        }
        default: {
            auto prec = static_cast<int>(get_op_precedence(tok.get_kind()));
            if (prec == 0)
                throw std::runtime_error("invalid operator");

            auto operator_tok = tok;
            auto expr = prime_parse_expr(prec, ignore);
            if (!expr.is_valid())
                return ExprError();

            lhs = std::make_unique<ast::BinaryExpr>(
                operator_tok, std::move(lhs), expr.take());
            break;
        }
        }
    }
    return Result(std::move(lhs));
}

Result<std::unique_ptr<ast::StructExprField>>
Parser::parse_struct_expr_field() {
    if (!tok_assert(TokenKind::Identifier))
        return Result<std::unique_ptr<ast::StructExprField>>();

    auto ident = parse_ident_unchecked();

    if (!consume(TokenKind::Colon))
        return Result<std::unique_ptr<ast::StructExprField>>();

    auto expr = prime_parse_expr();
    if (!expr.is_valid())
        return Result<std::unique_ptr<ast::StructExprField>>();

    return Result<std::unique_ptr<ast::StructExprField>>(
        std::make_unique<ast::StructExprField>(std::move(ident), expr.take()));
}

std::unique_ptr<ast::Identifier> Parser::parse_ident_unchecked() {
    return std::make_unique<ast::Identifier>(
        tok, source->get_string(tok.get_span()));
}

std::unique_ptr<ast::Expr> Parser::parse_num() const {
    const auto str = source->get_string(tok.get_span());
    std::string val;

    for (const auto& c : str) {
        if (c != '_')
            val += c;
    }

    for (size_t i = 0; i < val.length(); i++) {
        if (val[i] == '.') {
            double num = NAN;
            if (std::from_chars(val.begin().base(), val.end().base(), num).ec ==
                std::errc::result_out_of_range) {
                diag.emit(tok.get_span(), DiagnosticKind::FloatOutOfRange);
                auto res = std::make_unique<ast::FloatExpr>(tok, num);
                res->mark_invalid();
                return res;
            }

            return std::make_unique<ast::FloatExpr>(tok, num);
        }
    }

    auto base = 10;
    if (val.length() >= 3 && val.front() == '0') {
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
        if (std::from_chars(&val[2], val.end().base(), num, base).ec ==
            std::errc::result_out_of_range) {
            diag.emit(tok.get_span(), DiagnosticKind::IntegerOutOfRange);
            auto res = std::make_unique<ast::IntExpr>(tok, num);
            res->mark_invalid();
            return res;
        }
    } else {
        if (std::from_chars(val.begin().base(), val.end().base(), num).ec ==
            std::errc::result_out_of_range) {
            diag.emit(tok.get_span(), DiagnosticKind::IntegerOutOfRange);
            auto res = std::make_unique<ast::IntExpr>(tok, num);
            res->mark_invalid();
            return res;
        }
    }

    return std::make_unique<ast::IntExpr>(tok, num);
}

ExprResult Parser::parse_for_expr() {
    tok_assert(TokenKind::KwFor);
    Span span = tok.get_span();

    if (!consume(TokenKind::Identifier))
        return ExprError();

    auto ident = parse_ident_unchecked();

    if (!consume(TokenKind::KwIn))
        return ExprError();

    auto expr = prime_parse_expr(0, TokenKind::LBrace);
    if (!expr.is_valid())
        return ExprError();

    auto block = parse_block(false);
    if (!block.is_valid())
        return ExprError();

    span += block.get()->span;
    return ExprResult(std::make_unique<ast::ForExpr>(
        span, std::move(ident), expr.take(), block.take()));
}

ExprResult Parser::parse_if_expr() {
    tok_assert(TokenKind::KwIf);
    Span span = tok.get_span();

    auto expr = prime_parse_expr(0, TokenKind::LBrace);
    if (!expr.is_valid())
        return ExprError();

    auto block = parse_block();
    if (!block.is_valid())
        return ExprError();

    if (tok.is(TokenKind::KwElse)) {
        std::unique_ptr<ast::ElseExpr> else_expr;
        Span else_span = tok.get_span();

        if (kind(TokenKind::KwIf)) {
            auto if_expr = parse_if_expr();
            if (!if_expr.is_valid())
                return ExprError();

            else_span += if_expr.get()->span;
            else_expr =
                std::make_unique<ast::ElseExpr>(else_span, if_expr.take());
        } else {
            auto else_block = parse_block();
            if (!else_block.is_valid())
                return ExprError();

            else_span += else_block.get()->span;
            else_expr =
                std::make_unique<ast::ElseExpr>(else_span, else_block.take());
        }

        span += else_span;
        return ExprResult(std::make_unique<ast::IfExpr>(
            span, expr.take(), block.take(), std::move(else_expr)));
    }

    span += block.get()->span;
    return ExprResult(
        std::make_unique<ast::IfExpr>(span, expr.take(), block.take()));
}

ExprResult Parser::parse_loop_expr() {
    tok_assert(TokenKind::KwLoop);
    Span span = tok.get_span();

    ExprResult expr;

    if (!kind(TokenKind::LBrace)) {
        expr = parse_expr(0, TokenKind::LBrace);
        if (!expr.is_valid())
            return ExprError();
    }

    auto block = parse_block(false);
    if (!block.is_valid())
        return ExprError();

    span += block.get()->span;
    if (expr.is_valid()) {
        return ExprResult(
            std::make_unique<ast::LoopExpr>(span, expr.take(), block.take()));
    }

    return ExprResult(
        std::make_unique<ast::LoopExpr>(span, nullptr, block.take()));
}

ExprResult Parser::parse_while_expr() {
    tok_assert(TokenKind::KwWhile);
    Span span = tok.get_span();

    auto expr = prime_parse_expr(0, TokenKind::LBrace);
    if (!expr.is_valid())
        return ExprError();

    auto block = parse_block(false);
    if (!block.is_valid())
        return ExprError();

    span += block.get()->span;
    return ExprResult(
        std::make_unique<ast::WhileExpr>(span, expr.take(), block.take()));
}

TypeResult Parser::prime_parse_type() {
    next_token();
    return parse_type();
}

TypeResult Parser::parse_type() {
    std::unique_ptr<type::Type> type;
    if (tok.is(TokenKind::Identifier)) {

        if (const auto val_str = source->get_string(tok.get_span());
            val_str.at(0) == 'u') {
            if (val_str == "u8") {
                type = std::make_unique<type::IntegerType>(8, false);
            } else if (val_str == "u16") {
                type = std::make_unique<type::IntegerType>(16, false);
            } else if (val_str == "u32") {
                type = std::make_unique<type::IntegerType>(32, false);
            } else if (val_str == "u64") {
                type = std::make_unique<type::IntegerType>(64, false);
            }
        } else if (val_str.at(0) == 'i') {
            if (val_str == "i8") {
                type = std::make_unique<type::IntegerType>(8, true);
            } else if (val_str == "i16") {
                type = std::make_unique<type::IntegerType>(16, true);
            } else if (val_str == "i32") {
                type = std::make_unique<type::IntegerType>(32, true);
            } else if (val_str == "i64") {
                type = std::make_unique<type::IntegerType>(64, true);
            }
        } else if (val_str.at(0) == 'f') {
            if (val_str == "f32") {
                type = std::make_unique<type::FloatType>(32);
            } else if (val_str == "f64") {
                type = std::make_unique<type::FloatType>(64);
            }
        } else if (val_str == "bool") {
            type = std::make_unique<type::BooleanType>();
        } else if (val_str == "str") {
            type = std::make_unique<type::StringType>();
        } else if (val_str == "char") {
            type = std::make_unique<type::CharType>();
        } else {
            type = std::make_unique<type::UnknownType>(parse_ident_unchecked());
        }

        while (kind(TokenKind::Star)) {
            type = std::make_unique<type::PointerType>(std::move(type));
        }
    } else if (tok.is(TokenKind::LBracket)) {
        auto array_type = prime_parse_type();
        if (!array_type.is_valid())
            return TypeError();

        if (tok.is(TokenKind::Semi)) {
            auto size = prime_parse_expr();
            if (!size.is_valid())
                return TypeError();

            type = std::make_unique<type::ArrayType>(array_type.take(),
                                                     size.take());
        } else {
            type = std::make_unique<type::ArrayType>(array_type.take());
        }
        tok_assert(TokenKind::RBracket);
        next_token();
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
    return tok_assert(kind);
}

bool Parser::kind(const TokenKind kind) {
    next_token();
    return tok.is(kind);
}

bool Parser::tok_assert(const TokenKind kind) {
    if (!tok.is(kind)) {
        if (kind == TokenKind::Semi) {
            diag.emit(
                Span(prev_tok.get_span().index + prev_tok.get_span().len, 1),
                DiagnosticKind::ExpectedSemi);
        } else {
            diag.emit(tok.get_span(), DiagnosticKind::ExpectedToken,
                      tok_kind_to_string(kind),
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
} // namespace z
