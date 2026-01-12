#pragma once

#include "constraint.h"
#include "diagnostics.h"
#include "type.h"
#include <memory>
#include <vector>

namespace z::type {

class ConstraintSolver {
    struct Entry {
        TypeID parent;
        std::shared_ptr<Type> type;
        unsigned int rank = 0;
    };

    std::vector<Entry> entries;
    DiagnosticsEngine* diag;

    TypeID find(TypeID x);
    void union_types(TypeID x, TypeID y, std::shared_ptr<Type> merged);
    bool solve_equality(EqualityConstraint& c);
    std::shared_ptr<Type> canonicalize(std::shared_ptr<Type> type);
    bool unify_with_variable(std::shared_ptr<Type>& lhs,
                             std::shared_ptr<Type>& rhs);
    static bool can_instantiate(const InferredType* var, const Type* concrete);
    std::shared_ptr<Type> pick_more_specific(const InferredType* a,
                                             const InferredType* b);
    static bool types_compatible(const Type* a, const Type* b);

public:
    explicit ConstraintSolver(DiagnosticsEngine* diag) : diag(diag) {}

    void register_vars(std::vector<std::shared_ptr<InferredType>>& types);
    bool solve(std::vector<Constraint>& constraints);
    std::shared_ptr<Type> resolve(std::shared_ptr<Type> type);
};

} // namespace z::type
