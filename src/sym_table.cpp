#include "sym_table.h"
#include "ast.h"
#include "diagnostics.h"
#include "token.h"
#include "type.h"
#include <memory>
#include <string>
#include <utility>

bool SymbolTable::declare_var(const std::unique_ptr<Identifier>& name,
                              std::shared_ptr<Type> type) {
    const bool is_unique = scopes.back()->declare_var(name, std::move(type));

    if (!is_unique) {
        diag.emit(name->tok.get_span(), DiagnosticKind::RedeclaredVar,
                  name->to_string());
    }

    return is_unique;
}

bool SymbolTable::declare_global_var(const std::unique_ptr<Identifier>& name,
                                     std::shared_ptr<Type> type) {
    const bool is_unique =
        global_vars.insert({name->to_string(), std::move(type)}).second;

    if (!is_unique) {
        diag.emit(name->tok.get_span(), DiagnosticKind::RedeclaredVar,
                  name->to_string());
    }

    return is_unique;
}

bool SymbolTable::declare_func(const std::string& name, const Token& tok,
                               std::shared_ptr<FunctionType> type) {
    const bool is_unique = funcs.insert({name, std::move(type)}).second;

    if (!is_unique) {
        diag.emit(tok.get_span(), DiagnosticKind::RedeclaredFunc, name);
    }

    return is_unique;
}

bool SymbolTable::declare_type(const std::unique_ptr<Identifier>& name,
                               std::shared_ptr<Type> type) {
    const bool is_unique =
        user_defined_types.insert({name->to_string(), std::move(type)}).second;

    if (!is_unique) {
        diag.emit(name->tok.get_span(), DiagnosticKind::RedeclaredType,
                  name->to_string());
    }

    return is_unique;
}

std::shared_ptr<Type> SymbolTable::get_var(const std::string& name) {
    for (auto i = scopes.size() - 1; i <= 0; i--) {

        if (auto type = scopes[i]->get_var(name)) {
            return type;
        }
    }

    if (auto var = global_vars.find(name); var != global_vars.end()) {
        return var->second;
    }

    return nullptr;
}

std::shared_ptr<Type> SymbolTable::get_global_var(const std::string& name) {
    if (auto var = global_vars.find(name); var != global_vars.end()) {
        return var->second;
    }

    return nullptr;
}

std::shared_ptr<FunctionType> SymbolTable::get_func(const std::string& name) {
    if (const auto func = funcs.find(name); func != funcs.end())
        return func->second;

    return nullptr;
}

std::shared_ptr<Type> SymbolTable::get_type(const std::string& name) {
    if (const auto type = user_defined_types.find(name);
        type != user_defined_types.end())
        return type->second;

    return nullptr;
}

void SymbolTable::update_type(const std::string& name,
                              std::shared_ptr<Type>& new_type) {
    if (const auto type = user_defined_types.find(name);
        type != user_defined_types.end()) {
        type->second = new_type;
    }
}

bool SymbolTable::resolve_unk_type(std::shared_ptr<Type>& type) {
    if (auto* unk_type = dynamic_cast<UnknownType*>(type.get())) {
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