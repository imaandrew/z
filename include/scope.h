#pragma once

#include "src_mgr.h"
#include "string_pool.h"
#include "type_ref.h"
#include <memory>
#include <optional>
#include <unordered_map>

namespace z {
namespace ast {
struct Identifier;
}

class ScopeContext {
    struct TypeWithSpan {
        type::TypeRef type;
        Span span;

        TypeWithSpan(type::TypeRef type, Span span) : type(type), span(span) {}
    };

    type::TypeRef scope_type;
    std::unordered_map<StringID, TypeWithSpan> vars;
    std::unordered_map<StringID, TypeWithSpan> funcs;
    std::unordered_map<StringID, TypeWithSpan> user_defined_types;

public:
    bool declare_var(const std::unique_ptr<ast::Identifier>& name,
                     type::TypeRef type);
    bool declare_func(const std::unique_ptr<ast::Identifier>& name,
                      type::TypeRef type);
    bool declare_type(const std::unique_ptr<ast::Identifier>& name,
                      type::TypeRef type);
    std::optional<TypeWithSpan> get_var(StringID name) const;
    std::optional<TypeWithSpan> get_func(StringID name) const;
    std::optional<TypeWithSpan> get_type(StringID name) const;

    bool set_type(type::TypeRef type) {
        if (scope_type.is_valid()) {
            return false;
        }

        scope_type = type;
        return true;
    }

    type::TypeRef get_type() { return scope_type; }
};
} // namespace z
