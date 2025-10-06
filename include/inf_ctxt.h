#pragma once

#include "type.h"
#include <memory>
#include <utility>
#include <vector>

static const InferredType* get_inf_type(const Type* type) {
    if (type->is_variable()) {
        return dynamic_cast<const InferredType*>(
            dynamic_cast<const VariableType*>(type)->get_type().get());
    }

    return dynamic_cast<const InferredType*>(type);
}

class UnificationTable final {
    struct Value final {
        TypeID parent;
        std::shared_ptr<Type> type;
        unsigned int rank;
    };

    std::vector<Value> values;

    TypeID get_root_key(TypeID x) {
        if (values.at(x).parent != x)
            values[x].parent = get_root_key(values[x].parent);

        return values[x].parent;
    }

    void unify_roots(TypeID x, TypeID y, std::shared_ptr<Type> type) {
        const TypeID root_x = get_root_key(x);
        const TypeID root_y = get_root_key(y);

        if (root_x == root_y)
            return;

        if (values.at(root_x).rank > values.at(root_y).rank) {
            values[root_y].parent = root_x;
            values[root_x].type = std::move(type);
        } else if (values[root_x].rank < values[root_y].rank) {
            values[root_x].parent = root_y;
            values[root_y].type = std::move(type);
        } else {
            values[root_y].parent = root_x;
            values[root_x].type = std::move(type);
            values[root_x].rank++;
        }
    }

public:
    std::shared_ptr<Type> new_type(InferType inf_type) {
        const TypeID id = values.size();
        std::shared_ptr<Type> type =
            std::make_shared<InferredType>(id, inf_type);
        values.emplace_back(Value{.parent = id, .type = type, .rank = 0});
        return type;
    }

    bool unify(TypeID x, TypeID y) {
        const TypeID root_x = get_root_key(x);
        const TypeID root_y = get_root_key(y);

        if (root_x == root_y)
            return true;

        std::shared_ptr<Type> val;
        const auto type_x = values.at(root_x).type;
        const auto type_y = values.at(root_y).type;
        if (!type_x->is_assignment_compatible(type_y.get())) {
            return false;
        }

        const auto* type_x_inf = get_inf_type(type_x.get());
        const auto* type_y_inf = get_inf_type(type_y.get());

        if (type_x->is_explicit())
            val = type_x;
        else if (type_y->is_explicit())
            val = type_y;
        else if (type_x_inf->get_infer_type() != InferType::Var &&
                 type_x_inf->get_infer_type() != InferType::Block)
            val = type_x;
        else if (type_y_inf->get_infer_type() != InferType::Var &&
                 type_y_inf->get_infer_type() != InferType::Block)
            val = type_y;
        else
            val = type_x;

        unify_roots(root_x, root_y, val);
        return true;
    }

    bool unify(TypeID x, std::shared_ptr<Type> y) {
        const TypeID root_x = get_root_key(x);

        if (values.at(root_x).type &&
            !values.at(root_x).type->is_assignment_compatible(y.get())) {
            return false;
        }

        values.at(root_x).type = std::move(y);
        return true;
    }

    std::shared_ptr<Type> get_val(TypeID x) {
        return values.at(get_root_key(x)).type;
    }
};

class InferenceContext final {
    UnificationTable unification_table;

public:
    std::shared_ptr<Type> new_type(InferType inf_type) {
        return unification_table.new_type(inf_type);
    }

    bool eq(std::shared_ptr<Type> lhs, std::shared_ptr<Type> rhs) {
        if (lhs->is_explicit() && rhs->is_explicit()) {
            return true;
        }

        lhs = try_resolve(lhs);
        rhs = try_resolve(rhs);

        if (lhs->is_explicit() && rhs->is_explicit()) {
            return true;
        }

        if (!lhs->is_explicit() && !rhs->is_explicit()) {
            const auto* lhs_inf = get_inf_type(lhs.get());
            const auto* rhs_inf = get_inf_type(rhs.get());
            return unification_table.unify(lhs_inf->get_id(),
                                           rhs_inf->get_id());
        }

        if (!lhs->is_explicit()) {
            const auto* lhs_inf = get_inf_type(lhs.get());
            return unification_table.unify(lhs_inf->get_id(), rhs);
        }

        const auto* rhs_inf = get_inf_type(rhs.get());
        return unification_table.unify(rhs_inf->get_id(), lhs);
    }

    [[nodiscard]] std::shared_ptr<Type>
    try_resolve(std::shared_ptr<Type> type) {
        if (!type->is_explicit()) {
            const auto* infer = get_inf_type(type.get());
            if (auto known_type = unification_table.get_val(infer->get_id())) {
                if (type->is_variable()) {
                    dynamic_cast<VariableType*>(type.get())->replace_type(known_type);
                    return type;
                }
                return known_type;
            }
        }

        return type;
    }
};