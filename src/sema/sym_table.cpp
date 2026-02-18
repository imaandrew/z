#include "sym_table.h"
#include "core/string_pool.h"
#include "diag/diagnostics.h"
#include "parser/ast.h"
#include "sema/scope.h"
#include "type/type.h"
#include "type/type_ref.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <ranges>

namespace z {

std::optional<ScopeContext::VarInfo>
SymbolTable::_get_var(StringID name) const {
    for (auto* scope : std::ranges::reverse_view(stack)) {
        if (auto type = scope->get_var(name)) {
            return type;
        }
    }

    return std::nullopt;
}

bool SymbolTable::declare_var(const std::unique_ptr<ast::Identifier>& name,
                              type::TypeRef type, bool is_const,
                              bool is_initialized) {
    // _ variable name "discards" value
    if (name->get_id() == StringPool::UNDERSCORE)
        return true;

    const bool is_unique =
        stack.back()->declare_var(name, type, is_const, is_initialized);

    if (!is_unique) {
        auto err = diag.error(name->get_span(), DiagnosticKind::RedeclaredVar,
                              strings->get_string(name->get_id()));

        err.add_primary_note("defined again here");

        for (const auto* scope : stack) {
            if (auto type = scope->get_var(name->get_id())) {
                err.add_note(type->type.span, "first defined here");
                break;
            }
        }
    }

    return is_unique;
}

bool SymbolTable::declare_func(const std::unique_ptr<ast::Identifier>& name,
                               type::TypeRef type) {
    const bool is_unique = stack.back()->declare_func(name, type);

    if (!is_unique) {
        auto err = diag.error(name->get_span(), DiagnosticKind::RedeclaredFunc,
                              strings->get_string(name->get_id()));

        err.add_primary_note("defined again here");

        for (const auto* scope : stack) {
            if (auto type = scope->get_func(name->get_id())) {
                err.add_note(type->span, "first defined here");
                break;
            }
        }
    }

    return is_unique;
}

bool SymbolTable::declare_type(const std::unique_ptr<ast::Identifier>& name,
                               type::TypeRef type) {
    const bool is_unique = stack.back()->declare_type(name, type);

    if (!is_unique) {
        auto err = diag.error(name->get_span(), DiagnosticKind::RedeclaredType,
                              strings->get_string(name->get_id()));

        err.add_primary_note("defined again here");

        for (const auto* scope : stack) {
            if (auto type = scope->get_type(name->get_id())) {
                err.add_note(type->span, "first defined here");
                break;
            }
        }
    }

    return is_unique;
}

std::optional<type::TypeRef> SymbolTable::get_var(StringID name) const {
    return _get_var(name).transform(
        [](ScopeContext::VarInfo var) { return var.type.type; });
}

std::optional<type::TypeRef> SymbolTable::get_global_var(StringID name) const {
    if (const auto* scope = stack.front()) {
        if (auto type = scope->get_var(name)) {
            return type->type.type;
        }
    }

    return std::nullopt;
}

std::optional<type::TypeRef> SymbolTable::get_func(StringID name) const {
    for (auto* scope : std::ranges::reverse_view(stack)) {
        if (auto type = scope->get_func(name)) {
            return type->type;
        }
    }

    return std::nullopt;
}

std::optional<type::TypeRef> SymbolTable::get_type(StringID name) const {
    for (auto* scope : std::ranges::reverse_view(stack)) {
        if (auto type = scope->get_type(name)) {
            return type->type;
        }
    }

    return std::nullopt;
}

void SymbolTable::update_type(StringID name, type::TypeRef& new_type) {

    for (auto* scope : std::ranges::reverse_view(stack)) {
        if (auto type = scope->get_type(name)) {
            type->type = new_type;
        }
    }
}

[[nodiscard]] bool SymbolTable::is_var_initialized(StringID name) const {
    if (auto v = _get_var(name)) {
        return v->is_initialized;
    }

    return false;
}
[[nodiscard]] bool SymbolTable::is_var_const(StringID name) const {
    if (auto v = _get_var(name)) {
        return v->is_const;
    }

    return false;
}

[[nodiscard]] bool SymbolTable::is_var_local(StringID name) const {
    return std::ranges::any_of(
        stack.cbegin(), stack.cend(), [this, name](const auto* scope) {
            return scope != stack.front() && scope->get_var(name);
        });
}
} // namespace z
