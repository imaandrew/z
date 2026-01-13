#include "constraint_solve.h"
#include "constraint.h"
#include "type.h"
#include "type_ref.h"
#include <cassert>
#include <cstddef>
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

void ConstraintSolver::union_types(TypeID x, TypeID y, TypeRef merged) {
    x = find(x);
    y = find(y);
    if (x == y)
        return;

    if (entries[x].rank < entries[y].rank)
        std::swap(x, y);
    entries[y].parent = x;
    entries[x].type = merged;
    if (entries[x].rank == entries[y].rank)
        entries[x].rank++;
}

bool ConstraintSolver::solve_equality(EqualityConstraint& c) {
    auto lhs_ref = canonicalize(c.lhs);
    auto rhs_ref = canonicalize(c.rhs);

    auto* lhs = ty->get(lhs_ref);
    auto* rhs = ty->get(rhs_ref);

    if (*lhs == *rhs)
        return true;

    if (lhs->is_explicit() && rhs->is_explicit())
        return true;

    if (!types_compatible(lhs, rhs))
        return false;

    return unify_with_variable(lhs_ref, rhs_ref, lhs, rhs);
}

TypeRef ConstraintSolver::canonicalize(TypeRef type) {
    if (auto* infer = ty->get_as<InferredType>(type)) {
        return entries[find(infer->get_id())].type;
    }
    return type;
}

bool ConstraintSolver::unify_with_variable(TypeRef lhs_ref, TypeRef rhs_ref,
                                           Type* lhs, Type* rhs) {
    auto* lhs_var = dyn_cast<InferredType>(lhs);
    auto* rhs_var = dyn_cast<InferredType>(rhs);

    if (lhs_var && rhs_var) {
        auto merged = pick_more_specific(lhs_var, rhs_var);
        union_types(lhs_var->get_id(), rhs_var->get_id(), merged);
        return true;
    }

    if (lhs_var) {
        if (!can_instantiate(lhs_var, rhs))
            return false;
        entries[find(lhs_var->get_id())].type = rhs_ref;
    } else {
        if (!can_instantiate(rhs_var, lhs))
            return false;
        entries[find(rhs_var->get_id())].type = lhs_ref;
    }
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

TypeRef ConstraintSolver::pick_more_specific(const InferredType* a,
                                             const InferredType* b) {
    if (a->get_infer_type() != InferType::Var &&
        a->get_infer_type() != InferType::Block)
        return entries[a->get_id()].type;
    return entries[b->get_id()].type;
}

bool ConstraintSolver::types_compatible(const Type* a, const Type* b) {
    const auto* a_inf = dyn_cast<InferredType>(a);
    const auto* b_inf = dyn_cast<InferredType>(b);
    assert(a_inf || b_inf);

    if (a_inf && b_inf) {
        const auto a_type = a_inf->get_infer_type();
        const auto b_type = b_inf->get_infer_type();

        if (a_type == InferType::Var || b_type == InferType::Var ||
            a_type == InferType::Block || b_type == InferType::Block)
            return true;

        return a_type == b_type;
    }

    if (a_inf) {
        const auto a_type = a_inf->get_infer_type();
        return (a_type == InferType::IntLiteral && b->is_integral()) ||
               (a_type == InferType::FloatLiteral && b->is_float()) ||
               a_type == InferType::Var || a_type == InferType::Block;
    }

    const auto b_type = b_inf->get_infer_type();
    return (b_type == InferType::IntLiteral && b->is_integral()) ||
           (b_type == InferType::FloatLiteral && b->is_float()) ||
           b_type == InferType::Var || b_type == InferType::Block;
}

void ConstraintSolver::register_vars(std::vector<TypeRef>& types) {
    entries.reserve(types.size());
    for (std::size_t i = 0; i < types.size(); i++) {
        auto* inf = ty->get_as<InferredType>(types[i]);
        assert(i == inf->get_id());
        entries.emplace_back(inf->get_id(), types[i]);
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

TypeRef ConstraintSolver::resolve(TypeRef type) {
    if (auto* infer = ty->get_as<InferredType>(type)) {
        auto resolved = entries[find(infer->get_id())].type;
        auto* resolved_type = ty->get(resolved);

        if (resolved_type->is_explicit())
            return resolved;

        auto* resolved_infer = dyn_cast<InferredType>(resolved_type);
        if (resolved_infer->get_infer_type() == InferType::IntLiteral)
            return TypeArena::I32;
        if (resolved_infer->get_infer_type() == InferType::FloatLiteral)
            return TypeArena::F64;
        if (resolved_infer->get_infer_type() == InferType::Block)
            return TypeArena::VOID;
        return resolved.is_valid() ? resolved : type;
    }

    if (const auto* arr = ty->get_as<ArrayType>(type)) {
        const auto resolved_elem = resolve(arr->get_type());
        if (resolved_elem != arr->get_type()) {
            return ty->make<ArrayType>(resolved_elem, arr->get_size());
        }

        return type;
    }

    // TODO: look at other nested types

    return type;
}
} // namespace z::type
