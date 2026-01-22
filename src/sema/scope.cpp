#include "scope.h"
#include "core/string_pool.h"
#include "parser/ast.h"
#include "type/type.h"
#include "type/type_ref.h"
#include <memory>
#include <optional>
#include <utility>

namespace z {

bool ScopeContext::declare_var(const std::unique_ptr<ast::Identifier>& name,
                               type::TypeRef type, bool is_const,
                               bool is_initialized) {
    const bool is_unique =
        vars.insert({name->get_id(), VarInfo(type, name->tok.get_span(),
                                             is_const, is_initialized)})
            .second;

    return is_unique;
}

bool ScopeContext::declare_func(const std::unique_ptr<ast::Identifier>& name,
                                type::TypeRef type) {
    const bool is_unique =
        funcs.insert({name->get_id(), TypeWithSpan(type, name->tok.get_span())})
            .second;

    return is_unique;
}

bool ScopeContext::declare_type(const std::unique_ptr<ast::Identifier>& name,
                                type::TypeRef type) {
    const bool is_unique =
        user_defined_types
            .insert({name->get_id(), TypeWithSpan(type, name->tok.get_span())})
            .second;

    return is_unique;
}

std::optional<ScopeContext::VarInfo>
ScopeContext::get_var(StringID name) const {
    if (auto var = vars.find(name); var != vars.end()) {
        return var->second;
    }

    return std::nullopt;
}

std::optional<ScopeContext::TypeWithSpan>
ScopeContext::get_func(StringID name) const {
    if (const auto func = funcs.find(name); func != funcs.end())
        return func->second;

    return std::nullopt;
}

std::optional<ScopeContext::TypeWithSpan>
ScopeContext::get_type(StringID name) const {
    if (const auto type = user_defined_types.find(name);
        type != user_defined_types.end())
        return type->second;

    return std::nullopt;
}
} // namespace z
