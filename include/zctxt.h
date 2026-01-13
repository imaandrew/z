#pragma once

#include "diagnostics.h"
#include "src_mgr.h"
#include "string_pool.h"
#include "sym_table.h"
#include "type.h"
#include "type_arena.h"
#include "type_ref.h"
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace z {
struct ZContext {
    std::unique_ptr<StringPool> strings;
    std::unique_ptr<SourceManager> src;
    std::unique_ptr<type::TypeArena> ty;
    std::unique_ptr<SymbolTable> syms;
    DiagnosticsEngine diag;

    explicit ZContext(std::unique_ptr<SourceManager> src)
        : strings(std::make_unique<StringPool>()), src(std::move(src)),
          ty(std::make_unique<type::TypeArena>()),
          syms(std::make_unique<SymbolTable>(this->src.get(),
                                             this->strings.get())),
          diag(this->src.get()) {}

public:
    static std::optional<ZContext> Create(const std::string& path) {
        auto src = SourceManager::CreateFromPath(path);
        if (!src)
            return std::nullopt;

        return ZContext(std::move(src));
    }

    bool resolve_unk_type(type::TypeRef& type) const;
};
} // namespace z
