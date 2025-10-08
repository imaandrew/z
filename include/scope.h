#pragma once

#include "src_mgr.h"
#include "token.h"
#include "type.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

class ScopeContext {
    struct TypeWithSpan {
        std::shared_ptr<Type> type;
        Span span;
    };

    std::shared_ptr<Type> scope_type;
    std::unordered_map<std::string, TypeWithSpan> vars;
    std::unordered_map<std::string, TypeWithSpan> funcs;
    std::unordered_map<std::string, TypeWithSpan> user_defined_types;

    bool declare_var(const std::unique_ptr<Identifier>& name,
                     std::shared_ptr<Type> type);
    bool declare_func(const std::string& name, const Token& tok,
                      std::shared_ptr<FunctionType> type);
    bool declare_type(const std::unique_ptr<Identifier>& name,
                      std::shared_ptr<Type> type);
    std::optional<TypeWithSpan> get_var(const std::string& name) const;
    std::optional<TypeWithSpan> get_func(const std::string& name) const;
    std::optional<TypeWithSpan> get_type(const std::string& name) const;

    friend class SymbolTable;
    friend class TypeResolver;

public:
    bool set_type(std::shared_ptr<Type> type) {
        if (scope_type) {
            return false;
        }

        scope_type = std::move(type);
        return true;
    }

    std::shared_ptr<Type>& get_type() { return scope_type; }
};