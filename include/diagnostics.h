#pragma once

#include "src_mgr.h"
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template <class T> class Result {
    std::optional<T> val;
    bool valid = true;

public:
    explicit Result(const bool valid = true)
        : val(std::nullopt), valid(valid) {};
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
    UndeclaredVar,
    ExpectedInteger,
    ExpectedFloat,
    TypeMismatch,
    UnassignableType,
    InvalidAssignment,
    InvalidOperand,
    InvalidOperands,
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
            {DiagnosticKind::UndeclaredVar, "use of undeclared var `{0}`"},
            {DiagnosticKind::ExpectedInteger,
             "expected integral value, found `{0}`"},
            {DiagnosticKind::ExpectedFloat,
             "expected floating value, found `(0)`"},
            {DiagnosticKind::TypeMismatch,
             "expected value of type `{0}`, found value of type `{1}`"},
            {DiagnosticKind::UnassignableType,
             "cannot assign value to unassignable type"},
            {DiagnosticKind::InvalidAssignment, "cannot assign `{0}` to `{1}`"},
            {DiagnosticKind::InvalidOperand,
             "invalid operand to operator: `{0}`"},
            {DiagnosticKind::InvalidOperands,
             "invalid operands to operator: `{0}` and `{1}`"}};

    if (const auto str = diag_strs.find(kind); str != diag_strs.end()) {
        return str->second;
    }

    throw std::runtime_error("Invalid DiagnosticKind");
}

class DiagnosticData {
public:
    virtual ~DiagnosticData() = default;
    [[nodiscard]] virtual std::string format_message() const = 0;
    virtual void add_note(Span /*span*/, std::string /*note*/) {}
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

    void add_note(Span span, std::string note) override {
        notes.emplace_back(span, std::move(note));
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

    void add_note(Span span, std::string note) const {
        data->add_note(span, std::move(note));
    }
};

class DiagnosticsEngine {
    SourceManager* source;

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

    template <typename... Args>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    void emit(const Span& span, const DiagnosticKind kind, Args&&... args) const {
        print_diagnostic(Diagnostic(
            kind, span,
            std::make_unique<SimpleDiagnosticData>(std::vformat(
                get_diagnostic_string(kind), std::make_format_args(args...)))));
    }

    template <typename... Args>
    Diagnostic
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    emit_with_notes(const Span& span, DiagnosticKind kind, Args&&... args) const {
        auto data = std::make_unique<MultiLocationDiagnosticData>(std::vformat(
            get_diagnostic_string(kind), std::make_format_args(args...)));
        return Diagnostic(kind, span, std::move(data));
    }

    void emit(const Diagnostic& diag) const { print_diagnostic(diag); }
};