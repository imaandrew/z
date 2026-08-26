#pragma once

#include "ir/analysis/analyses.h"
#include "ir/ir.h"
#include "ir/pass.h"

namespace z::ir {
class ConstPropPass : public IRPass {
public:
    [[nodiscard]] std::string_view name() const override {
        return "ConstPropPass";
    }

    bool run(IRFunction& func, FuncAnalyses& /*analyses*/) override {
        bool changed = false;

        for (auto& inst : func.insts) {
            if (inst.op != IROp::LoadConst)
                continue;

            auto imm = inst.operands.front();
            auto& uses = func.get_uses(inst.dest.value());

            std::erase_if(uses, [&](auto& u) {
                auto& use = func.insts[u.inst.id];
                if (use.op == IROp::Phi)
                    return false;

                for (auto& op : use.operands) {
                    if (op.is_reg() && op.as_reg().id == inst.dest->id) {
                        op = imm;
                    }
                }

                changed = true;
                return true;
            });
        }

        return changed;
    }
};
} // namespace z::ir
