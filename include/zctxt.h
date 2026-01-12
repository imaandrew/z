#pragma once

#include "diagnostics.h"
#include "src_mgr.h"
#include "string_pool.h"
#include "sym_table.h"
#include "type_arena.h"
#include <optional>

namespace z {
struct ZContext {
    std::unique_ptr<SourceManager> src;
    std::unique_ptr<type::TypeArena> ty;
    std::unique_ptr<SymbolTable> syms;
    std::unique_ptr<StringPool> strings;
    DiagnosticsEngine diag;

    explicit ZContext(std::unique_ptr<SourceManager> src)
        : src(std::move(src)), syms(std::make_unique<SymbolTable>(src.get())),
          diag(src.get()) {}

public:
    static std::optional<ZContext> Create(const std::string& path) {
        auto src = SourceManager::CreateFromPath(path);
        if (!src)
            return std::nullopt;

        return ZContext(std::move(src));
    }
};
} // namespace z
