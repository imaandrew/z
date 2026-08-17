#pragma once

#include "core/types.h"
#include <numeric>
#include <utility>
#include <vector>

class ValueSet {
    std::vector<u32> parent;
    std::vector<u32> rank;

public:
    explicit ValueSet(u32 n) : parent(n), rank(n, 1) {
        std::ranges::iota(parent, 0);
    }

    void grow() {
        parent.push_back(parent.size());
        rank.push_back(1);
    }

    u32 find(u32 x) {
        while (parent[x] != x)
            x = parent[x] = parent[parent[x]];

        return x;
    }

    void merge(u32 x, u32 y) {
        x = find(x);
        y = find(y);

        if (x == y)
            return;

        if (rank[x] < rank[y])
            std::swap(x, y);

        parent[y] = x;
        if (rank[x] == rank[y])
            rank[x]++;
    }

    bool same(u32 x, u32 y) { return find(x) == find(y); }
};
