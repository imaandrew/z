#pragma once

#include "core/types.h"
#include "ir/ir.h"
#include <algorithm>
#include <ranges>
#include <stack>
#include <utility>
#include <vector>

namespace z::ir {

inline std::vector<BlockID>
compute_postorder(const std::vector<BasicBlock>& blocks) {
    std::vector<BlockID> postorder;
    postorder.reserve(blocks.size());

    auto visited = std::vector<bool>(blocks.size(), false);
    std::stack<std::pair<BlockID, bool>> stack;
    stack.emplace(0, false);

    while (!stack.empty()) {
        auto [b, returning] = stack.top();
        stack.pop();

        if (returning) {
            postorder.push_back(b);
        } else {
            stack.emplace(b, true);
            visited[b.id] = true;

            for (auto succ : blocks[b.id].successors) {
                if (!visited[succ.id])
                    stack.emplace(succ, false);
            }
        }
    }

    return postorder;
}
class InstOrder {
    std::vector<u32> inst_order;
    std::vector<BlockID> postorder_;
    std::vector<BlockID> revpostorder_;

    void compute_postorder(const std::vector<BasicBlock>& blocks) {
        postorder_.reserve(blocks.size());
        revpostorder_.reserve(blocks.size());

        auto visited = std::vector<bool>(blocks.size(), false);
        std::stack<std::pair<BlockID, bool>> stack;
        stack.emplace(0, false);

        while (!stack.empty()) {
            auto [b, returning] = stack.top();
            stack.pop();

            if (returning) {
                postorder_.push_back(b);
            } else {
                stack.emplace(b, true);
                visited[b.id] = true;

                for (auto succ : blocks[b.id].successors) {
                    if (!visited[succ.id])
                        stack.emplace(succ, false);
                }
            }
        }

        std::ranges::reverse_copy(postorder_, revpostorder_.begin());
    }

    void number_insts(const IRFunction& func) {
        auto idx = 1;

        for (auto id : postorder_ | std::views::reverse) {
            const auto& block = func.blocks.at(id.id);
            for (auto inst : block.insts) {
                inst_order[inst.id] = idx;
                idx += 2;
            }
        }
    }

public:
    explicit InstOrder(const IRFunction& func) {
        inst_order.resize(func.insts.size());
        compute_postorder(func.blocks);
        number_insts(func);
    }

    [[nodiscard]] u32 get_inst_order(InstId id) const {
        return inst_order[id.id];
    }

    const std::vector<BlockID>& postorder() const { return postorder_; }

    const std::vector<BlockID>& rev_postorder() const { return revpostorder_; }
};
} // namespace z::ir
