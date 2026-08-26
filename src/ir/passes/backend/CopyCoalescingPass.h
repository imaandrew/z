#pragma once

#include "core/types.h"
#include "ir/analysis/analyses.h"
#include "ir/analysis/liveness.h"
#include "ir/analysis/order.h"
#include "ir/ir.h"
#include "ir/merge_set.h"
#include "ir/pass.h"
#include <algorithm>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace z::ir {
class CopyCoalescingPass : public IRPass {

    class LiveAt {
        const LivenessInfo* live;
        std::unordered_map<u64, u32> last_use;

        static u64 key(u32 vreg, u32 block) {
            return (static_cast<u64>(vreg) << 32U) | block;
        }

    public:
        LiveAt(const IRFunction& func, const InstOrder& order,
               const LivenessInfo& live)
            : live(&live) {
            for (u32 v = 0; v < func.num_regs(); v++) {
                for (auto [inst, block] :
                     func.get_uses(VReg{.id = v, .type = {}})) {
                    if (func.insts[inst.id].op == IROp::Phi)
                        continue;
                    auto pos = order.get_inst_order(inst);
                    auto [it, inserted] =
                        last_use.try_emplace(key(v, block.id), pos);
                    if (!inserted)
                        it->second = std::max(it->second, pos);
                }
            }
        }

        bool operator()(VReg v, BlockID block, u32 pos) const {
            if (live->is_live_out(v, block))
                return true;
            auto it = last_use.find(key(v.id, block.id));
            return it != last_use.end() && it->second > pos;
        }
    };

public:
    [[nodiscard]] std::string_view name() const override {
        return "CopyCoalescingPass";
    }

    bool run(IRFunction& func, FuncAnalyses& analyses) override {
        const auto& order = analyses.order();
        const auto& dom_tree = analyses.dom();
        const auto live_at = LiveAt(func, order, analyses.live());
        auto& values = analyses.values();

        auto def_order = std::vector<u64>(func.num_regs(), 0);
        for (const auto& block : func.blocks) {
            for (const auto inst_id : block.insts) {
                const auto& inst = func.get_inst(inst_id);
                const u64 inst_idx =
                    static_cast<u64>(order.get_inst_order(inst_id)) << 32U;
                if (inst.op == IROp::ParallelCopy) {
                    u64 copy_idx = 0;
                    for (const auto& [dest, src] : inst.copy_pairs()) {
                        def_order[dest.id] = inst_idx | copy_idx++;
                    }
                } else if (inst.dest) {
                    def_order[inst.dest.value().id] = inst_idx;
                }
            }
        }

        const auto def_before = [&](VReg a, VReg b) {
            return def_order[a.id] < def_order[b.id];
        };

        const auto dominates = [&](VReg a, VReg b) {
            const auto def_a = func.get_def(a);
            const auto def_b = func.get_def(b);

            if (def_a.block == def_b.block) {
                return def_before(a, b);
            }

            return dom_tree.dominates(def_a.block, def_b.block);
        };

        const auto interferes = [&](VReg a, VReg b) {
            if (values.same(a.id, b.id))
                return false;
            if (!dominates(a, b)) {
                if (!dominates(b, a))
                    return true;
                std::swap(a, b);
            }
            const auto def_a = func.get_def(a);
            const auto def_b = func.get_def(b);
            if (def_a.inst == def_b.inst &&
                func.get_inst(def_a.inst).op == IROp::ParallelCopy)
                return true;
            return live_at(a, def_b.block, order.get_inst_order(def_b.inst));
        };

        auto m = buildMergeSetManager<VReg>(def_before, interferes, dominates);

        for (const auto& block : func.blocks) {
            for (const auto phi_id : block.phis()) {
                const auto& phi = func.get_inst(phi_id);
                for (const auto [reg, _] : phi.phi_operands()) {
                    m.merge(phi.dest.value(), reg);
                }
            }
        }

        for (const auto& block : func.blocks) {
            for (const auto inst_id : block.insts) {
                const auto& inst = func.get_inst(inst_id);
                if (inst.op != IROp::ParallelCopy)
                    continue;

                for (const auto [dest, src] : inst.copy_pairs()) {
                    m.try_coalesce(dest, src);
                }
            }
        }

        for (const auto& block : func.blocks) {
            for (const auto inst_id : block.insts) {
                auto& inst = func.get_inst(inst_id);

                if (inst.dest)
                    inst.dest.emplace(m.representative(*inst.dest));

                for (auto& op : inst.operands) {
                    if (op.is_reg()) {
                        op = Operand::reg(m.representative(op.as_reg()));
                    }
                }
            }
        }

        for (auto& param : func.params) {
            param = m.representative(param);
        }

        return true;
    }
};
} // namespace z::ir
