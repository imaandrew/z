#include "diag/src_mgr.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using namespace z;

class LexerTest : public testing::Test {
protected:
    struct LexerWithSource {
        std::unique_ptr<SourceManager> source;
        Lexer lexer;

        explicit LexerWithSource(const std::string& input)
            : source(SourceManager::Create(input)), lexer(source.get()) {}

        std::vector<Token> lex() {
            std::vector<Token> tokens;

            while (true) {
                auto tok = lexer.lex_token();
                tokens.push_back(tok);
                if (tok.get_kind() == TokenKind::Eof) {
                    break;
                }
            }

            return tokens;
        }
    };

    static std::vector<Token> lex_string(const std::string& input) {
        auto l = LexerWithSource(input);
        return l.lex();
    }
};

TEST_F(LexerTest, LexesKeywords) {
    auto tokens = lex_string("as in while const return while");

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::KwAs);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::KwIn);
    EXPECT_EQ(tokens[2].get_kind(), TokenKind::KwWhile);
    EXPECT_EQ(tokens[3].get_kind(), TokenKind::KwConst);
    EXPECT_EQ(tokens[4].get_kind(), TokenKind::KwReturn);
    EXPECT_EQ(tokens[5].get_kind(), TokenKind::KwWhile);
    EXPECT_EQ(tokens[6].get_kind(), TokenKind::Eof);
}

TEST_F(LexerTest, LexesSingleCharOperators) {
    auto tokens = lex_string("+ = < - * [ / *");

    ASSERT_EQ(tokens.size(), 9);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::Plus);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Eq);
    EXPECT_EQ(tokens[2].get_kind(), TokenKind::Lt);
    EXPECT_EQ(tokens[3].get_kind(), TokenKind::Minus);
    EXPECT_EQ(tokens[4].get_kind(), TokenKind::Star);
    EXPECT_EQ(tokens[5].get_kind(), TokenKind::LBracket);
    EXPECT_EQ(tokens[6].get_kind(), TokenKind::Slash);
    EXPECT_EQ(tokens[7].get_kind(), TokenKind::Star);
    EXPECT_EQ(tokens[8].get_kind(), TokenKind::Eof);
}

TEST_F(LexerTest, LexesMultiCharOperators) {
    auto tokens = lex_string("++ += >= :: ..= || <<=");

    ASSERT_EQ(tokens.size(), 8);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::PlusPlus);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::PlusEq);
    EXPECT_EQ(tokens[2].get_kind(), TokenKind::Ge);
    EXPECT_EQ(tokens[3].get_kind(), TokenKind::ColonColon);
    EXPECT_EQ(tokens[4].get_kind(), TokenKind::RangeEq);
    EXPECT_EQ(tokens[5].get_kind(), TokenKind::Or);
    EXPECT_EQ(tokens[6].get_kind(), TokenKind::ShlEq);
    EXPECT_EQ(tokens[7].get_kind(), TokenKind::Eof);
}

TEST_F(LexerTest, LexesMixedOperators) {
    auto tokens = lex_string("-> % %= , ~ && >>= ^");

    ASSERT_EQ(tokens.size(), 9);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::Arrow);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Percent);
    EXPECT_EQ(tokens[2].get_kind(), TokenKind::PercentEq);
    EXPECT_EQ(tokens[3].get_kind(), TokenKind::Comma);
    EXPECT_EQ(tokens[4].get_kind(), TokenKind::Not);
    EXPECT_EQ(tokens[5].get_kind(), TokenKind::AndAnd);
    EXPECT_EQ(tokens[6].get_kind(), TokenKind::ShrEq);
    EXPECT_EQ(tokens[7].get_kind(), TokenKind::Caret);
    EXPECT_EQ(tokens[8].get_kind(), TokenKind::Eof);
}

TEST_F(LexerTest, DoesNotLexMultiCharOpAcrossWhitespace) {
    auto tokens = lex_string("+ + = % = > >");

    ASSERT_EQ(tokens.size(), 8);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::Plus);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Plus);
    EXPECT_EQ(tokens[2].get_kind(), TokenKind::Eq);
    EXPECT_EQ(tokens[3].get_kind(), TokenKind::Percent);
    EXPECT_EQ(tokens[4].get_kind(), TokenKind::Eq);
    EXPECT_EQ(tokens[5].get_kind(), TokenKind::Gt);
    EXPECT_EQ(tokens[6].get_kind(), TokenKind::Gt);
    EXPECT_EQ(tokens[7].get_kind(), TokenKind::Eof);
}

