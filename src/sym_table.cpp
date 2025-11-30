#include "sym_table.h"
#include "ast.h"
#include "diagnostics.h"
#include "token.h"
#include "type.h"
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

bool SymbolTable::declare_var(const std::unique_ptr<Identifier>& name,
                              std::shared_ptr<Type> type) {
    const bool is_unique = scopes.back()->declare_var(name, std::move(type));

    if (!is_unique) {
        auto data = diag.emit_with_notes(name->tok.get_span(),
                                         DiagnosticKind::RedeclaredVar,
                                         name->to_string());

        for (const auto* scope : scopes) {
            if (auto type = scope->get_var(name->get_ident())) {
                data.add_note(type->span, "first defined here");
                break;
            }
        }
        diag.emit(data);
    }

    return is_unique;
}

bool SymbolTable::declare_func(const std::string& name, const Token& tok,
                               std::shared_ptr<FunctionType> type) {
    const bool is_unique =
        scopes.back()->declare_func(name, tok, std::move(type));

    if (!is_unique) {
        auto data = diag.emit_with_notes(tok.get_span(),
                                         DiagnosticKind::RedeclaredFunc, name);

        for (const auto* scope : scopes) {
            if (auto type = scope->get_func(name)) {
                data.add_note(type->span, "first defined here");
                break;
            }
        }
        diag.emit(data);
    }

    return is_unique;
}

bool SymbolTable::declare_type(const std::unique_ptr<Identifier>& name,
                               std::shared_ptr<Type> type) {
    const bool is_unique = scopes.back()->declare_type(name, std::move(type));

    if (!is_unique) {
        auto data = diag.emit_with_notes(name->tok.get_span(),
                                         DiagnosticKind::RedeclaredType,
                                         name->to_string());

        for (const auto* scope : scopes) {
            if (auto type = scope->get_type(name->get_ident())) {
                data.add_note(type->span, "first defined here");
                break;
            }
        }
        diag.emit(data);
    }

    return is_unique;
}

std::shared_ptr<Type> SymbolTable::get_var(std::string_view name) const {
    for (auto* scope : std::ranges::reverse_view(scopes)) {
        if (auto type = scope->get_var(name)) {
            return type->type;
        }
    }

    return nullptr;
}

std::shared_ptr<Type> SymbolTable::get_global_var(std::string_view name) const {
    if (const auto* scope = scopes.front()) {
        if (auto type = scope->get_var(name)) {
            return type->type;
        }
    }

    return nullptr;
}

std::shared_ptr<Type> SymbolTable::get_func(const std::string& name) const {
    for (auto* scope : std::ranges::reverse_view(scopes)) {
        if (auto type = scope->get_func(name)) {
            return type->type;
        }
    }

    return nullptr;
}

std::shared_ptr<Type> SymbolTable::get_type(std::string_view name) const {
    for (auto* scope : std::ranges::reverse_view(scopes)) {
        if (auto type = scope->get_type(name)) {
            return type->type;
        }
    }

    return nullptr;
}

void SymbolTable::update_type(std::string_view name,
                              std::shared_ptr<Type>& new_type) {

    for (auto* scope : std::ranges::reverse_view(scopes)) {
        if (auto type = scope->get_type(name)) {
            type->type = new_type;
        }
    }
}

bool SymbolTable::resolve_unk_type(std::shared_ptr<Type>& type) const {
    if (auto* unk_type = dyn_cast<UnknownType>(type.get())) {
        const auto ident = unk_type->to_string();
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