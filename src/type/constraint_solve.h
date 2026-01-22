#pragma once

#include "constraint.h"
#include "core/zctxt.h"
#include "diag/diagnostics.h"
#include "type.h"
#include "type_arena.h"
#include "type_ref.h"
#include <memory>
#include <vector>

namespace z::type {

class ConstraintSolver {
    struct Entry {
        TypeID parent;
        TypeRef type;
        unsigned int rank = 0;

        Entry(TypeID parent, TypeRef type) : parent(parent), type(type) {}
    };

    std::vector<Entry> entries;
    DiagnosticsEngine* diag;
    TypeArena* ty;

    TypeID find(TypeID x);
    void union_types(TypeID x, TypeID y, TypeRef merged);
    bool solve_equality(EqualityConstraint& c);
    TypeRef canonicalize(TypeRef type);
    bool unify_with_variable(TypeRef lhs_ref, TypeRef rhs_ref, Type* lhs,
                             Type* rhs);
    static bool can_instantiate(const InferredType* var, const Type* concrete);
    TypeRef pick_more_specific(const InferredType* a, const InferredType* b);
    static bool types_compatible(const Type* a, const Type* b);

public:
    explicit ConstraintSolver(ZContext& ctxt)
        : diag(&ctxt.diag), ty(ctxt.ty.get()) {}

    void register_vars(std::vector<TypeRef>& types);
    bool solve(std::vector<Constraint>& constraints);
    TypeRef resolve(TypeRef type);
};

} // namespace z::type
