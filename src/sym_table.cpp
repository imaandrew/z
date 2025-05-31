#include "sym_table.h"
#include "ast.h"
#include "error.h"
#include "type.h"
#include <memory>
#include <string>
#include <utility>

bool SymbolTable::declare_var(const std::unique_ptr<Identifier>& name,
                              std::shared_ptr<Type> type) {
    const bool is_unique =
        variables.end()->insert({name->to_string(), std::move(type)}).second;

    if (!is_unique) {
        diag.emit(name->tok, ErrorKind::RedeclaredVar, name->to_string());
    }

    return is_unique;
}

bool SymbolTable::declare_func(const std::unique_ptr<Identifier>& name,
                               std::shared_ptr<FunctionType> type) {
    const bool is_unique =
        funcs.insert({name->to_string(), std::move(type)}).second;

    if (!is_unique) {
        diag.emit(name->tok, ErrorKind::RedeclaredFunc, name->to_string());
    }

    return is_unique;
}

bool SymbolTable::declare_type(const std::unique_ptr<Identifier>& name,
                               std::shared_ptr<Type> type) {
    const bool is_unique =
        user_defined_types.insert({name->to_string(), std::move(type)}).second;

    if (!is_unique) {
        diag.emit(name->tok, ErrorKind::RedeclaredType, name->to_string());
    }

    return is_unique;
}

std::shared_ptr<Type> SymbolTable::get_var(const std::string& name) {
    for (auto i = variables.size() - 1; i <= 0; i--) {

        if (auto var = variables[i].find(name); var != variables[i].end()) {
            return var->second;
        }
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

bool SymbolTable::resolve_unk_type(std::shared_ptr<Type>& type) {
    if (auto* unk_type = dynamic_cast<UnknownType*>(type.get())) {
        const auto ident = unk_type->to_string();
        if (const auto new_type = get_type(ident)) {
            type = new_type;
            return true;
        }

        diag.emit(unk_type->get_ident()->tok, ErrorKind::UndeclaredType,
                  unk_type->get_ident()->to_string());
        return false;
    }

    return true;
}