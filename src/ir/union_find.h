#pragma once

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <utility>
#include <vector>

namespace z::ir {
class UnionFind {
    std::vector<std::uint32_t> parent;
    std::vector<std::uint32_t> rank_;

public:
    explicit UnionFind(std::size_t n) : parent(n), rank_(n, 0) {
        std::ranges::iota(parent, 0);
    }

    std::uint32_t find(std::uint32_t x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }
        return x;
    }

    void merge(std::uint32_t a, std::uint32_t b) {
        a = find(a);
        b = find(b);
        if (a == b)
            return;
        if (rank_[a] < rank_[b])
            std::swap(a, b);
        parent[b] = a;
        if (rank_[a] == rank_[b])
            rank_[a]++;
    }

    [[nodiscard]] bool same(std::uint32_t a, std::uint32_t b) {
        return find(a) == find(b);
    }
};
} // namespace z::ir
