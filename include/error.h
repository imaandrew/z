#pragma once

#include "sourceman.h"
#include "token.h"
#include <cstddef>
#include <expected>
#include <format>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

template <class T> class Result {
    std::optional<T> val;
    bool valid = true;

public:
    explicit Result(bool valid = true) : val(std::nullopt), valid(valid) {};
    explicit Result(T&& val) : val(std::move(val)) {}

    [[nodiscard]] bool is_valid() const { return valid; }
    T take() {
        if (val.has_value()) {
            T tmp = std::move(val.value());
            val.reset();
            return tmp;
        }

        throw std::runtime_error("Result does not hold value");
    }
};

struct Error {};

enum class ErrorKind : std::uint8_t {
    InvalidChar,
    EndOfTokens,
    ExpectedToken,
    ExpectedSemi,
    ExpectedDecl
};

class LexerError : public Error {
    ErrorKind kind;
    std::size_t pos;

public:
    LexerError(ErrorKind kind, std::size_t pos) : kind(kind), pos(pos) {};
};

inline const std::string& get_err_msg(ErrorKind kind) {
    static std::unordered_map<ErrorKind, const std::string> err_msgs = {
        {ErrorKind::InvalidChar, "unknown character '{0}'"},
        {ErrorKind::EndOfTokens, "unexpectedly reached end of input"},
        {ErrorKind::ExpectedToken, "expected `{0}`, found `{1}`"},
        {ErrorKind::ExpectedSemi, "expected `;` after statement"},
        {ErrorKind::ExpectedDecl, "expected declaration, found `{0}`"}};

    if (auto str = err_msgs.find(kind); str != err_msgs.end()) {
        return str->second;
    }

    throw std::runtime_error("Invalid ErrorKind");
}

class DiagnosticEmitter {
    SourceManager* source;
    bool has_error = false;

public:
    explicit DiagnosticEmitter(SourceManager* source) : source(source) {};
    template <typename... Args>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    void emit(const Token& tok, ErrorKind kind, Args&&... args) {
        auto str = get_err_msg(kind);
        std::cerr << std::format("{}:{}:{}: \x1B[31merror:\033[0m ",
                                 source->get_path(), tok.get_line(),
                                 tok.get_col());
        std::cerr << std::vformat(str, std::make_format_args(args...)) << '\n';
        auto line_prefix = std::format("{:5} ", tok.get_line());
        auto val = source->get_line(tok.get_pos());
        if (!val) {
            throw std::runtime_error("DiagnosticEmitter: line does not exist");
        }
        std::cerr << std::format("{}| {}", line_prefix,
                                 val.value())
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
};