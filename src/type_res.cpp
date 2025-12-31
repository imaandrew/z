#include "type_res.h"
#include "ast.h"
#include <memory>

namespace z::type {

void TypeResolver::resolve_decls(const ast::SourceFileDecl* file) const {
    for (const auto& decl : file->decls) {
        decl->declare_type(syms);
    }

    for (const auto& decl : file->decls) {
        decl->resolve_sym(syms);
    }
}

void TypeResolver::resolve(ast::SourceFileDecl* file) {
    resolve_decls(file);

    auto constraints = cc.collect(file);

    cs.register_vars(types);

    cs.solve(constraints);

    resolve_subtree(file);
}

void TypeResolver::resolve_subtree(ast::ASTNode* node) {
    if (node->has_type())
        node->node_type = cs.resolve(node->node_type);

    for (auto* child : node->children())
        resolve_subtree(child);
}
} // namespace z::type
