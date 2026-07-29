#pragma once

#include "core/types.h"
#include "ir/bit_vec.h"
#include "ir/ir.h"
#include <vector>
namespace z::ir {
struct LivenessInfo {
    std::vector<BitVector> live_in;
    std::vector<BitVector> live_out;
    std::vector<BitVector> phi_defs;

    LivenessInfo(u32 num_blocks, u32 num_vregs)
        : live_in(num_blocks, BitVector(num_vregs)),
          live_out(num_blocks, BitVector(num_vregs)),
          phi_defs(num_blocks, BitVector(num_vregs)) {}
};

class LivenessBuilder {
    const IRFunction& func;

    void compute_livesets(LivenessInfo& live);
    void up_and_mark(LivenessInfo& live, BlockID block, u32 vreg);

public:
    explicit LivenessBuilder(const IRFunction& func) : func(func) {};
    LivenessInfo build() {
        auto live = LivenessInfo(func.blocks.size(), func.vreg_info.size());

        compute_livesets(live);

        return live;
    }
};
} // namespace z::ir
