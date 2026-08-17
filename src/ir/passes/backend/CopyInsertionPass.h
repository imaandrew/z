#pragma once

#include "core/panic.h"
#include "ir/analysis/analyses.h"
#include "ir/analysis/value_set.h"
#include "ir/ir.h"
#include "ir/pass.h"
#include <initializer_list>
#include <optional>
#include <utility>
#include <vector>

namespace z::ir {
class CopyInsertionPass : public IRPass {
    IRFunction* func;
    ValueSet* values;

    VReg new_reg(VReg like, InstId inst, BlockID block) {
        auto reg_id = func->add_reg(inst, block);
        values->grow();
        return VReg{.id = reg_id, .type = like.type};
    }

    InstId get_copy_inst(std::vector<std::optional<InstId>>& v, BlockID b) {
        if (!v[b.id]) {
            auto inst = func->insts.size();
            func->insts.push_back(
                Instruction(InstId(inst), IROp::ParallelCopy, {}));
            v[b.id].emplace(inst);
        }

        return *v[b.id];
    }

    void add_copy(InstId copy_id, VReg dest, VReg src, InstId old_id,
                  BlockID block) {
        auto& copy = func->get_inst(copy_id);
        copy.add_copy_pair(dest, src);
        func->remove_use(src, old_id, block);
        func->add_use(src, copy_id, block);
        func->add_use(dest, old_id, block);
        values->merge(dest.id, src.id);
    }

    InstId isolate_operands(InstId id, BlockID block) {
        auto copy_id = InstId(func->insts.size());
        func->insts.push_back(Instruction(copy_id, IROp::ParallelCopy, {}));

        for (auto& op : func->get_inst(id).operands) {
            if (op.is_reg()) {
                const auto reg = op.as_reg();
                const auto new_arg = new_reg(reg, copy_id, block);
                add_copy(copy_id, new_arg, reg, id, block);
                op = Operand::reg(new_arg);
            }
        }

        return copy_id;
    }

    InstId make_call_copies(InstId id, BlockID block) {
        expect(func->get_inst(id).op == IROp::Call,
               "make_call_copies on non-Call inst");
        return isolate_operands(id, block);
    }

    InstId make_return_copy(InstId id, BlockID block) {
        expect(func->get_inst(id).op == IROp::Ret,
               "make_return_copy on non-Ret inst");
        return isolate_operands(id, block);
    }

public:
    [[nodiscard]] const char* name() const override {
        return "CopyInsertionPass";
    }

    bool run(IRFunction& func, FuncAnalyses& analyses) override {
        std::vector<std::optional<InstId>> prologue(func.blocks.size());
        std::vector<std::optional<InstId>> epilogue(func.blocks.size());
        this->func = &func;
        values = &analyses.values();

        for (const auto& block : func.blocks) {
            for (auto p : block.phis) {
                for (auto&& [reg, label] : func.get_inst(p).phi_operands()) {
                    const auto pred = label.block_id;
                    const auto inst_id = get_copy_inst(epilogue, pred);
                    auto copy = new_reg(reg, inst_id, pred);
                    add_copy(inst_id, copy, reg, p, pred);
                    reg = copy;
                }

                const auto inst_id = get_copy_inst(prologue, block.id);
                auto dest = *func.get_inst(p).dest;
                auto copy = new_reg(dest, inst_id, block.id);

                auto& copy_inst = func.get_inst(inst_id);
                copy_inst.add_copy_pair(dest, copy);
                func.add_use(copy, inst_id, block.id);
                values->merge(dest.id, copy.id);

                func.get_def(dest) = {inst_id, block.id};
                func.get_inst(p).dest.emplace(copy);
                func.get_def(copy) = {func.get_inst(p).id, block.id};
            }
        }

        for (auto& block : func.blocks) {
            std::vector<InstId> out;
            out.reserve(block.insts.size() + 4);

            const auto block_term = block.terminator();

            if (auto pc = prologue[block.id.id];
                pc && !func.get_inst(*pc).operands.empty())
                out.push_back(*pc);

            for (const auto id : block.insts) {
                if (id == block_term)
                    break;

                if (func.get_inst(id).op == IROp::Call)
                    out.push_back(make_call_copies(id, block.id));

                out.push_back(id);

                if (func.get_inst(id).op == IROp::Ret)
                    out.push_back(make_return_copy(id, block.id));
            }

            if (auto pc = epilogue[block.id.id];
                pc && !func.get_inst(*pc).operands.empty())
                out.push_back(*pc);

            out.push_back(block_term);

            block.insts = std::move(out);
        }

        return true;
    }
};
} // namespace z::ir
