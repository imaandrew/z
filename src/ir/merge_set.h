#pragma once

#include <concepts>
#include <list>
#include <unordered_map>
#include <utility>
#include <vector>

namespace z::ir {
/*
    MergeSetManager inspired by Mesa's implementation of `merge_set`
    https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/compiler/nir/nir_from_ssa.c
 */
template <typename Value, std::predicate<Value, Value> DefBefore,
          std::predicate<Value, Value> Interferes,
          std::predicate<Value, Value> Dominates>
class MergeSetManager {
    struct Node {
        Node* parent = nullptr;
        unsigned rank = 0;
        Value value;

        std::list<Value> members;

        explicit Node(Value v) : parent(this), value(v) {
            members.push_back(v);
        }
    };

    Node* find(Node* n) {
        if (n->parent != n)
            n->parent = find(n->parent);
        return n->parent;
    }

    Node* get_or_create(Value v) {
        auto [it, inserted] = nodes.try_emplace(v, v);
        return &it->second;
    }

    void merge_sorted_lists(std::list<Value>& dst, std::list<Value>& src) {
        auto d_it = dst.begin();
        auto s_it = src.begin();

        while (s_it != src.end()) {
            if (d_it == dst.end() || !def_before(*d_it, *s_it)) {
                auto next = std::next(s_it);
                dst.splice(d_it, src, s_it);
                s_it = next;
            } else {
                d_it++;
            }
        }
    }

    /// Boissinot Algorithm 2
    bool sets_interfere(Value a, Value b) {
        Node* ra = find(get_or_create(a));
        Node* rb = find(get_or_create(b));

        if (ra == rb)
            return false;

        std::vector<std::pair<Value, bool>> dom;
        auto it_a = ra->members.begin();
        auto it_b = rb->members.begin();

        while (it_a != ra->members.end() || it_b != rb->members.end()) {
            Value current;
            bool from_a = false;

            if (it_b == rb->members.end()) {
                current = *it_a++;
                from_a = true;
            } else if (it_a == ra->members.end()) {
                current = *it_b++;
                from_a = false;
            } else if (def_before(*it_a, *it_b)) {
                current = *it_a++;
                from_a = true;
            } else {
                current = *it_b++;
                from_a = false;
            }

            while (!dom.empty() && !dominates(dom.back().first, current))
                dom.pop_back();

            if (!dom.empty() && dom.back().second != from_a &&
                interferes(dom.back().first, current))
                return true;

            dom.push_back({current, from_a});
        }

        return false;
    }

    DefBefore def_before;
    Interferes interferes;
    Dominates dominates;
    std::unordered_map<Value, Node> nodes;

public:
    MergeSetManager(DefBefore def_before, Interferes interferes,
                    Dominates dominates)
        : def_before(def_before), interferes(interferes), dominates(dominates) {
    }

    void merge(Value a, Value b) {
        Node* ra = find(get_or_create(a));
        Node* rb = find(get_or_create(b));

        if (ra == rb)
            return;

        if (ra->rank < rb->rank)
            std::swap(ra, rb);

        rb->parent = ra;
        if (ra->rank == rb->rank)
            ra->rank++;

        merge_sorted_lists(ra->members, rb->members);
    }

    bool try_coalesce(Value a, Value b) {
        if (sets_interfere(a, b))
            return false;
        merge(a, b);
        return true;
    }

    Value representative(Value v) { return find(get_or_create(v))->value; }
};

template <typename Value, std::predicate<Value, Value> DefBefore_,
          std::predicate<Value, Value> Interferes_,
          std::predicate<Value, Value> Dominates_>
MergeSetManager<Value, DefBefore_, Interferes_, Dominates_>
buildMergeSetManager(DefBefore_ def_before, Interferes_ interferes,
                     Dominates_ dominates) {
    return MergeSetManager<Value, DefBefore_, Interferes_, Dominates_>(
        def_before, interferes, dominates);
}

} // namespace z::ir
