#pragma once

#include "ir/analysis/analyses.h"
#include "ir/ir.h"
#include "ir/ir_ops.h"
#include "ir/pass.h"
#include <string_view>
#include <vector>

namespace z::ir {

class SimplifyCFGPass : public IRPass {
public:
    [[nodiscard]] std::string_view name() const override {
        return "SimplifyCFGPass";
    }

    bool run(IRFunction& func, FuncAnalyses& /*analyses*/,
             const IRFile& /*file*/) override {
        bool changed = false;

        for (auto& block : func.blocks) {
            if (block.insts.size() != 1 ||
                func.get_inst(block.insts.front()).op != IROp::Jump)
                continue;

            if (block.id.id == 0)
                continue;

            auto& inst = func.get_inst(block.insts.front());
            auto& target =
                func.get_block(inst.operands.front().as_label().block_id);

            if (target.id == block.id)
                continue;

            changed = true;

            for (const auto pred_id : block.predecessors) {
                auto& pred = func.get_block(pred_id);
                for (auto& op : func.get_inst(pred.terminator()).operands) {
                    if (op.is_label() && op.as_label().block_id == block.id) {
                        op.as_label().block_id = target.id;
                    }
                }
                pred.replace_successor(block.id, target.id);
                target.remove_predecessor(block.id);
                target.add_predecessor(pred_id);
            }

            inst.op = IROp::Dead;
            block.insts.clear();
            block.predecessors.clear();
            block.successors.clear();
            block.term = TerminatorKind::Unreachable;
        }

        return changed;
    }
};
} // namespace z::ir
