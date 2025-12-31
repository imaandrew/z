#pragma once

#include "ast.h"
#include "constraint_gen.h"
#include "constraint_solve.h"
#include "diagnostics.h"
#include "src_mgr.h"
#include "sym_table.h"
#include <memory>
#include <vector>

namespace z::type {

class TypeResolver {
    std::vector<std::shared_ptr<InferredType>> types;
    SymbolTable* syms;
    DiagnosticsEngine diag;
    ConstraintGenerator cc;
    ConstraintSolver cs;

    void resolve_decls(const ast::SourceFileDecl* file) const;

public:
    TypeResolver(SymbolTable* syms, SourceManager* src)
        : syms(syms), diag(src), cc(types, syms), cs(diag) {};

    void resolve(ast::SourceFileDecl* file, bool dump_constraints = false);
    void resolve_subtree(ast::ASTNode* node);
};

} // namespace z::type
