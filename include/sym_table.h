#pragma once

#include "scope.h"
#include "src_mgr.h"
#include "token.h"
#include "type.h"
#include <diagnostics.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


class SymbolTable {
    std::unordered_map<std::string, std::shared_ptr<Type>> global_vars;
    std::vector<ScopeContext*> scopes;
    std::unordered_map<std::string, std::shared_ptr<FunctionType>> funcs;
    std::unordered_map<std::string, std::shared_ptr<Type>> user_defined_types;

public:
    DiagnosticsEngine diag;

    explicit SymbolTable(SourceManager* source) : diag(source) {};

    void enter_scope(ScopeContext* scope) { scopes.push_back(scope); }

    void exit_scope() { scopes.pop_back(); }

    bool declare_global_var(const std::unique_ptr<Identifier>& name,
                            std::shared_ptr<Type> type);
    bool declare_var(const std::unique_ptr<Identifier>& name,
                     std::shared_ptr<Type> type);
    bool declare_func(const std::string& name, const Token& tok,
                      std::shared_ptr<FunctionType> type);
    bool declare_type(const std::unique_ptr<Identifier>& name,
                      std::shared_ptr<Type> type);
    std::shared_ptr<Type> get_global_var(const std::string& name);
    std::shared_ptr<Type> get_var(const std::string& name);
    std::shared_ptr<FunctionType> get_func(const std::string& name);
    std::shared_ptr<Type> get_type(const std::string& name);
    void update_type(const std::string& name, std::shared_ptr<Type>& type);
    bool resolve_unk_type(std::shared_ptr<Type>& type);
    ScopeContext* get_current_scope() { return scopes.back(); }
};
