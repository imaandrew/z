#pragma once

#include "diagnostics.h"
#include "scope.h"
#include "src_mgr.h"
#include "type.h"
#include <memory>
#include <string_view>
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

public:
    DiagnosticsEngine diag;

    explicit SymbolTable(SourceManager* source)
        : scopes({ScopeContext()}), stack({&scopes.front()}), diag(source) {};

    ScopeID new_scope() {
        auto id = scopes.size();
        scopes.emplace_back();
        return ScopeID(id);
    }

    void enter_scope(ScopeID scope) { stack.push_back(&scopes[scope.id]); }

    void exit_scope() { stack.pop_back(); }

    bool declare_var(const std::unique_ptr<ast::Identifier>& name,
                     std::shared_ptr<type::Type> type);
    bool declare_func(const std::unique_ptr<ast::Identifier>& name,
                      std::shared_ptr<type::FunctionType> type);
    bool declare_type(const std::unique_ptr<ast::Identifier>& name,
                      std::shared_ptr<type::Type> type);
    [[nodiscard]] std::shared_ptr<type::Type>
    get_var(std::string_view name) const;
    [[nodiscard]] std::shared_ptr<type::Type>
    get_global_var(std::string_view name) const;
    [[nodiscard]] std::shared_ptr<type::Type>
    get_func(std::string_view name) const;
    [[nodiscard]] std::shared_ptr<type::Type>
    get_type(std::string_view name) const;
    void update_type(std::string_view name, std::shared_ptr<type::Type>& type);
    bool resolve_unk_type(std::shared_ptr<type::Type>& type) const;
    ScopeContext* get_current_scope() { return stack.back(); }
    ScopeContext& get_scope(ScopeID scope) { return scopes[scope.id]; }
};
} // namespace z
