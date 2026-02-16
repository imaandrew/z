#pragma once

#include "core/string_pool.h"
#include "diag/diagnostics.h"
#include "diag/src_mgr.h"
#include "sema/scope.h"
#include "type/type_ref.h"
#include <deque>
#include <memory>
#include <optional>
#include <vector>

namespace z {

struct ScopeTag {};
using ScopeID = Index<ScopeTag>;

class SymbolTable {
    std::deque<ScopeContext> scopes;
    std::vector<ScopeContext*> stack;
    DiagnosticsEngine diag;
    StringPool* strings;

    [[nodiscard]] std::optional<ScopeContext::VarInfo>
    _get_var(StringID name) const;

public:
    explicit SymbolTable(SourceManager* source, StringPool* strings)
        : scopes({ScopeContext()}), stack({&scopes.front()}), diag(source),
          strings(strings) {};

    ScopeID new_scope() {
        auto id = scopes.size();
        scopes.emplace_back();
        return ScopeID(id);
    }

    void enter_scope(ScopeID scope) { stack.push_back(&scopes[scope.id]); }

    void exit_scope() { stack.pop_back(); }

    bool declare_var(const std::unique_ptr<ast::Identifier>& name,
                     type::TypeRef type, bool is_const, bool is_initialized);
    bool declare_func(const std::unique_ptr<ast::Identifier>& name,
                      type::TypeRef type);
    bool declare_type(const std::unique_ptr<ast::Identifier>& name,
                      type::TypeRef type);
    [[nodiscard]] std::optional<type::TypeRef> get_var(StringID name) const;
    [[nodiscard]] std::optional<type::TypeRef>
    get_global_var(StringID name) const;
    [[nodiscard]] std::optional<type::TypeRef> get_func(StringID name) const;
    [[nodiscard]] std::optional<type::TypeRef> get_type(StringID name) const;
    void update_type(StringID name, type::TypeRef& type);
    [[nodiscard]] bool is_var_initialized(StringID name) const;
    [[nodiscard]] bool is_var_const(StringID name) const;
    [[nodiscard]] bool is_var_local(StringID name) const;
    ScopeContext* get_current_scope() { return stack.back(); }
    ScopeContext& get_scope(ScopeID scope) { return scopes[scope.id]; }
};
} // namespace z
