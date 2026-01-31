#pragma once

#include "src_mgr.h"
#include <cstdint>
#include <format>
#include <iostream>
#include <magic_enum/magic_enum_format.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace z {

template <class T> class Result {
    std::optional<T> val;

public:
    Result() = default;
    explicit Result(T val) : val(std::move(val)) {}

    [[nodiscard]] bool is_valid() const { return val.has_value(); }
    const T& get() {
        if (val.has_value()) {
            return val.value();
        }

        throw std::runtime_error("Result does not hold value");
    }
    T take() {
        if (val.has_value()) {
            T tmp = std::move(val.value());
            val.reset();
            return tmp;
        }

        throw std::runtime_error("Result does not hold value");
    }
};

enum class DiagnosticKind : std::uint8_t {
    ExpectedToken,
    ExpectedSemi,
    ExpectedDecl,
    IntegerOutOfRange,
    FloatOutOfRange,
    RedeclaredType,
    RedeclaredFunc,
    RedeclaredVar,
    DuplicateField,
    UndeclaredType,
    ExpectedInteger,
    ExpectedFloat,
    TypeMismatch,
    UnassignableType,
    InvalidAssignment,
    InvalidUnaryOperand,
    InvalidBinaryOperand,
    InvalidOperands,
    UndefinedIdentifier,
    ExprNotAssignable,
    TypeHasNoFields,
    IncorrectArgQuantity,
    TypeCannotBeIndexed,
    InvalidIndexType,
    ReturnTypeMismatch,
    NotAStruct,
    TypeNotIterable,
    ElseExprTypeMismatch,
    UnknownField,
    DuplicateFieldInitialization,
    FieldNotInitialized,
    MoreThanOneChar,
    NumericLiteralTooBig,
    BreakOutsideLoop,
    ContinueOutsideLoop,
    UnreachableStmt,
    UninitializedVar,
    AssignmentToConst,
    DivisionByZero,
    InvalidArraySize,
    MainFunctionParams,
    MainFunctionReturnType,
    RecursiveStructDefiniton,
    BreakTypeMismatch,
    InfiniteLoop
};

inline const std::string& get_diagnostic_string(const DiagnosticKind kind) {
    static const std::unordered_map<DiagnosticKind, const std::string>
        diag_strs = {
            {DiagnosticKind::ExpectedToken, "expected `{0}`, found `{1}`"},
            {DiagnosticKind::ExpectedSemi, "expected `;` after statement"},
            {DiagnosticKind::ExpectedDecl, "expected declaration, found `{0}`"},
            {DiagnosticKind::IntegerOutOfRange, "integer literal out of range"},
            {DiagnosticKind::FloatOutOfRange, "float literal out of range"},
            {DiagnosticKind::RedeclaredType, "redeclaration of type `{0}`"},
            {DiagnosticKind::RedeclaredFunc, "redeclaration of function `{0}`"},
            {DiagnosticKind::RedeclaredVar, "redeclaration of variable `{0}`"},
            {DiagnosticKind::DuplicateField, "`{0}` has duplicate field `{1}`"},
            {DiagnosticKind::UndeclaredType, "use of undeclared type `{0}`"},
            {DiagnosticKind::UndefinedIdentifier, "`{0}` is undefined"},
            {DiagnosticKind::ExpectedInteger,
             "expected integral value, found `{0}`"},
            {DiagnosticKind::ExpectedFloat,
             "expected floating value, found `(0)`"},
            {DiagnosticKind::TypeMismatch, "expected `{0}`, found `{1}`"},
            {DiagnosticKind::UnassignableType,
             "cannot assign value to unassignable type"},
            {DiagnosticKind::InvalidAssignment, "cannot assign `{0}` to `{1}`"},
            {DiagnosticKind::InvalidUnaryOperand,
             "cannot apply unary operator `{0}` to type: `{1}`"},
            {DiagnosticKind::InvalidBinaryOperand,
             "cannot apply binary operator `{0}` to type: `{1}`"},
            {DiagnosticKind::InvalidOperands,
             "invalid operands to `{0}`: `{1}` and `{2}`"},
            {DiagnosticKind::ExprNotAssignable,
             "cannot assign value to expression"},
            {DiagnosticKind::TypeHasNoFields,
             "type `{0}` does not have fields"},
            {DiagnosticKind::IncorrectArgQuantity,
             "function takes {0} arguments but {1} were supplied"},
            {DiagnosticKind::TypeCannotBeIndexed, "`{0}` cannot be indexed"},
            {DiagnosticKind::InvalidIndexType,
             "`{0}` is not a valid index type"},
            {DiagnosticKind::ReturnTypeMismatch,
             "`{0}` should return `{1}` but got `{2}`"},
            {DiagnosticKind::NotAStruct,
             "non struct type `{0}` used in struct initializer"},
            {DiagnosticKind::TypeNotIterable, "type `{0}` is not iterable"},
            {DiagnosticKind::ElseExprTypeMismatch,
             "else clause should have same type as if clause, which has type "
             "`{0}`, instead of `{1}`"},
            {DiagnosticKind::UnknownField,
             "type `{0}` doesn't have field `{1}`"},
            {DiagnosticKind::DuplicateFieldInitialization,
             "field `{0}` already initialized"},
            {DiagnosticKind::FieldNotInitialized,
             "required field `{0}` not initialized"},
            {DiagnosticKind::MoreThanOneChar,
             "char literal may contain at most one character"},
            {DiagnosticKind::NumericLiteralTooBig,
             "numeric literal too big for `{0}`"},
            {DiagnosticKind::BreakOutsideLoop,
             "break statement outside of loop"},
            {DiagnosticKind::ContinueOutsideLoop,
             "continue statement outside of loop"},
            {DiagnosticKind::UnreachableStmt, "statement is unreachable"},
            {DiagnosticKind::UninitializedVar,
             "`{0}` should be initialized before use"},
            {DiagnosticKind::AssignmentToConst,
             "cannot reassign value of const variable `{0}`"},
            {DiagnosticKind::DivisionByZero, "denominator cannot be zero"},
            {DiagnosticKind::InvalidArraySize,
             "size of array must be a positive integer"},
            {DiagnosticKind::MainFunctionParams,
             "main function cannot have parameters"},
            {DiagnosticKind::MainFunctionReturnType,
             "main function must either return `()` or `i32`"},
            {DiagnosticKind::RecursiveStructDefiniton,
             "struct `{0}` cannot contain a field of its own type"},
            {DiagnosticKind::BreakTypeMismatch,
             "loop has type `{0}` but this statement returns a `{1}`"},
            {DiagnosticKind::InfiniteLoop, "infinite loop never breaks"}};

    if (const auto str = diag_strs.find(kind); str != diag_strs.end()) {
        return str->second;
    }

    throw std::runtime_error("Invalid DiagnosticKind");
}

