#pragma once

#include "core/colour.h"
#include "core/panic.h"
#include "core/types.h"
#include "src_mgr.h"
#include <format>
#include <functional>
#include <iostream>
#include <magic_enum/magic_enum_format.hpp> // NOLINT(misc-include-cleaner)
#include <map>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace z {

enum class DiagnosticKind : u8 {
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
    InfiniteLoop,
    OperationOverflows
};

constexpr std::string_view get_diagnostic_string(DiagnosticKind kind) {
    switch (kind) {
    case DiagnosticKind::ExpectedToken:
        return "expected `{0}`, found `{1}`";
    case DiagnosticKind::ExpectedSemi:
        return "expected `;` after statement";
    case DiagnosticKind::ExpectedDecl:
        return "expected declaration, found `{0}`";
    case DiagnosticKind::IntegerOutOfRange:
        return "integer literal out of range";
    case DiagnosticKind::FloatOutOfRange:
        return "float literal out of range";
    case DiagnosticKind::RedeclaredType:
        return "redeclaration of type `{0}`";
    case DiagnosticKind::RedeclaredFunc:
        return "redeclaration of function `{0}`";
    case DiagnosticKind::RedeclaredVar:
        return "redeclaration of variable `{0}`";
    case DiagnosticKind::DuplicateField:
        return "`{0}` has duplicate field `{1}`";
    case DiagnosticKind::UndeclaredType:
        return "use of undeclared type `{0}`";
    case DiagnosticKind::UndefinedIdentifier:
        return "`{0}` is undefined";
    case DiagnosticKind::ExpectedInteger:
        return "expected integral value, found `{0}`";
    case DiagnosticKind::ExpectedFloat:
        return "expected floating value, found `(0)`";
    case DiagnosticKind::TypeMismatch:
        return "expected `{0}`, found `{1}`";
    case DiagnosticKind::UnassignableType:
        return "cannot assign value to unassignable type";
    case DiagnosticKind::InvalidAssignment:
        return "cannot assign `{0}` to `{1}`";
    case DiagnosticKind::InvalidUnaryOperand:
        return "cannot apply unary operator `{0}` to type: `{1}`";
    case DiagnosticKind::InvalidBinaryOperand:
        return "cannot apply binary operator `{0}` to type: `{1}`";
    case DiagnosticKind::InvalidOperands:
        return "invalid operands to `{0}`: `{1}` and `{2}`";
    case DiagnosticKind::ExprNotAssignable:
        return "cannot assign value to expression";
    case DiagnosticKind::TypeHasNoFields:
        return "type `{0}` does not have fields";
    case DiagnosticKind::IncorrectArgQuantity:
        return "function takes {0} arguments but {1} were supplied";
    case DiagnosticKind::TypeCannotBeIndexed:
        return "`{0}` cannot be indexed";
    case DiagnosticKind::InvalidIndexType:
        return "`{0}` is not a valid index type";
    case DiagnosticKind::ReturnTypeMismatch:
        return "`{0}` should return `{1}` but got `{2}`";
    case DiagnosticKind::NotAStruct:
        return "non struct type `{0}` used in struct initializer";
    case DiagnosticKind::TypeNotIterable:
        return "type `{0}` is not iterable";
    case DiagnosticKind::ElseExprTypeMismatch:
        return "else clause should have same type as if clause, which has type "
               "`{0}`, instead of `{1}`";
    case DiagnosticKind::UnknownField:
        return "type `{0}` doesn't have field `{1}`";
    case DiagnosticKind::DuplicateFieldInitialization:
        return "field `{0}` already initialized";
    case DiagnosticKind::FieldNotInitialized:
        return "required field `{0}` not initialized";
    case DiagnosticKind::MoreThanOneChar:
        return "char literal may contain at most one character";
    case DiagnosticKind::NumericLiteralTooBig:
        return "numeric literal too big for `{0}`";
    case DiagnosticKind::BreakOutsideLoop:
        return "break statement outside of loop";
    case DiagnosticKind::ContinueOutsideLoop:
        return "continue statement outside of loop";
    case DiagnosticKind::UnreachableStmt:
        return "statement is unreachable";
    case DiagnosticKind::UninitializedVar:
        return "`{0}` should be initialized before use";
    case DiagnosticKind::AssignmentToConst:
        return "cannot reassign value of const variable `{0}`";
    case DiagnosticKind::DivisionByZero:
        return "denominator cannot be zero";
    case DiagnosticKind::InvalidArraySize:
        return "size of array must be a positive integer";
    case DiagnosticKind::MainFunctionParams:
        return "main function cannot have parameters";
    case DiagnosticKind::MainFunctionReturnType:
        return "main function must either return `()` or `i32`";
    case DiagnosticKind::RecursiveStructDefiniton:
        return "struct `{0}` cannot contain a field of its own type";
    case DiagnosticKind::BreakTypeMismatch:
        return "loop has type `{0}` but this statement returns a `{1}`";
    case DiagnosticKind::InfiniteLoop:
        return "infinite loop never breaks";
    case DiagnosticKind::OperationOverflows:
        return "operation overflows type `{0}`";
    }

    std::unreachable();
}

enum class DiagnosticCategory : u8 { Error, Warning };

struct Diagnostic {
    using Callback = std::function<void(const Diagnostic&)>;

    DiagnosticCategory category;
    DiagnosticKind kind;
    Span primary_location;
    std::string msg;
    Callback print_callback;

    std::optional<std::string> primary_note;
    std::vector<std::pair<Span, std::string>> notes;

    Diagnostic(DiagnosticCategory category, DiagnosticKind kind, Span loc,
               std::string msg, Callback fn)
        : category(category), kind(kind), primary_location(loc),
          msg(std::move(msg)), print_callback(std::move(fn)) {};

