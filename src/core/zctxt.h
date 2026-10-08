#pragma once

#include "diag/diagnostics.h"
#include "diag/src_mgr.h"
#include "parser/const_helpers.h"
#include "sema/sym_table.h"
#include "string_pool.h"
#include "type/type.h"
#include "type/type_arena.h"
#include "type/type_ref.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace z {
namespace ast {
struct Expr;
} // namespace ast

struct ZContext {
    std::unique_ptr<StringPool> strings;
    std::unique_ptr<SourceManager> src;
    std::unique_ptr<type::TypeArena> ty;
    std::unique_ptr<SymbolTable> syms;
    DiagnosticsEngine diag;
    std::vector<std::unique_ptr<ast::Expr>> pending_array_exprs;
    std::unordered_map<StringID, ConstRecord> const_decls;

    explicit ZContext(std::unique_ptr<SourceManager> src);

public:
    static std::optional<ZContext> Create(const std::string& path);
    static std::optional<ZContext> CreateFromPath(const std::string& path);
    bool resolve_unk_type(type::TypeRef& type);
};
} // namespace z
