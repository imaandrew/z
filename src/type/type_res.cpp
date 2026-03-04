#include "type_res.h"
#include "constraint.h"
#include "parser/ast.h"
#include <iostream>
#include <memory>
#include <variant>

namespace z::type {

void TypeResolver::resolve_decls(const ast::SourceFileDecl* file) const {
    for (const auto& decl : file->const_decls)
        decl->declare_type(ctxt);

    for (const auto& decl : file->decls) {
        decl->declare_type(ctxt);
    }

    for (const auto& decl : file->const_decls)
        decl->resolve_sym(ctxt);

    for (const auto& decl : file->decls) {
        decl->resolve_sym(ctxt);
    }
}

void TypeResolver::resolve(ast::SourceFileDecl* file, bool dump_constraints) {
    resolve_decls(file);

    auto constraints = cc.collect(file);

    if (dump_constraints) {
        for (const auto& c : constraints) {
            if (const auto* eq = std::get_if<EqualityConstraint>(&c))
                std::cout << eq->to_string(ctxt) << '\n';
        }
    }

    cs.register_vars(types);

    cs.solve(constraints);

    resolve_subtree(file);
}

void TypeResolver::resolve_subtree(ast::ASTNode* node) {
    if (node->has_type())
        node->set_type(cs.resolve(node->get_type()));

    for (auto* child : node->children())
        resolve_subtree(child);
}
} // namespace z::type
