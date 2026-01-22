#pragma once

#include "constraint_gen.h"
#include "constraint_solve.h"
#include "core/zctxt.h"
#include "parser/ast.h"
#include "type_ref.h"
#include <vector>

namespace z::type {

class TypeResolver {
    std::vector<TypeRef> types;
    ZContext* ctxt;
    ConstraintGenerator cc;
    ConstraintSolver cs;

    void resolve_decls(const ast::SourceFileDecl* file) const;

public:
    explicit TypeResolver(ZContext& ctxt)
        : ctxt(&ctxt), cc(types, ctxt), cs(ctxt) {};

    void resolve(ast::SourceFileDecl* file, bool dump_constraints = false);
    void resolve_subtree(ast::ASTNode* node);
};

} // namespace z::type
