#pragma once

#include "src_mgr.h"
#include "string_pool.h"
#include "type.h"
#include <memory>
#include <optional>
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
    std::unordered_map<StringID, TypeWithSpan> vars;
    std::unordered_map<StringID, TypeWithSpan> funcs;
    std::unordered_map<StringID, TypeWithSpan> user_defined_types;

public:
    bool declare_var(const std::unique_ptr<ast::Identifier>& name,
                     std::shared_ptr<type::Type> type);
    bool declare_func(const std::unique_ptr<ast::Identifier>& name,
                      std::shared_ptr<type::FunctionType> type);
    bool declare_type(const std::unique_ptr<ast::Identifier>& name,
                      std::shared_ptr<type::Type> type);
    std::optional<TypeWithSpan> get_var(StringID name) const;
    std::optional<TypeWithSpan> get_func(StringID name) const;
    std::optional<TypeWithSpan> get_type(StringID name) const;

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