    ~Diagnostic() {
        if (!print_callback)
            panic("Diagnostic callback ptr is null");
        print_callback(*this);
    }

    Diagnostic(Diagnostic&&) = delete;
    Diagnostic& operator=(Diagnostic&&) = delete;

    Diagnostic(const Diagnostic&) = delete;
    Diagnostic& operator=(const Diagnostic&) = delete;

    Diagnostic& add_primary_note(std::string_view note) {
        primary_note = note;
        return *this;
    }

    Diagnostic& add_note(Span span, std::string_view note) {
        notes.emplace_back(span, note);
        return *this;
    }
};

class DiagnosticsEngine {
    SourceManager* source;
    bool has_err = false;

    void print_location(const LinePos& pos, std::ostream& out) const {
        std::print(out, "{}:{}:{}", source->get_path(), pos.get_line(),
                   pos.get_col());
    }

    static std::string make_prefix(usize line) {
        return std::format("  {}   ", line);
    }

    static void print_empty_prefix(std::ostream& out, usize width) {
        std::println(out, "{:>{}}|", "", width);
    }

    void print_source(std::ostream& out, const LinePos& pos) const {
        const auto prefix = make_prefix(pos.get_line());
        print_empty_prefix(out, prefix.length());
        std::println(out, "{}|  {}", prefix, source->get_line(pos.get_line()));
        std::print(out, "{:>{}}|", "", prefix.length());
    }

    static void print_underline(std::ostream& out, usize col, usize len,
                                char fill, std::string_view colour, bool space,
                                std::string_view msg) {
        auto underline = std::string(len, fill);
        std::println(out, "{:>{}}{}{}{}{}{}", "", col, colour, underline,
                     space ? " " : "", msg, colour::RESET);
    }

    void print_note_info(std::ostream& out, const LinePos& pos,
                         bool is_multiline, const char* colour,
                         const Span& span, char underline,
                         std::string_view msg) const {
        print_source(out, pos);

        if (is_multiline) {
            print_underline(out, 2, pos.get_col(), '_', colour, false,
                            std::string{underline});

            const auto last_pos = source->get_last_line(span, pos);
            const auto last_prefix = make_prefix(last_pos.get_line());
            std::println(out, "{:>{}}| {}|{}", "", last_prefix.length(), colour,
                         colour::RESET);
            std::println(out, "{}| {}|{}{}", last_prefix, colour, colour::RESET,
                         source->get_line(last_pos.get_line()));
            std::print(out, "{:>{}}| {}|", "", last_prefix.length(), colour);
            print_underline(out, 0, last_pos.get_col(), '_', colour, false,
                            std::format("{} {}", underline, msg));

        } else {
            print_underline(out, pos.get_col() + 2, span.len, underline, colour,
                            true, msg);
        }
    }

    void print_diagnostic(const Diagnostic& diag,
                          std::ostream& out = std::cerr) const {
        bool is_primary_span_multiline = false;
        const auto pos =
            source->get_pos(diag.primary_location, is_primary_span_multiline);
        const auto [main_colour, msg] = [](const auto& cat) {
            switch (cat) {
            case DiagnosticCategory::Error:
                return std::make_pair(colour::RED, "error");
            case DiagnosticCategory::Warning:
                return std::make_pair(colour::YELLOW, "warning");
            }
            std::unreachable();
        }(diag.category);

        print_location(pos, out);

        std::println(out, ": {}{}:{} {}", main_colour, msg, colour::RESET,
                     diag.msg);

        if (!diag.notes.empty()) {
            std::map<Span, std::pair<char, std::string>> spans;

            for (const auto& [span, label] : diag.notes) {
                spans[span] = {'~', label};
            }

            spans[diag.primary_location] = {'^',
                                            diag.primary_note.value_or("")};

            for (auto& [span, info] : spans) {
                auto [fill, label] = info;
                const auto* col = fill == '^' ? colour::RED : colour::BLUE;
                bool is_note_span_multiline = false;
                const auto note_pos =
                    source->get_pos(span, is_note_span_multiline);

                print_note_info(out, note_pos, is_note_span_multiline, col,
                                span, fill, label);
            }
            print_empty_prefix(out, make_prefix(pos.get_line()).length());
            return;
        }

        print_note_info(out, pos, is_primary_span_multiline, main_colour,
                        diag.primary_location, '^',
                        diag.primary_note.value_or(""));
        print_empty_prefix(out, make_prefix(pos.get_line()).length());
    }

    // NOLINTBEGIN(cppcoreguidelines-missing-std-forward)
    template <typename... Args>
    Diagnostic emit(DiagnosticCategory cat, const Span& span,
                    const DiagnosticKind kind, Args&&... args) {
        return Diagnostic(
            cat, kind, span,
            std::vformat(get_diagnostic_string(kind),
                         std::make_format_args(args...)),
            [this](const Diagnostic& d) { this->print_diagnostic(d); });
    }

public:
    explicit DiagnosticsEngine(SourceManager* source) : source(source) {};

    template <typename... Args>
    Diagnostic error(const Span& span, const DiagnosticKind kind,
                     Args&&... args) {
        has_err = true;
        return emit(DiagnosticCategory::Error, span, kind, args...);
    }

    template <typename... Args>
    Diagnostic warn(const Span& span, const DiagnosticKind kind,
                    Args&&... args) {
        return emit(DiagnosticCategory::Warning, span, kind, args...);
    }
    // NOLINTEND(cppcoreguidelines-missing-std-forward)

    [[nodiscard]] bool has_error() const { return has_err; }
};
} // namespace z
