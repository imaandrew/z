#pragma once

#include "type.h"
#include <cstddef>
#include <error.h>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

using SymbolIndex = std::size_t;

class Symbol {
    SymbolIndex idx;
    std::span<const char> span;

public:
    Symbol(SymbolIndex idx, const char* ptr, std::size_t len)
        : idx(idx), span(ptr, len) {};

    [[nodiscard]] SymbolIndex get_index() const { return idx; }
};

class SymbolTable {
    std::vector<std::unordered_map<std::string, std::shared_ptr<Type>>>
        variables;
    std::unordered_map<std::string, std::shared_ptr<FunctionType>> funcs;
    std::unordered_map<std::string, std::shared_ptr<Type>> user_defined_types;
    SymbolIndex next_free = 0;

public:
    DiagnosticEmitter diag;

    explicit SymbolTable(SourceManager* source) : diag(source) {};

    void enter_scope() { variables.emplace_back(); }

    void exit_scope() { variables.pop_back(); }

    bool declare_var(const std::unique_ptr<Identifier>& name,
                     std::shared_ptr<Type> type);
    bool declare_func(const std::unique_ptr<Identifier>& name,
                      std::shared_ptr<FunctionType> type);
    bool declare_type(const std::unique_ptr<Identifier>& name,
                      std::shared_ptr<Type> type);
    std::shared_ptr<Type> get_var(const std::string& name);
    std::shared_ptr<FunctionType> get_func(const std::string& name);
    std::shared_ptr<Type> get_type(const std::string& name);
    bool resolve_unk_type(std::shared_ptr<Type>& type);
};
