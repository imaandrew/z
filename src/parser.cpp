#include "parser.h"
#include "token.h"
#include "ast.h"
#include "type.h"
#include <charconv>
#include <exception>
#include <optional>
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
        case TokenKind::LBrace:
        case TokenKind::PlusPlus:
        case TokenKind::MinusMinus:
        case TokenKind::Dot:
            return BinOpPrecedence::Postfix;
        default:
            return BinOpPrecedence::Unknown;
    }
}

std::vector<std::unique_ptr<Decl>> Parser::parse() {
    next_token();

    std::vector<std::unique_ptr<Decl>> decls;

    while (!lexer.at_end()) {
        switch (tok.get_kind()) {
            case TokenKind::KwStruct:
                decls.push_back(std::make_unique<StructDecl>(parse_struct_decl()));
                break;
            case TokenKind::KwEnum:
                decls.push_back(std::make_unique<EnumDecl>(parse_enum_decl()));
                break;
            case TokenKind::KwConst:
                decls.push_back(std::make_unique<ConstDecl>(parse_const_decl()));
                break;
            case TokenKind::KwStatic:
                decls.push_back(std::make_unique<StaticDecl>(parse_static_decl()));
                break;
            case TokenKind::KwFn:
                decls.push_back(std::make_unique<FuncDecl>(parse_func_decl()));
                break;
            default:
                throw std::runtime_error(std::format("Invalid declaration {}", tok_kind_to_string(tok.get_kind())));
        }
    }

    return decls;
}

StructDecl Parser::parse_struct_decl() {
    assert(TokenKind::KwStruct);

    consume(TokenKind::Identifier);
    auto ident = Identifier(tok);

    consume(TokenKind::LBrace);

    std::vector<StructField> fields;
    while (!kind(TokenKind::RBrace)) {
        fields.push_back(parse_struct_field());

        if (!tok.is(TokenKind::Comma)) {
            assert(TokenKind::RBrace);
            break;
        }
    }

    next_token();

    return StructDecl(ident, std::move(fields));
}

StructField Parser::parse_struct_field() {
    assert(TokenKind::Identifier);
    auto ident = Identifier(tok);

    consume(TokenKind::Colon);

    auto type = prime_parse_type();

    return StructField(ident, type);
}

EnumDecl Parser::parse_enum_decl() {
    assert(TokenKind::KwEnum);

    consume(TokenKind::Identifier);
    auto ident = Identifier(tok);

    consume(TokenKind::LBrace);

    std::vector<EnumField> fields;
    while (!kind(TokenKind::RBrace)) {
        fields.push_back(parse_enum_field());

        if (!tok.is(TokenKind::Comma)) {
            assert(TokenKind::RBrace);
            break;
        }
    }

    next_token();

    return EnumDecl(ident, std::move(fields));
}

EnumField Parser::parse_enum_field() {
    assert(TokenKind::Identifier);
    auto ident = Identifier(tok);

    if (kind(TokenKind::LParen)) {
        std::vector<Type> types;
        while (!kind(TokenKind::RParen)) {
            types.push_back(parse_type());

            if (!tok.is(TokenKind::Comma)) {
                assert(TokenKind::RParen);
                next_token();
                break;
            }
        }

        return EnumField(ident, types);
    } else {
        return EnumField(ident);
    }
}

ConstDecl Parser::parse_const_decl() {
    assert(TokenKind::KwConst);

    consume(TokenKind::Identifier);
    auto ident = Identifier(tok);

    consume(TokenKind::Colon);
    auto type = prime_parse_type();

    assert(TokenKind::Eq);
    auto expr = prime_parse_expr();

    assert(TokenKind::Semi);
    next_token();

    return ConstDecl(ident, type, std::move(expr));
}

StaticDecl Parser::parse_static_decl() {
    assert(TokenKind::KwStatic);

    consume(TokenKind::Identifier);
    auto ident = Identifier(tok);

    consume(TokenKind::Colon);
    auto type = prime_parse_type();

    assert(TokenKind::Eq);
    auto expr = prime_parse_expr();

    assert(TokenKind::Semi);
    next_token();

    return StaticDecl(ident, type, std::move(expr));
}

