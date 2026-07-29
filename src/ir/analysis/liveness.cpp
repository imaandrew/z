#include "liveness.h"
#include "core/types.h"
#include "ir/ir.h"
#include <stack>
#include <vector>

// Implements Algorithms 5 and 6 from
// Brandner et al., "Computing Liveness Sets for SSA-Form Programs" (2011).

namespace z::ir {

void LivenessBuilder::compute_livesets(LivenessInfo& live) {
    for (const auto& block : func.blocks) {
        for (const auto phi_id : block.phis) {
            const auto& phi = func.insts[phi_id.id];
            if (phi.op == IROp::Phi)
                live.phi_defs[block.id.id].set(phi.dest.value().id);
        }
    }

    for (usize i = 0; i < func.vreg_info.size(); i++) {
        const auto vreg = static_cast<u32>(i);
        for (const auto [inst_id, block_id] : func.vreg_info[i].uses) {
            if (func.insts[inst_id.id].op == IROp::Phi)
                live.live_out[block_id.id].set(vreg);

            up_and_mark(live, block_id, vreg);
        }
    }
}

void LivenessBuilder::up_and_mark(LivenessInfo& live, BlockID block_id,
                                  u32 vreg) {
    std::stack<BlockID> blocks;
    blocks.push(block_id);

    while (!blocks.empty()) {
        const auto bid = blocks.top();
        blocks.pop();

        if (func.vreg_info[vreg].def.block == bid &&
            func.insts[func.vreg_info[vreg].def.inst.id].op != IROp::Phi) {
            continue;
        }

        if (live.live_in[bid.id].test(vreg)) {
            continue;
        }

        live.live_in[bid.id].set(vreg);

        if (live.phi_defs[bid.id].test(vreg)) {
            continue;
        }

        for (auto pred_id : func.get_block(bid).predecessors) {
            live.live_out[pred_id.id].set(vreg);
            blocks.push(pred_id);
        }
    }
}
} // namespace z::ir
