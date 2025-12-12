#include "src_mgr.h"
#include "test_helpers.h"
#include "token.h"
#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

namespace z {
void PrintTo(const TokenKind& kind, std::ostream* os) {
    *os << tok_kind_to_string(kind);
}
} // namespace z

namespace z::test {
class LexerTest : public testing::Test {
protected:
    static z::SourceManager create_source(const std::string& src) {
        return z::SourceManager::Create(src);
    }
};

TEST_F(LexerTest, LexKeywords) {
    auto sm = create_source("as const for fn let return while");
    auto tokens = tokenize(sm);

    std::array expected{TokenKind::KwAs,   TokenKind::KwConst,
                        TokenKind::KwFor,  TokenKind::KwFn,
                        TokenKind::KwLet,  TokenKind::KwReturn,
                        TokenKind::KwWhile};

    ASSERT_EQ(tokens.size(), expected.size());

    for (size_t i = 0; i < tokens.size(); i++) {
        EXPECT_EQ(tokens[i].get_kind(), expected[i]);
    }
}

TEST_F(LexerTest, LexSingleCharacterTokens) {
    auto sm = create_source("( { ; , : + = )");
    auto tokens = tokenize(sm);

    std::array expected{TokenKind::LParen, TokenKind::LBrace, TokenKind::Semi,
                        TokenKind::Comma,  TokenKind::Colon,  TokenKind::Plus,
                        TokenKind::Eq,     TokenKind::RParen};

    ASSERT_EQ(tokens.size(), expected.size());

    for (size_t i = 0; i < tokens.size(); i++) {
        EXPECT_EQ(tokens[i].get_kind(), expected[i]);
    }
}

TEST_F(LexerTest, LexDoubleCharacterTokens) {
    auto sm = create_source("++ == <= << /=");
    auto tokens = tokenize(sm);

    std::array expected{TokenKind::PlusPlus, TokenKind::EqEq, TokenKind::Le,
                        TokenKind::Shl, TokenKind::SlashEq};

    ASSERT_EQ(tokens.size(), expected.size());

    for (size_t i = 0; i < tokens.size(); i++) {
        EXPECT_EQ(tokens[i].get_kind(), expected[i]);
    }
}

TEST_F(LexerTest, LexTripleCharacterTokens) {
    auto sm = create_source("<<= >>=");
    auto tokens = tokenize(sm);

    std::array expected{TokenKind::ShlEq, TokenKind::ShrEq};

    ASSERT_EQ(tokens.size(), expected.size());

    for (size_t i = 0; i < tokens.size(); i++) {
        EXPECT_EQ(tokens[i].get_kind(), expected[i]);
    }
}

TEST_F(LexerTest, LexStringLiteral) {
    auto sm = create_source(R"("asdf" "asdfasdf" "test")");
    auto tokens = tokenize(sm);

    ASSERT_EQ(tokens.size(), 3);

    for (auto token : tokens) {
        EXPECT_EQ(token.get_kind(), TokenKind::String);
    }

    EXPECT_EQ(sm.get_string(tokens[0].get_span()), "asdf");
    EXPECT_EQ(sm.get_string(tokens[1].get_span()), "asdfasdf");
    EXPECT_EQ(sm.get_string(tokens[2].get_span()), "test");
}

TEST_F(LexerTest, LexCharLiteral) {
    auto sm = create_source("'a' 'b' 'z'");
    auto tokens = tokenize(sm);

    ASSERT_EQ(tokens.size(), 3);

    for (auto token : tokens) {
        EXPECT_EQ(token.get_kind(), TokenKind::Char);
    }
}

TEST_F(LexerTest, LexIdentifier) {
    auto sm = create_source("test ident asdfasdf");
    auto tokens = tokenize(sm);

    ASSERT_EQ(tokens.size(), 3);

    for (auto token : tokens) {
        EXPECT_EQ(token.get_kind(), TokenKind::Identifier);
    }
}

TEST_F(LexerTest, LexNumbers) {
    auto sm = create_source("0x12 413 3.43 0b01110 0o12 231_312");
    auto tokens = tokenize(sm);

    ASSERT_EQ(tokens.size(), 6);

    for (auto token : tokens) {
        EXPECT_EQ(token.get_kind(), TokenKind::Number);
    }
}

TEST_F(LexerTest, IgnoreComments) {
    auto sm = create_source("asdf /* comment */ asdf // comment");
    auto tokens = tokenize(sm);

    ASSERT_EQ(tokens.size(), 2);

    EXPECT_EQ(tokens[0].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Identifier);
}

TEST_F(LexerTest, IgnoreWhitespace) {
    auto sm = create_source("1 + \n1 =     2");
    auto tokens = tokenize(sm);

    std::array expected{TokenKind::Number, TokenKind::Plus, TokenKind::Number,
                        TokenKind::Eq, TokenKind::Number};

    ASSERT_EQ(tokens.size(), expected.size());

    for (size_t i = 0; i < tokens.size(); i++) {
        EXPECT_EQ(tokens[i].get_kind(), expected[i]);
    }
}
} // namespace z::test
