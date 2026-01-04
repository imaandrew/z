#pragma once

#include "src_mgr.h"
#include "type.h"
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace z {
namespace ast {
struct Identifier;
}

class ScopeContext {
    struct TypeWithSpan {
        std::shared_ptr<type::Type> type;
        Span span;
    };

    std::shared_ptr<type::Type> scope_type;
    std::unordered_map<std::string_view, TypeWithSpan> vars;
    std::unordered_map<std::string_view, TypeWithSpan> funcs;
    std::unordered_map<std::string_view, TypeWithSpan> user_defined_types;

public:
    bool declare_var(const std::unique_ptr<ast::Identifier>& name,
                     std::shared_ptr<type::Type> type);
    bool declare_func(const std::unique_ptr<ast::Identifier>& name,
                      std::shared_ptr<type::FunctionType> type);
    bool declare_type(const std::unique_ptr<ast::Identifier>& name,
                      std::shared_ptr<type::Type> type);
    std::optional<TypeWithSpan> get_var(std::string_view name) const;
    std::optional<TypeWithSpan> get_func(std::string_view name) const;
    std::optional<TypeWithSpan> get_type(std::string_view name) const;

    bool set_type(std::shared_ptr<type::Type> type) {
        if (scope_type) {
            return false;
        }

        scope_type = std::move(type);
        return true;
    }

    std::shared_ptr<type::Type>& get_type() { return scope_type; }
};
} // namespace z
