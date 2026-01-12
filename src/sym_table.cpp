#include "sym_table.h"
#include "ast.h"
#include "diagnostics.h"
#include "string_pool.h"
#include "token.h"
#include "type.h"
#include <memory>
#include <ranges>
#include <utility>

namespace z {

bool SymbolTable::declare_var(const std::unique_ptr<ast::Identifier>& name,
                              std::shared_ptr<type::Type> type) {
    // _ variable name "discards" value
    if (name->get_id() == StringPool::UNDERSCORE)
        return true;

    const bool is_unique = stack.back()->declare_var(name, std::move(type));

    if (!is_unique) {
        auto data = diag.emit_with_notes(name->tok.get_span(),
                                         DiagnosticKind::RedeclaredVar,
                                         name->to_string());

        for (const auto* scope : stack) {
            if (auto type = scope->get_var(name->get_id())) {
                data.add_note(type->span, "first defined here");
                break;
            }
        }
        diag.emit(data);
    }

    return is_unique;
}

bool SymbolTable::declare_func(const std::unique_ptr<ast::Identifier>& name,
                               std::shared_ptr<type::FunctionType> type) {
    const bool is_unique = stack.back()->declare_func(name, std::move(type));

    if (!is_unique) {
        auto data = diag.emit_with_notes(name->tok.get_span(),
                                         DiagnosticKind::RedeclaredFunc,
                                         name->to_string());

        for (const auto* scope : stack) {
            if (auto type = scope->get_func(name->get_id())) {
                data.add_note(type->span, "first defined here");
                break;
            }
        }
        diag.emit(data);
    }

    return is_unique;
}

bool SymbolTable::declare_type(const std::unique_ptr<ast::Identifier>& name,
                               std::shared_ptr<type::Type> type) {
    const bool is_unique = stack.back()->declare_type(name, std::move(type));

    if (!is_unique) {
        auto data = diag.emit_with_notes(name->tok.get_span(),
                                         DiagnosticKind::RedeclaredType,
                                         name->to_string());

        for (const auto* scope : stack) {
            if (auto type = scope->get_type(name->get_id())) {
                data.add_note(type->span, "first defined here");
                break;
            }
        }
        diag.emit(data);
    }

    return is_unique;
}

std::shared_ptr<type::Type> SymbolTable::get_var(StringID name) const {
    for (auto* scope : std::ranges::reverse_view(stack)) {
        if (auto type = scope->get_var(name)) {
            return type->type;
        }
    }

    return nullptr;
}

std::shared_ptr<type::Type> SymbolTable::get_global_var(StringID name) const {
    if (const auto* scope = stack.front()) {
        if (auto type = scope->get_var(name)) {
            return type->type;
        }
    }

    return nullptr;
}

std::shared_ptr<type::Type> SymbolTable::get_func(StringID name) const {
    for (auto* scope : std::ranges::reverse_view(stack)) {
        if (auto type = scope->get_func(name)) {
            return type->type;
        }
    }

    return nullptr;
}

std::shared_ptr<type::Type> SymbolTable::get_type(StringID name) const {
    for (auto* scope : std::ranges::reverse_view(stack)) {
        if (auto type = scope->get_type(name)) {
            return type->type;
        }
    }

    return nullptr;
}

void SymbolTable::update_type(StringID name,
                              std::shared_ptr<type::Type>& new_type) {

    for (auto* scope : std::ranges::reverse_view(stack)) {
        if (auto type = scope->get_type(name)) {
            type->type = new_type;
        }
    }
}

bool SymbolTable::resolve_unk_type(std::shared_ptr<type::Type>& type) const {
    if (auto* unk_type = dyn_cast<type::UnknownType>(type.get())) {
        const auto ident = unk_type->get_ident()->get_id();
        if (const auto new_type = get_type(ident)) {
            type = new_type;
            return true;
        }

        diag.emit(unk_type->get_ident()->tok.get_span(),
                  DiagnosticKind::UndeclaredType,
                  unk_type->get_ident()->to_string());
        return false;
    }

    return true;
}
} // namespace z