FuncDecl Parser::parse_func_decl() {
    assert(TokenKind::KwFn);

    consume(TokenKind::Identifier);
    auto func_ident = Identifier(tok);
    
    std::optional<Identifier> impl_type;
    if (kind(TokenKind::ColonColon)) {
        consume(TokenKind::Identifier);

        impl_type.emplace(Identifier(tok));
        next_token();
    }

    auto params = parse_func_params();

    std::optional<Type> ret; 
    if (tok.is(TokenKind::Arrow)) {
        ret.emplace(prime_parse_type());
    }

    auto block = parse_block();
    return FuncDecl(func_ident, impl_type, params, ret, std::move(block));
}

std::vector<Param> Parser::parse_func_params() {
    assert(TokenKind::LParen);
    
    std::vector<Param> params;
    while (!kind(TokenKind::RParen)) {
        params.push_back(parse_param_decl());
        if (!tok.is(TokenKind::Comma)) {
            assert(TokenKind::RParen);
            break;
        }
    }

    next_token();
    return params;
}

Param Parser::parse_param_decl() {
    assert(TokenKind::Identifier);
    auto name = Identifier(tok);

    consume(TokenKind::Colon);

    auto type = prime_parse_type();

    return Param(name, type);
}

Block Parser::parse_block(bool implicit_return) {
    assert(TokenKind::LBrace);
    next_token();

    std::vector<std::unique_ptr<Stmt>> stmts;
    while (!tok.is(TokenKind::RBrace)) {
        if (!tok.is(TokenKind::Semi))
            stmts.push_back(parse_stmt());

        if (implicit_return && tok.is(TokenKind::RBrace))
            break;

        if (required_semi) {
            assert(TokenKind::Semi);
            next_token();
        } else {
            required_semi = true;
        }
    }

    try {
        next_token();   
    } catch (std::runtime_error e) {};
    return Block(std::move(stmts));
}

std::unique_ptr<Stmt> Parser::parse_stmt() {
    std::unique_ptr<Stmt> stmt;
    if (tok.is(TokenKind::KwBreak)) {
        try {
            auto expr = prime_parse_expr();
            stmt = std::make_unique<BreakStmt>(tok, std::move(expr));
        } catch (std::exception) {
            stmt = std::make_unique<BreakStmt>(tok);
        }
    } else if (tok.is(TokenKind::KwContinue)) {
        stmt = std::make_unique<ContinueStmt>(tok);
        next_token();
    } else if (tok.is(TokenKind::KwLet)) {
        consume(TokenKind::Identifier);
        auto ident = Identifier(tok);
        if (kind(TokenKind::Colon)) {
            auto type = prime_parse_type();
            assert(TokenKind::Eq);
            stmt = std::make_unique<LetStmt>(ident, type, prime_parse_expr());
        } else if (tok.is(TokenKind::Eq)) {
            stmt = std::make_unique<LetStmt>(ident, prime_parse_expr());
        } else {
            stmt = std::make_unique<LetStmt>(ident);
        }
    } else if (tok.is(TokenKind::KwReturn)) {
        try {
            auto expr = prime_parse_expr();
            stmt = std::make_unique<ReturnStmt>(std::move(expr));
        } catch (std::exception) {
            stmt = std::make_unique<ReturnStmt>();
        }
    } else {
        auto x = tok.is(TokenKind::KwIf) || tok.is(TokenKind::KwFor) || tok.is(TokenKind::KwLoop) || tok.is(TokenKind::KwWhile);
        stmt = parse_expr();
        required_semi = !x;
    }
    
    return stmt;
}

std::unique_ptr<Expr> Parser::prime_parse_expr(int precedence) {
    next_token();
    return parse_expr(precedence);
}

