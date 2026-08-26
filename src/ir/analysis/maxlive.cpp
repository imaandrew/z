#include "maxlive.h"
#include "core/types.h"
#include "ir/ir.h"
#include "liveness.h"
#include <algorithm>
#include <ranges>

namespace z::ir {
u32 compute_maxlive(const IRFunction& func, const LivenessInfo& liveinfo) {
    u32 maxlive = 0;

    for (const auto& block : func.blocks) {
        auto live = liveinfo.live_out[block.id.id];
        auto block_max = live.count();

        for (const auto [i] : block.insts | std::views::reverse) {
            const auto& inst = func.insts[i];
            if (inst.op == IROp::Dead)
                continue;

            if (inst.dest) {
                const auto d = inst.dest.value().id;
                if (!live.test(d))
                    block_max = std::max(block_max, live.count() + 1);
                live.unset(d);
            }

            if (inst.op != IROp::Phi) {
                for (const auto& op : inst.operands) {
                    if (op.is_reg()) {
                        live.set(op.as_reg().id);
                    }
                }
            }

            block_max = std::max(block_max, live.count());
        }

        block_max = std::max(block_max, live.count());
        maxlive = std::max(maxlive, block_max);
    }

    return maxlive;
}
} // namespace z::ir
