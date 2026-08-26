#pragma once

#include "core/panic.h"
#include "ir/analysis/analyses.h"
#include "ir/ir.h"
#include "ir/pass.h"
#include <deque>
#include <initializer_list>
#include <string_view>
#include <utility>
#include <vector>

namespace z::ir {
class ImmToReg : public IRPass {
    IRFunction* func;
    FuncAnalyses* analyses;
    std::deque<InstId> block_insts;

    void handle_arith_inst(Instruction& inst, BlockID block) {
        ASSERT(inst.operands.size() == 2);
        ASSERT(inst.dest.has_value());
        const auto* lhs = &inst.operands[0];
        const auto* rhs = &inst.operands[1];
        const auto& dest = inst.dest.value();

        if (lhs->is_reg() && rhs->is_reg())
            return;

        if (lhs->is_imm())
            std::swap(lhs, rhs);
        ASSERT(rhs->is_imm());
        ASSERT(lhs->is_reg());

        const auto& imm = rhs->as_imm().as_int();
        const auto reg = lhs->as_reg();

        if (inst.op == IROp::IAdd && imm.get_width() <= 8) // will lower to addq
            return;

        if (reg.id == dest.id && imm.get_width() <= 16) {
            return;
        }

        auto inst_id = InstId(func->insts.size());
        analyses->values().grow();
        auto t = VReg{.id = func->add_reg(inst_id, block), .type = reg.type};
        func->insts.emplace_back(inst_id, IROp::LoadConst, t,
                                 std::initializer_list<Operand>{*rhs});
        inst.operands[1] = Operand::reg(t);
        func->add_use(t, inst.id, block);
        block_insts.push_back(inst_id);
    }

    void handle_ineg(Instruction& inst, BlockID block) {
        ASSERT(inst.operands.size() == 1);
        ASSERT(inst.dest.has_value());
        const auto& v = inst.operands.front();
        const auto& dest = inst.dest.value();

        if (v.is_reg())
            return;
        ASSERT(v.is_imm());

        auto inst_id = InstId(func->insts.size());
        analyses->values().grow();
        auto t = VReg{.id = func->add_reg(inst_id, block), .type = dest.type};
        func->insts.emplace_back(inst_id, IROp::LoadConst, t,
                                 std::initializer_list<Operand>{v});
        inst.operands[1] = Operand::reg(t);
        func->add_use(t, inst.id, block);
        block_insts.push_back(inst_id);
    }

public:
    [[nodiscard]] std::string_view name() const override { return "ImmToReg"; }

    bool run(IRFunction& func, FuncAnalyses& analyses) override {
        this->func = &func;
        this->analyses = &analyses;
        for (auto& block : func.blocks) {
            for (const auto inst_id : block.insts) {
                auto& inst = func.get_inst(inst_id);

                switch (inst.op) {
                case IROp::IAdd:
                case IROp::ISub:
                case IROp::IMul:
                case IROp::SDiv:
                case IROp::UDiv:
                case IROp::SRem:
                case IROp::URem:
                case IROp::And:
                case IROp::Or:
                case IROp::Not:
                    handle_arith_inst(inst, block.id);
                    break;
                case IROp::INeg:
                    handle_ineg(inst, block.id);
                    break;
                default:
                    break;
                }

                block_insts.push_back(inst_id);
            }

            block.insts = std::move(block_insts);
        }

        return true;
    }
};
} // namespace z::ir