std::unique_ptr<Expr> Parser::parse_expr(int precedence) {
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
            auto t = tok;
            next_token();
            return std::make_unique<StringExpr>(t.get_val(), t.get_len());
        }
        case TokenKind::PlusPlus:
        case TokenKind::MinusMinus:
        case TokenKind::LogicalNot:
        case TokenKind::Not:
        case TokenKind::Minus: {
            auto t = tok;
            lhs = std::make_unique<PrefixExpr>(t, prime_parse_expr(static_cast<int>(BinOpPrecedence::Prefix)));
            break;
        }
        case TokenKind::LParen: {
            lhs = prime_parse_expr();
            assert(TokenKind::RParen);
            next_token();
            break;
        }
        case TokenKind::LBrace: {
            std::vector<std::unique_ptr<Expr>> vals;

            while (!kind(TokenKind::RBrace)) {
                vals.push_back(parse_expr());

                if (!tok.is(TokenKind::Comma)) {
                    assert(TokenKind::RBrace);
                    break;
                }
            }

            next_token();
            lhs.reset(new ArrayInitExpr(std::move(vals)));
            break;
        }
        case TokenKind::KwFor:
            return std::make_unique<ForExpr>(parse_for_expr());
        case TokenKind::KwIf:
            return std::make_unique<IfExpr>(parse_if_expr());
        case TokenKind::KwLoop:
            return std::make_unique<LoopExpr>(parse_loop_expr());
        case TokenKind::KwWhile:
            return std::make_unique<WhileExpr>(parse_while_expr());
        default:
            throw std::runtime_error(std::format("invalid expr {}", tok_kind_to_string(tok.get_kind())));
    }

    while (precedence < static_cast<int>(get_op_precedence(tok.get_kind()))) {
        switch (tok.get_kind()) {
            case TokenKind::PlusPlus:
            case TokenKind::MinusMinus:
                lhs.reset(new PostfixExpr(tok, std::move(lhs)));
                next_token();
                break;
            case TokenKind::Question: {
                auto t = tok;
                auto then_expr = prime_parse_expr();
                consume(TokenKind::Colon);
                auto else_expr = parse_expr(static_cast<int>(BinOpPrecedence::Prefix) - 1);
                lhs.reset(new TernaryExpr(t, std::move(lhs), std::move(then_expr), std::move(else_expr)));
                break;
            }
            case TokenKind::LParen: {
                std::vector<std::unique_ptr<Expr>> args;

                while (!kind(TokenKind::RParen)) {
                    args.push_back(parse_expr());

                    if (!tok.is(TokenKind::Comma)) {
                        assert(TokenKind::RParen);
                        break;
                    }
                }

                next_token();
                lhs.reset(new CallExpr(std::move(lhs), std::move(args)));
                break;
            }
            case TokenKind::LBracket:
                if (kind(TokenKind::RBracket)) {
                    lhs.reset(new ArrayExpr(std::move(lhs)));
                } else {
                    lhs.reset(new ArrayExpr(std::move(lhs), parse_expr()));
                    assert(TokenKind::RBracket);
                }
                next_token();
                break;
            case TokenKind::LBrace: {
                std::vector<std::unique_ptr<Expr>> vals;

                while (!kind(TokenKind::RBrace)) {
                    vals.push_back(parse_expr());

                    if (!tok.is(TokenKind::Comma)) {
                        assert(TokenKind::RBrace);
                        break;
                    }
                }

                next_token();
                lhs.reset(new StructInitExpr(std::move(lhs), std::move(vals)));
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
                auto t = tok;
                lhs.reset(new BinaryExpr(t, std::move(lhs), prime_parse_expr(static_cast<int>(get_op_precedence(tok.get_kind())) - 1)));
                break;
            }
            default:
                auto prec = static_cast<int>(get_op_precedence(tok.get_kind()));
                if (prec == 0)
                    throw std::runtime_error("invalid operator");

                auto t = tok;
                lhs.reset(new BinaryExpr(t, std::move(lhs), prime_parse_expr(prec)));
                break;
        }
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parse_num() const {
    auto val = tok.get_val();
    auto len = tok.get_len();

    for (int i = 0; i < len; i++) {
        if (val[i] == '.') {
            double num;
            std::from_chars(val, val + len, num);
            return std::make_unique<FloatExpr>(tok, num);
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

    return std::make_unique<IntExpr>(tok, num);
}

ForExpr Parser::parse_for_expr() {
    assert(TokenKind::KwFor);

    consume(TokenKind::Identifier);
    auto ident = Identifier(tok);

    consume(TokenKind::KwIn);

    auto expr = prime_parse_expr();
    auto block = parse_block(false);

    return ForExpr(ident, std::move(expr), std::move(block));
}

IfExpr Parser::parse_if_expr() {
    assert(TokenKind::KwIf);

    auto expr = prime_parse_expr();
    auto block = parse_block();

    if (tok.is(TokenKind::KwElse)) {
        std::unique_ptr<ElseExpr> else_expr;
        if (kind(TokenKind::KwIf)) {
            auto if_expr = parse_if_expr();
            else_expr = std::make_unique<ElseExpr>(std::make_unique<IfExpr>(std::move(if_expr)));
        } else {
            auto block = parse_block();
            else_expr = std::make_unique<ElseExpr>(std::move(block));
        }

        return IfExpr(std::move(expr), std::move(block), std::move(else_expr));
    }
    return IfExpr(std::move(expr), std::move(block));
}

LoopExpr Parser::parse_loop_expr() {
    assert(TokenKind::KwLoop);

    std::unique_ptr<Expr> expr;
    
    if (!kind(TokenKind::LBrace)) {
        expr = parse_expr();
    }

    auto block = parse_block(false);
    return LoopExpr(std::move(expr), std::move(block));
}

WhileExpr Parser::parse_while_expr() {
    assert(TokenKind::KwWhile);

    auto expr = prime_parse_expr();
    auto block = parse_block(false);
    return WhileExpr(std::move(expr), std::move(block));
}

Type Parser::prime_parse_type() {
    next_token();
    return parse_type();
}

Type Parser::parse_type() {
    Type t;
    if (tok.is(TokenKind::Identifier)) {
        auto val_str = std::string(tok.get_val(), tok.get_len());
        auto v = tok.get_val();

        if (v[0] == 'u') {
            if (val_str == "u8") {
                t = IntegerType(1, false);
            } else if (val_str == "u16") {
                t = IntegerType(2, false);
            } else if (val_str == "u32") {
                t = IntegerType(3, false);
            } else if (val_str == "u64") {
                t = IntegerType(4, false);
            }
        } else if (v[0] == 'i') {
            if (val_str == "i8") {
                t = IntegerType(1, true);
            } else if (val_str == "i16") {
                t = IntegerType(2, true);
            } else if (val_str == "i32") {
                t = IntegerType(3, true);
            } else if (val_str == "i64") {
                t = IntegerType(4, true);
            }
        } else if (v[0] == 'f') {
            if (val_str == "f32") {
                t = FloatType(3);
            } else if (val_str == "f64") {
                t = FloatType(4);
            }
        } else if (val_str == "bool") {
            t = BooleanType();
        } else if (val_str == "str") {
            t = StringType();
        } else if (val_str == "char") {
            t = CharType();
        } else {
            t = UserDefinedType(std::make_unique<Identifier>(tok));
        }

        while (kind(TokenKind::Star)) {
            t = PointerType(t);
        }
    } else if (tok.is(TokenKind::LBracket)) {
        auto array_type = prime_parse_type();

        if (tok.is(TokenKind::Semi)) {
            t = ArrayType(array_type, prime_parse_expr());
        } else {
            t = ArrayType(array_type);
        }
        assert(TokenKind::RBracket);
        next_token();
    } else if (tok.is(TokenKind::LParen)) {
        std::vector<Type> types;

        while (!kind(TokenKind::RParen)) {
            types.push_back(parse_type());

            if (!tok.is(TokenKind::Comma)) {
                assert(TokenKind::RParen);
                next_token();
                break;
            }
        }
    }

    return t;
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

void Parser::assert(TokenKind kind) const {
    if (!tok.is(kind))
        throw std::runtime_error(std::format("unexpected token {}, wanted {}", tok_kind_to_string(tok.get_kind()), tok_kind_to_string(kind)));
}