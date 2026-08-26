#pragma once

#include "core/panic.h"
#include "core/types.h"
#include "ir/analysis/analyses.h"
#include "ir/ir.h"
#include "ir/pass.h"
#include <cstdint>
#include <initializer_list>
#include <stack>
#include <string_view>
#include <utility>
#include <vector>

namespace z::ir {
class CopyLoweringPass : public IRPass {
    IRFunction* func;
    struct Copy {
        VReg dest;
        VReg src;
    };

    std::vector<InstId> sequentialize_copies(std::vector<Copy>& pcopies,
                                             u32 num_regs, BlockID block) {
        if (pcopies.empty())
            return {};
        std::stack<u32> ready;
        std::stack<u32> todo;
        const auto max_regs = num_regs + pcopies.size();
        std::vector<u32> loc(max_regs, UINT32_MAX);
        std::vector<u32> pred(max_regs, UINT32_MAX);
        std::vector<VReg> vreg(max_regs);
        std::vector<InstId> copies;

        auto emit_copy = [&](VReg dest, VReg src) {
            auto inst_id = InstId(func->insts.size());
            func->insts.emplace_back(inst_id, IROp::Copy, dest,
                                     std::initializer_list<Operand>{{src}});
            copies.push_back(inst_id);
        };

        auto emit_copy_temp = [&](VReg src) {
            auto inst_id = InstId(func->insts.size());
            auto dest = VReg(func->add_reg(inst_id, block), src.type);
            func->insts.emplace_back(inst_id, IROp::Copy, dest,
                                     std::initializer_list<Operand>{{src}});
            copies.push_back(inst_id);
            return dest;
        };

        for (const auto [dest, src] : pcopies) {
            vreg[dest.id] = dest;
            vreg[src.id] = src;

            loc[src.id] = src.id;
            pred[dest.id] = src.id;
            todo.push(dest.id);
        }

        for (const auto [dest, _] : pcopies) {
            if (loc[dest.id] == UINT32_MAX)
                ready.push(dest.id);
        }

        while (!todo.empty()) {
            while (!ready.empty()) {
                auto b = ready.top();
                ready.pop();
                auto a = pred[b];
                auto c = loc[a];

                emit_copy(vreg[b], vreg[c]);
                loc[a] = b;
                if (a == c && pred[a] != UINT32_MAX)
                    ready.push(a);
            }

            auto b = todo.top();
            todo.pop();

            if (b != loc[pred[b]]) {
                auto n = emit_copy_temp(vreg[b]);
                vreg[n.id] = n;
                loc[b] = n.id;
                ready.push(b);
            }
        }

        return copies;
    }

    InstId emit_copy(VReg dest, VReg src) {
        auto inst_id = InstId(func->insts.size());
        func->insts.emplace_back(inst_id, IROp::Copy, dest,
                                 std::initializer_list<Operand>{{src}});
        return inst_id;
    }

    void fix_reg_info() {
        std::vector<u32> reg_map(func->num_regs(), UINT32_MAX);
        func->vreg_info = IRFunction::VRegInfo();

        for (auto& block : func->blocks) {
            for (auto inst_id : block.insts) {
                auto& inst = func->get_inst(inst_id);
                if (inst.op == IROp::Dead)
                    continue;

                if (inst.dest && reg_map[inst.dest.value().id] == UINT32_MAX) {
                    reg_map[inst.dest.value().id] =
                        func->add_reg(inst_id, block.id);
                }
            }
        }

        for (auto& block : func->blocks) {
            for (auto inst_id : block.insts) {
                auto& inst = func->get_inst(inst_id);

                if (inst.op == IROp::Dead)
                    continue;

                if (inst.dest) {
                    auto id = reg_map[inst.dest.value().id];
                    inst.dest.value().id = id;
                }

                for (auto& op : inst.operands) {
                    if (op.is_reg()) {
                        auto& reg = op.as_reg();
                        expect(reg_map[reg.id] != UINT32_MAX,
                               "undefined register in operands: {}", reg.id);
                        reg.id = reg_map[reg.id];
                        func->add_use(reg, inst_id, block.id);
                    }
                }
            }
        }

        for (auto& param : func->params) {
            expect(reg_map[param.id] != UINT32_MAX,
                   "undefined register in params: {}", param.id);
            param.id = reg_map[param.id];
        }
    }

public:
    [[nodiscard]] std::string_view name() const override {
        return "CopyLoweringPass";
    }

    bool run(IRFunction& func, FuncAnalyses& /*analyses*/) override {
        this->func = &func;

        for (auto& block : func.blocks) {
            std::deque<InstId> insts;

            for (const auto inst_id : block.insts) {
                auto& inst = func.get_inst(inst_id);
                if (inst.op == IROp::Phi) {
                    inst.op = IROp::Dead;
                    continue;
                }

                if (inst.op != IROp::ParallelCopy) {
                    insts.push_back(inst_id);
                    continue;
                }

                std::vector<Copy> pcopies;
                for (const auto& [dest, src] : inst.copy_pairs()) {
                    if (dest.id == src.id)
                        continue;
                    pcopies.emplace_back(dest, src);
                }
                auto copies =
                    sequentialize_copies(pcopies, func.num_regs(), block.id);
                insts.insert(insts.end(), copies.begin(), copies.end());
                inst.op = IROp::Dead;
            }

            for (const auto phi : block.phis()) {
                func.get_inst(phi).op = IROp::Dead;
            }

            block.insts = std::move(insts);
            block.num_phis = 0;
        }

        fix_reg_info();

        return true;
    }
};
} // namespace z::ir
