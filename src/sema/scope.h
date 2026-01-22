#pragma once

#include "core/string_pool.h"
#include "diag/src_mgr.h"
#include "type/type_ref.h"
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

    struct VarInfo {
        TypeWithSpan type;
        bool is_const;
        bool is_initialized;

        VarInfo(type::TypeRef type, Span span, bool is_const,
                bool is_initialized)
            : type(type, span), is_const(is_const),
              is_initialized(is_initialized) {}
    };

    std::unordered_map<StringID, VarInfo> vars;
    std::unordered_map<StringID, TypeWithSpan> funcs;
    std::unordered_map<StringID, TypeWithSpan> user_defined_types;

    friend class SymbolTable;

public:
    bool declare_var(const std::unique_ptr<ast::Identifier>& name,
                     type::TypeRef type, bool is_const, bool is_initialized);
    bool declare_func(const std::unique_ptr<ast::Identifier>& name,
                      type::TypeRef type);
    bool declare_type(const std::unique_ptr<ast::Identifier>& name,
                      type::TypeRef type);
    std::optional<VarInfo> get_var(StringID name) const;
    std::optional<TypeWithSpan> get_func(StringID name) const;
    std::optional<TypeWithSpan> get_type(StringID name) const;

    void set_initialized(StringID name) {
        if (vars.contains(name)) {
            auto& var = vars.at(name);
            var.is_initialized = true;
        }
    }
};
} // namespace z
