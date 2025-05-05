#pragma once

#include "sourceman.h"
#include "token.h"
#include <cstddef>
#include <expected>
#include <format>
#include <iostream>
#include <unordered_map>

template <class T> class Result {
    std::optional<T> val;
    bool valid = true;

public:
    Result(bool valid = true) : val(std::nullopt), valid(valid) {};
    Result(T val) : val(std::move(val)) {};

    inline bool is_valid() const { return valid; }
    inline T take() {
        T tmp = std::move(val.value());
        val.reset();
        return tmp;
    }
};

struct Error {};

enum class ErrorKind : unsigned char {
    InvalidChar,
    EndOfTokens,
    ExpectedToken,
    ExpectedSemi,
    ExpectedDecl
};

struct LexerError : public Error {
    ErrorKind kind;
    std::size_t pos;

public:
    LexerError(ErrorKind kind, std::size_t pos) : kind(kind), pos(pos) {};
};

const std::unordered_map<ErrorKind, std::string> ERROR_MSGS = {
    {ErrorKind::InvalidChar, "unknown character '{0}'"},
    {ErrorKind::EndOfTokens, "unexpectedly reached end of input"},
    {ErrorKind::ExpectedToken, "expected `{0}`, found `{1}`"},
    {ErrorKind::ExpectedSemi, "expected `;` after statement"},
    {ErrorKind::ExpectedDecl, "expected declaration, found `{0}`"}};

class DiagnosticEmitter {
    SourceManager& source;
    bool has_error = false;

public:
    DiagnosticEmitter(SourceManager& source) : source(source) {};
    template <typename... Args>
    inline void emit(const Token& tok, ErrorKind kind, Args&&... args) {
        if (auto str = ERROR_MSGS.find(kind); str != ERROR_MSGS.end()) {
            std::cerr << std::format("{}:{}:{}: \x1B[31merror:\033[0m ",
                                     source.get_path(), tok.get_line(),
                                     tok.get_col());
            std::cerr << std::vformat(str->second,
                                      std::make_format_args(args...))
                      << '\n';
            auto line_prefix = std::format("{:5} ", tok.get_line());
            std::cerr << std::format("{}| {}", line_prefix,
                                     source.get_line(tok.get_pos()).value())
                      << '\n';
            for (int i = 0; i < line_prefix.length(); i++) {
                std::cerr << ' ';
            }
            std::cerr << '|';
            for (int i = 0; i < tok.get_col(); i++) {
                std::cerr << ' ';
            }
            std::cerr << "\x1B[32m^\033[0m" << '\n';

            has_error = true;
        }
    }
};