#pragma once

#include "core/types.h"
#include "ir/ir.h"
#include <algorithm>
#include <cstdint>
#include <ranges>
#include <utility>
#include <vector>

namespace z::ir {
class DominatorTree {
    static constexpr usize UNDEFINED = UINT32_MAX;
    std::vector<u32> rpo_order;
    std::vector<u32> rpo_number;

    std::vector<u32> idom;
    std::vector<std::vector<u32>> children;
    std::vector<u32> pre;
    std::vector<u32> post;

    void reverse_postorder(const std::vector<BasicBlock>& blocks) {
        auto visited = std::vector<bool>(blocks.size(), false);
        auto stack = std::vector<std::pair<u32, bool>>();
        stack.emplace_back(0, false);

        while (!stack.empty()) {
            auto [b, returning] = stack.back();
            stack.pop_back();

            if (returning) {
                rpo_order.push_back(b);
            } else {
                stack.emplace_back(b, true);
                visited[b] = true;

                for (auto succ : blocks[b].successors) {
                    if (!visited[succ.id])
                        stack.emplace_back(succ, false);
                }
            }
        }

        std::ranges::reverse(rpo_order);

        for (usize i = 0; i < rpo_order.size(); i++) {
            rpo_number[rpo_order[i]] = i;
        }
    }

    void compute_idom(const std::vector<BasicBlock>& blocks) {
        idom[0] = 0;
        bool changed = true;

        while (changed) {
            changed = false;
            for (auto b : rpo_order | std::views::drop(1)) {
                const auto& preds = blocks[b].predecessors;

                auto new_idom = UNDEFINED;
                for (auto p : preds) {
                    if (idom[p.id] != UNDEFINED) {
                        new_idom = p.id;
                        break;
                    }
                }

                for (auto p : preds) {
                    if (p.id != new_idom && idom[p.id] != UNDEFINED)
                        new_idom = intersect(p.id, new_idom);
                }

                if (idom[b] != new_idom) {
                    idom[b] = new_idom;
                    changed = true;
                }
            }
        }
    }

    u32 intersect(u32 a, u32 b) {
        while (rpo_number[a] != rpo_number[b]) {
            while (rpo_number[a] > rpo_number[b])
                a = idom[a];
            while (rpo_number[b] > rpo_number[a])
                b = idom[b];
        }

        return a;
    }

    void compute_dom_dfs() {
        for (usize i = 1; i < idom.size(); i++) {
            if (idom[i] != UNDEFINED)
                children[idom[i]].push_back(i);
        }

        auto time = 0;
        auto stack = std::vector<std::pair<u32, bool>>();
        stack.emplace_back(0, false);

        while (!stack.empty()) {
            auto [b, returning] = stack.back();
            stack.pop_back();

            if (returning) {
                post[b] = time++;
            } else {
                pre[b] = time++;
                stack.emplace_back(b, true);
                for (auto child : children[b]) {
                    stack.emplace_back(child, false);
                }
            }
        }
    }

    DominatorTree() = default;

public:
    static DominatorTree build(const std::vector<BasicBlock>& blocks) {
        DominatorTree dt;
        auto n = blocks.size();
        dt.rpo_number.resize(n);
        dt.rpo_order.reserve(n);
        dt.children.resize(n);
        dt.pre.resize(n);
        dt.post.resize(n);
        dt.idom.assign(n, UINT32_MAX);
        dt.reverse_postorder(blocks);
        dt.compute_idom(blocks);
        dt.compute_dom_dfs();
        return dt;
    }

    [[nodiscard]] bool dominates(BlockID a, BlockID b) const {
        return pre[a.id] <= pre[b.id] && post[b.id] <= post[a.id];
    }

    [[nodiscard]] BlockID get_idom(BlockID b) const {
        return BlockID(idom[b.id]);
    }

    [[nodiscard]] u32 get_preorder_idx(BlockID b) const { return pre[b.id]; }
};
} // namespace z::ir
