#pragma once

#include "ir/ir.h"
#include "ir/pass.h"
#include <vector>

namespace z::ir {
class DCEPass : public IRPass {
public:
    [[nodiscard]] const char* name() const override { return "DCEPass"; }

    static constexpr bool is_removable(IROp op) {
        switch (op) {
        case IROp::Call:
        case IROp::CallIndirect:
        case IROp::Store:
        case IROp::StoreConst:
        case IROp::Branch:
        case IROp::Jump:
        case IROp::Ret:
        case IROp::Dead:
        case IROp::Arg:
            return false;
        default:
            return true;
        }
    }

    bool run(IRFunction& func) override {
        bool changed = false;
        std::vector<InstId> worklist;

        for (const auto& inst : func.insts) {
            if (inst.dest && is_removable(inst.op) &&
                func.get_uses(inst.dest.value()).empty()) {
                worklist.push_back(inst.id);
            }
        }

        while (!worklist.empty()) {
            const auto id = worklist.back();
            worklist.pop_back();
            auto& inst = func.insts[id.id];

            for (auto& op : inst.operands) {
                if (!op.is_reg())
                    continue;

                auto& uses = func.get_uses(op.as_reg());
                const auto& def = func.get_def(op.as_reg());
                std::erase_if(uses, [&](auto& u) { return u.inst == id; });
                if (uses.empty() && is_removable(func.insts[def.inst.id].op))
                    worklist.push_back(def.inst);
            }

            inst.op = IROp::Dead;
            changed = true;
        }

        return changed;
    }
};
} // namespace z::ir
