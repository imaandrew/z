#pragma once

#include "scope.h"
#include "src_mgr.h"
#include "token.h"
#include "type.h"
#include <diagnostics.h>
#include <memory>
#include <string>
#include <vector>

class SymbolTable {
    std::unique_ptr<ScopeContext> global_scope;
    std::vector<ScopeContext*> scopes;

public:
    DiagnosticsEngine diag;

    explicit SymbolTable(SourceManager* source)
        : global_scope(std::make_unique<ScopeContext>()),
          scopes({global_scope.get()}), diag(source) {};

    void enter_scope(ScopeContext* scope) { scopes.push_back(scope); }

    void exit_scope() { scopes.pop_back(); }

    bool declare_var(const std::unique_ptr<Identifier>& name,
                     std::shared_ptr<Type> type);
    bool declare_func(const std::string& name, const Token& tok,
                      std::shared_ptr<FunctionType> type);
    bool declare_type(const std::unique_ptr<Identifier>& name,
                      std::shared_ptr<Type> type);
    std::shared_ptr<Type> get_var(const std::string& name) const;
    std::shared_ptr<Type> get_func(const std::string& name) const;
    std::shared_ptr<Type> get_type(const std::string& name) const;
    void update_type(const std::string& name, std::shared_ptr<Type>& type);
    bool resolve_unk_type(std::shared_ptr<Type>& type);
    ScopeContext* get_current_scope() { return scopes.back(); }
};
