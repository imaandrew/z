#include "scope.h"
#include "ast.h"
#include "type.h"
#include <memory>
#include <string>
#include <utility>

bool ScopeContext::declare_var(const std::unique_ptr<Identifier>& name,
                               std::shared_ptr<Type> type) {
    return local_vars.insert({name->to_string(), std::move(type)}).second;
}

std::shared_ptr<Type> ScopeContext::get_var(const std::string& name) {
    if (auto var = local_vars.find(name); var != local_vars.end()) {
        return var->second;
    }

    return nullptr;
}