TEST_F(LexerTest, LexesIdentifiers) {
    auto l = LexerWithSource("test ident z003a _b8__938");
    auto tokens = l.lex();

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[2].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[3].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[4].get_kind(), TokenKind::Eof);

    EXPECT_EQ(l.source->get_string(tokens[0].get_span()), "test");
    EXPECT_EQ(l.source->get_string(tokens[1].get_span()), "ident");
    EXPECT_EQ(l.source->get_string(tokens[2].get_span()), "z003a");
    EXPECT_EQ(l.source->get_string(tokens[3].get_span()), "_b8__938");
}

TEST_F(LexerTest, LexesNumbers) {
    auto l = LexerWithSource("31 0x04423 0b11001 0.344 0O314");
    auto tokens = l.lex();

    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::Number);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Number);
    EXPECT_EQ(tokens[2].get_kind(), TokenKind::Number);
    EXPECT_EQ(tokens[3].get_kind(), TokenKind::Number);
    EXPECT_EQ(tokens[4].get_kind(), TokenKind::Number);
    EXPECT_EQ(tokens[5].get_kind(), TokenKind::Eof);

    EXPECT_EQ(l.source->get_string(tokens[0].get_span()), "31");
    EXPECT_EQ(l.source->get_string(tokens[1].get_span()), "0x04423");
    EXPECT_EQ(l.source->get_string(tokens[2].get_span()), "0b11001");
    EXPECT_EQ(l.source->get_string(tokens[3].get_span()), "0.344");
    EXPECT_EQ(l.source->get_string(tokens[4].get_span()), "0O314");
}

TEST_F(LexerTest, IgnoresUnderscoresInNumbers) {
    auto l = LexerWithSource("123_456_789 0x8040_0138 0b1101_1100");
    auto tokens = l.lex();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::Number);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Number);
    EXPECT_EQ(tokens[2].get_kind(), TokenKind::Number);
    EXPECT_EQ(tokens[3].get_kind(), TokenKind::Eof);

    EXPECT_EQ(l.source->get_string(tokens[0].get_span()), "123_456_789");
    EXPECT_EQ(l.source->get_string(tokens[1].get_span()), "0x8040_0138");
    EXPECT_EQ(l.source->get_string(tokens[2].get_span()), "0b1101_1100");
}

TEST_F(LexerTest, LexesNestedStrings) {
    auto l = LexerWithSource(R"("\"test string\"")");
    auto tokens = l.lex();

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::String);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Eof);

    EXPECT_EQ(l.source->get_string(tokens[0].get_span()), R"(\"test string\")");
}

TEST_F(LexerTest, LexesChars) {
    auto l = LexerWithSource("'a' 'b' 'c'");
    auto tokens = l.lex();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::Char);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Char);
    EXPECT_EQ(tokens[2].get_kind(), TokenKind::Char);
    EXPECT_EQ(tokens[3].get_kind(), TokenKind::Eof);

    EXPECT_EQ(l.source->get_string(tokens[0].get_span()), "a");
    EXPECT_EQ(l.source->get_string(tokens[1].get_span()), "b");
    EXPECT_EQ(l.source->get_string(tokens[2].get_span()), "c");
}

TEST_F(LexerTest, IgnoresSingleLineComments) {
    auto tokens = lex_string("test 1 // this should be ignored\nnot this");

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Number);
    EXPECT_EQ(tokens[2].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[3].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[4].get_kind(), TokenKind::Eof);
}

TEST_F(LexerTest, IgnoresMultiLineComments) {
    auto tokens =
        lex_string("test 1 /* this should \nall be\nignored */ not this");

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Number);
    EXPECT_EQ(tokens[2].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[3].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[4].get_kind(), TokenKind::Eof);
}

TEST_F(LexerTest, HandlesNestedComments) {
    auto tokens = lex_string(
        "test 1 /* this should be ignored /* this too */ */ not this");

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Number);
    EXPECT_EQ(tokens[2].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[3].get_kind(), TokenKind::Identifier);
    EXPECT_EQ(tokens[4].get_kind(), TokenKind::Eof);
}

TEST_F(LexerTest, LexesStrings) {
    auto l = LexerWithSource("\"test string\"");
    auto tokens = l.lex();

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].get_kind(), TokenKind::String);
    EXPECT_EQ(tokens[1].get_kind(), TokenKind::Eof);

    EXPECT_EQ(l.source->get_string(tokens[0].get_span()), "test string");
}
