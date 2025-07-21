#pragma once

#include "type.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

class ScopeContext {
    std::shared_ptr<Type> scope_type;
    std::unordered_map<std::string, std::shared_ptr<Type>> local_vars;

public:
    bool set_type(std::shared_ptr<Type> type) {
        if (scope_type) {
            return false;
        }

        scope_type = std::move(type);
        return true;
    }

    std::shared_ptr<Type>& get_type() { return scope_type; }

    bool declare_var(const std::unique_ptr<Identifier>& name,
                     std::shared_ptr<Type> type);
    std::shared_ptr<Type> get_var(const std::string& name);
};