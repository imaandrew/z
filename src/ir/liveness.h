#pragma once

#include "ir/dom_tree.h"
#include "ir/ir.h"
#include <algorithm>
#include <vector>
namespace z::ir {
class LiveCheck {
    const IRFunction* func;
    const DominatorTree* dom;

public:
    LiveCheck(const IRFunction& func, const DominatorTree& dom)
        : func(&func), dom(&dom) {}

    [[nodiscard]] bool is_live_in(VReg reg, BlockID block) const {
        const auto [_, def_block] = func->vreg_info[reg.id].def;
        const auto& uses = func->vreg_info[reg.id].uses;

        for (auto [_, use_block] : uses) {
            auto curr = use_block;

            while (curr != def_block && curr != dom->get_idom(curr)) {
                if (curr == block)
                    return true;

                curr = dom->get_idom(curr);
            }
        }

        return false;
    }

    [[nodiscard]] bool is_live_at_def(VReg a, VReg b) const {
        return is_live_in(a, func->vreg_info[b.id].def.second);
    }

    [[nodiscard]] bool is_live_out(VReg reg, BlockID block) const {
        return std::ranges::any_of(
            func->blocks[block.id].successors,
            [this, reg](auto succ) { return is_live_in(reg, succ); });
    }
};
} // namespace z::ir
