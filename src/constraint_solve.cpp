#include "constraint_solve.h"
#include "constraint.h"
#include "type.h"
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

namespace z::type {
TypeID ConstraintSolver::find(TypeID x) {
    if (entries[x].parent != x) {
        entries[x].parent = find(entries[x].parent);
    }
    return entries[x].parent;
}

void ConstraintSolver::union_types(TypeID x, TypeID y,
                                   std::shared_ptr<Type> merged) {
    x = find(x);
    y = find(y);
    if (x == y)
        return;

    if (entries[x].rank < entries[y].rank)
        std::swap(x, y);
    entries[y].parent = x;
    entries[x].type = std::move(merged);
    if (entries[x].rank == entries[y].rank)
        entries[x].rank++;
}

bool ConstraintSolver::solve_equality(EqualityConstraint& c) {
    auto lhs = canonicalize(c.lhs);
    auto rhs = canonicalize(c.rhs);

    if (lhs.get() == rhs.get())
        return true;

    if (!types_compatible(lhs.get(), rhs.get()))
        return false;

    if (lhs->is_explicit() && rhs->is_explicit())
        return true;

    return unify_with_variable(lhs, rhs);
}

std::shared_ptr<Type>
ConstraintSolver::canonicalize(std::shared_ptr<Type> type) {
    if (auto* infer = dyn_cast<InferredType>(type.get())) {
        return entries[find(infer->get_id())].type;
    }
    return type;
}

bool ConstraintSolver::unify_with_variable(std::shared_ptr<Type>& lhs,
                                           std::shared_ptr<Type>& rhs) {
    auto* lhs_var = dyn_cast<InferredType>(lhs.get());
    auto* rhs_var = dyn_cast<InferredType>(rhs.get());

    if ((lhs_var != nullptr) && (rhs_var != nullptr)) {
        auto merged = pick_more_specific(lhs_var, rhs_var);
        union_types(lhs_var->get_id(), rhs_var->get_id(), merged);
        return true;
    }

    auto* var = (lhs_var != nullptr) ? lhs_var : rhs_var;
    auto& concrete = (lhs_var != nullptr) ? rhs : lhs;

    if (!can_instantiate(var, concrete.get()))
        return false;

    entries[find(var->get_id())].type = concrete;
    return true;
}

bool ConstraintSolver::can_instantiate(const InferredType* var,
                                       const Type* concrete) {
    switch (var->get_infer_type()) {
    case InferType::Var:
    case InferType::Block:
        return true;
    case InferType::IntLiteral:
        return concrete->is_integral();
    case InferType::FloatLiteral:
        return concrete->is_float();
    }
    return false;
}

std::shared_ptr<Type>
ConstraintSolver::pick_more_specific(const InferredType* a,
                                     const InferredType* b) {
    if (a->get_infer_type() != InferType::Var &&
        a->get_infer_type() != InferType::Block)
        return entries[a->get_id()].type;
    return entries[b->get_id()].type;
}

bool ConstraintSolver::types_compatible(const Type* a, const Type* b) {
    return a->is_assignment_compatible(b);
}

void ConstraintSolver::register_vars(
    std::vector<std::shared_ptr<InferredType>>& types) {
    entries.reserve(types.size());
    for (std::size_t i = 0; i < types.size(); i++) {
        assert(i == types[i]->get_id());
        entries.push_back(
            Entry{.parent = types[i]->get_id(), .type = std::move(types[i])});
    }
}

bool ConstraintSolver::solve(std::vector<Constraint>& constraints) {
    bool all_ok = true;
    for (auto& c : constraints) {
        if (auto* eq = std::get_if<EqualityConstraint>(&c)) {
            if (!solve_equality(*eq))
                all_ok = false;
        }
    }
    return all_ok;
}

std::shared_ptr<Type> ConstraintSolver::resolve(std::shared_ptr<Type> type) {
    if (auto* infer = dyn_cast<InferredType>(type.get())) {
        auto resolved = entries[find(infer->get_id())].type;
        if (resolved && resolved->is_explicit())
            return resolved;

        auto* resolved_infer = dyn_cast<InferredType>(resolved.get());
        if (resolved_infer->get_infer_type() == InferType::IntLiteral)
            return std::make_shared<IntegerType>(32, true);
        if (resolved_infer->get_infer_type() == InferType::FloatLiteral)
            return std::make_shared<FloatType>(64);
        if (resolved_infer->get_infer_type() == InferType::Block)
            return std::make_shared<VoidType>();
        return resolved ? resolved : type;
    }
    return type;
}
} // namespace z::type
