#pragma once

#include "type.h"
#include <memory>
#include <utility>
#include <vector>

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
    TypeID new_type() {
        const TypeID id = values.size();
        values.emplace_back(Value{.parent = id, .type = nullptr, .rank = 0});
        return id;
    }

    void unify(TypeID x, TypeID y) {
        const TypeID root_x = get_root_key(x);
        const TypeID root_y = get_root_key(y);

        if (root_x == root_y)
            return;

        std::shared_ptr<Type> val;
        if (values.at(root_x).type && values.at(root_y).type) {
            if (!values.at(root_x).type->is_assignment_compatible(
                    values.at(root_y).type.get())) {
                // error
                return;
            }

            val = values.at(root_x).type;
        } else if (values.at(root_x).type) {
            val = values.at(root_x).type;
        } else if (values.at(root_y).type) {
            val = values.at(root_y).type;
        } else {
            val = nullptr;
        }

        unify_roots(root_x, root_y, val);
    }

    void unify(TypeID x, std::shared_ptr<Type> y) {
        const TypeID root_x = get_root_key(x);

        if (values.at(root_x).type &&
            !values.at(root_x).type->is_assignment_compatible(y.get())) {
            // error
            return;
        }

        values.at(root_x).type = std::move(y);
    }

    std::shared_ptr<Type> get_val(TypeID x) {
        return values.at(get_root_key(x)).type;
    }
};

class InferenceContext final {
    UnificationTable unification_table;

public:
    TypeID new_type() { return unification_table.new_type(); }

    void eq(std::shared_ptr<Type> lhs, std::shared_ptr<Type> rhs) {
        if (lhs->is_explicit() && rhs->is_explicit()) {
            return;
        }

        lhs = try_resolve(lhs);
        rhs = try_resolve(rhs);

        if (!lhs->is_explicit() && !rhs->is_explicit()) {
            auto* lhs_inf = dynamic_cast<InferredType*>(lhs.get());
            auto* rhs_inf = dynamic_cast<InferredType*>(rhs.get());
            unification_table.unify(lhs_inf->get_id(), rhs_inf->get_id());
        } else if (!lhs->is_explicit()) {
            auto* lhs_inf = dynamic_cast<InferredType*>(lhs.get());
            unification_table.unify(lhs_inf->get_id(), rhs);
        } else if (!rhs->is_explicit()) {
            auto* rhs_inf = dynamic_cast<InferredType*>(rhs.get());
            unification_table.unify(rhs_inf->get_id(), lhs);
        }
    }

    [[nodiscard]] std::shared_ptr<Type> try_resolve(std::shared_ptr<Type> type) {
        if (!type->is_explicit()) {
            auto* infer = dynamic_cast<InferredType*>(type.get());
            if (auto known_type = unification_table.get_val(infer->get_id())) {
                return known_type;
            }
        }

        return type;
    }
};