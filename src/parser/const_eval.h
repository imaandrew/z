#pragma once

#include "diag/diagnostics.h"
#include "parser/ast.h"
#include "parser/const_helpers.h"
#include "type/type_arena.h"
#include "type/type_ref.h"
#include <memory>
#include <optional>
namespace z {
struct ZContext;

/// Compile-time constant evaluator for the AST
///
/// This class evaluates any expressions whose values must be known early on in
/// the compilation pipeline, before the AST is lowered to IR. It does not
/// assume that any `ASTNode`s have yet been typed as it may be run before (or
/// during) the type inference pass has occurred. Currently, the only
/// such expressions are the length components of array types, so only integer
/// expressions need to be supported; everything else gets handled during AST
/// lowering. Expressions involving aggregate types are not supported yet.
///
/// Preconditions: every type name has been declared, and ZContext::const_decls
/// contains every const declaration in the file.
class ConstantEvaluator {
    type::TypeArena* ty;
    DiagnosticsEngine* diag;
    ZContext* ctxt;

    /// Evaluates a const declaration's initializer against its declared type
    /// and caches the result (including failure). Reports an error for any
    /// circular dependencies between constants.
    std::optional<ConstValue> evaluate(ConstRecord& rec);

public:
    explicit ConstantEvaluator(ZContext& ctxt)
        : ty(&*ctxt.ty), diag(&ctxt.diag), ctxt(&ctxt) {}

    /// Evaluates `expr` as a compile-time constant of type `expected`. Literals
    /// take their type from `expected`, which is propagated through operands;
    /// identifiers must have exactly this type.
    std::optional<ConstValue> eval(const ast::Expr& expr,
                                   type::TypeRef expected);
};
} // namespace z
