#pragma once

#include "ast.h"
#include "constraint_gen.h"
#include "constraint_solve.h"
#include "diagnostics.h"
#include "sym_table.h"
#include "type.h"
#include "zctxt.h"
#include <memory>
#include <vector>

namespace z::type {

class TypeResolver {
    std::vector<std::shared_ptr<InferredType>> types;
    SymbolTable* syms;
    DiagnosticsEngine* diag;
    ConstraintGenerator cc;
    ConstraintSolver cs;

    void resolve_decls(const ast::SourceFileDecl* file) const;

public:
    explicit TypeResolver(ZContext& ctxt)
        : syms(ctxt.syms.get()), diag(&ctxt.diag), cc(types, syms), cs(diag) {};

    void resolve(ast::SourceFileDecl* file, bool dump_constraints = false);
    void resolve_subtree(ast::ASTNode* node);
};

} // namespace z::type
