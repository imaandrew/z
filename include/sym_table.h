#pragma once

#include "diagnostics.h"
#include "scope.h"
#include "src_mgr.h"
#include "string_pool.h"
#include "type_ref.h"
#include <memory>
#include <optional>
#include <vector>

namespace z {
class ScopeID {
    unsigned int id;
    explicit ScopeID(unsigned int id) : id(id) {}
    friend class SymbolTable;
};

class SymbolTable {
    std::vector<ScopeContext> scopes;
    std::vector<ScopeContext*> stack;
    DiagnosticsEngine diag;
    StringPool* strings;

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
                     type::TypeRef type);
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
    ScopeContext* get_current_scope() { return stack.back(); }
    ScopeContext& get_scope(ScopeID scope) { return scopes[scope.id]; }
};
} // namespace z
