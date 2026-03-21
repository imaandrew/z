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
                func.get_reg_info(inst.dest.value()).uses.empty()) {
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

                auto& info = func.get_reg_info(op.as_reg());
                std::erase_if(info.uses,
                              [&](auto& u) { return u.first == id; });
                if (info.uses.empty() &&
                    is_removable(func.insts[info.def.id].op))
                    worklist.push_back(info.def);
            }

            inst.op = IROp::Dead;
            changed = true;
        }

        return changed;
    }
};
} // namespace z::ir
