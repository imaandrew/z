#pragma once

#include "core/types.h"
#include "ir/ir.h"
#include <algorithm>
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

inline std::vector<BlockID>
compute_reverse_postorder(const std::vector<BasicBlock>& blocks) {
    auto po = compute_postorder(blocks);
    std::ranges::reverse(po);
    return po;
}
} // namespace z::ir
