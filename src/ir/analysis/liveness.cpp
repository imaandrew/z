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

    for (usize i = 0; i < func.num_regs(); i++) {
        const auto vreg = VReg{.id = static_cast<u32>(i), .type = {}};
        for (const auto [inst_id, block_id] : func.get_uses(vreg)) {
            if (inst_id.id < func.insts.size() &&
                func.insts[inst_id.id].op == IROp::Phi)
                live.live_out[block_id.id].set(i);

            up_and_mark(live, block_id, vreg);
        }
    }
}

void LivenessBuilder::up_and_mark(LivenessInfo& live, BlockID block_id,
                                  VReg vreg) {
    std::stack<BlockID> blocks;
    blocks.push(block_id);

    while (!blocks.empty()) {
        const auto bid = blocks.top();
        blocks.pop();

        const auto& def = func.get_def(vreg);
        if (def.block == bid &&
            (def.inst == NO_INST || func.insts[def.inst.id].op != IROp::Phi)) {
            continue;
        }

        if (live.live_in[bid.id].test(vreg.id)) {
            continue;
        }

        live.live_in[bid.id].set(vreg.id);

        if (live.phi_defs[bid.id].test(vreg.id)) {
            continue;
        }

        for (auto pred_id : func.get_block(bid).predecessors) {
            live.live_out[pred_id.id].set(vreg.id);
            blocks.push(pred_id);
        }
    }
}
} // namespace z::ir