class DiagnosticData {
public:
    DiagnosticData() = default;
    virtual ~DiagnosticData() = default;
    DiagnosticData(const DiagnosticData&) = delete;
    DiagnosticData& operator=(const DiagnosticData&) = delete;
    DiagnosticData(DiagnosticData&&) = delete;
    DiagnosticData& operator=(DiagnosticData&&) = delete;
    [[nodiscard]] virtual std::string format_message() const = 0;
    virtual void add_note(Span /*span*/, const std::string& /*note*/) {}
    [[nodiscard]] virtual std::vector<std::pair<Span, std::string>>
    get_notes() const {
        return {};
    }
};

class SimpleDiagnosticData : public DiagnosticData {
    std::string message;

public:
    explicit SimpleDiagnosticData(std::string msg) : message(std::move(msg)) {}
    [[nodiscard]] std::string format_message() const override {
        return message;
    }
};

class MultiLocationDiagnosticData : public DiagnosticData {
    std::string message;
    std::vector<std::pair<Span, std::string>> notes;

public:
    explicit MultiLocationDiagnosticData(std::string msg)
        : message(std::move(msg)) {}

    void add_note(Span span, const std::string& note) override {
        notes.emplace_back(span, note);
    }

    [[nodiscard]] std::string format_message() const override {
        return message;
    }
    [[nodiscard]] std::vector<std::pair<Span, std::string>>
    get_notes() const override {
        return notes;
    }
};

struct Diagnostic {
    DiagnosticKind kind;
    Span primary_location;
    std::unique_ptr<DiagnosticData> data;

    Diagnostic(DiagnosticKind kind, Span loc,
               std::unique_ptr<DiagnosticData> data)
        : kind(kind), primary_location(loc), data(std::move(data)) {};

    void add_note(Span span, const std::string& note) const {
        data->add_note(span, note);
    }
};

class DiagnosticsEngine {
    SourceManager* source;
    bool error = false;

    void print_location(const LinePos& pos, std::ostream& out) const {
        out << source->get_path() << ":" << pos.get_line() << ":"
            << pos.get_col();
    }

    void print_diagnostic(const Diagnostic& diag,
                          std::ostream& out = std::cerr) const {
        const auto pos = source->get_pos(diag.primary_location);
        print_location(pos, out);

        out << ": error: " << diag.data->format_message() << "\n";

        for (const auto& [loc, note] : diag.data->get_notes()) {
            print_location(source->get_pos(loc), out);
            out << ": note: " << note << "\n";
        }
    }

public:
    explicit DiagnosticsEngine(SourceManager* source) : source(source) {};

    // NOLINTBEGIN(cppcoreguidelines-missing-std-forward)
    template <typename... Args>
    void emit(const Span& span, const DiagnosticKind kind, Args&&... args) {
        error = true;
        print_diagnostic(Diagnostic(
            kind, span,
            std::make_unique<SimpleDiagnosticData>(std::vformat(
                get_diagnostic_string(kind), std::make_format_args(args...)))));
    }

    template <typename... Args>
    Diagnostic emit_with_notes(const Span& span, DiagnosticKind kind,
                               Args&&... args) {
        error = true;
        auto data = std::make_unique<MultiLocationDiagnosticData>(std::vformat(
            get_diagnostic_string(kind), std::make_format_args(args...)));
        return Diagnostic(kind, span, std::move(data));
    }
    // NOLINTEND(cppcoreguidelines-missing-std-forward)

    void emit(const Diagnostic& diag) {
        error = true;
        print_diagnostic(diag);
    }

    [[nodiscard]] bool has_error() const { return error; }
};
} // namespace z
