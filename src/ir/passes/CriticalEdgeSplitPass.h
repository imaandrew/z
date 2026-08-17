#pragma once

#include "ir/analysis/analyses.h"
#include "ir/ir.h"
#include "ir/pass.h"
#include <initializer_list>
#include <utility>
#include <vector>

namespace z::ir {
class CriticalEdgeSplitPass : public IRPass {
public:
    [[nodiscard]] const char* name() const override {
        return "CriticalEdgeSplitPass";
    }

    static void update_terminators(BlockID old_id, BlockID new_id,
                                   BasicBlock& b, IRFunction& func) {
        auto& inst = func.insts[b.terminator().id];
        if (inst.op != IROp::Jump && inst.op != IROp::Branch)
            return;

        for (auto& op : inst.operands) {
            if (op.is_label() && op.as_label().block_id == old_id)
                op = Operand::label(new_id);
        }
    }

    static void update_phi_operands(BlockID old_id, BlockID new_id,
                                    BasicBlock& b, IRFunction& func) {
        for (auto id : b.phis) {
            auto& inst = func.insts[id.id];

            for (auto&& [_, label] : inst.phi_operands()) {
                if (label.block_id == old_id)
                    label.block_id = new_id;
            }
        }
    }

    bool run(IRFunction& func, FuncAnalyses& /*analyses*/) override {
        std::vector<std::pair<BlockID, BlockID>> critical_edges;

        for (auto& b : func.blocks) {
            if (b.successors.size() <= 1)
                continue;
            for (auto s : b.successors) {
                if (func.get_block(s).predecessors.size() > 1)
                    critical_edges.emplace_back(b.id, s);
            }
        }

        for (auto [pred_id, succ_id] : critical_edges) {
            auto& new_block =
                func.blocks.emplace_back(BlockID(func.blocks.size()));

            auto jump_id = InstId(func.insts.size());
            func.insts.emplace_back(
                jump_id, IROp::Jump,
                std::initializer_list{Operand::label(succ_id)});
            new_block.insts.push_back(jump_id);

            new_block.term = TerminatorKind::Jump;
            new_block.add_predecessor(pred_id);
            new_block.add_successor(succ_id);

            auto& pred = func.get_block(pred_id);
            auto& succ = func.get_block(succ_id);

            update_terminators(succ_id, new_block.id, pred, func);
            update_phi_operands(pred_id, new_block.id, succ, func);

            pred.replace_successor(succ_id, new_block.id);
            succ.replace_predecessor(pred_id, new_block.id);
        }

        return true;
    }
};
} // namespace z::ir
