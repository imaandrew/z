#include "scope.h"
#include "ast.h"
#include "type.h"
#include <memory>
#include <optional>
#include <string>
#include <utility>

bool ScopeContext::declare_var(const std::unique_ptr<Identifier>& name,
                               std::shared_ptr<Type> type) {
    const bool is_unique =
        vars.insert({name->to_string(),
                     TypeWithSpan(std::move(type), name->tok.get_span())})
            .second;

    return is_unique;
}

bool ScopeContext::declare_func(const std::string& name, const Token& tok,
                                std::shared_ptr<FunctionType> type) {
    const bool is_unique =
        funcs.insert({name, TypeWithSpan(std::move(type), tok.get_span())})
            .second;

    return is_unique;
}

bool ScopeContext::declare_type(const std::unique_ptr<Identifier>& name,
                                std::shared_ptr<Type> type) {
    const bool is_unique =
        user_defined_types
            .insert({name->to_string(),
                     TypeWithSpan(std::move(type), name->tok.get_span())})
            .second;

    return is_unique;
}

std::optional<ScopeContext::TypeWithSpan>
ScopeContext::get_var(const std::string& name) const {
    if (auto var = vars.find(name); var != vars.end()) {
        return var->second;
    }

    return std::nullopt;
}

std::optional<ScopeContext::TypeWithSpan>
ScopeContext::get_func(const std::string& name) const {
    if (const auto func = funcs.find(name); func != funcs.end())
        return func->second;

    return std::nullopt;
}

std::optional<ScopeContext::TypeWithSpan>
ScopeContext::get_type(const std::string& name) const {
    if (const auto type = user_defined_types.find(name);
        type != user_defined_types.end())
        return type->second;

    return std::nullopt;
}