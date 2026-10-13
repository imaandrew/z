#pragma once

#include "core/index.h"
#include "core/match.h"
#include "core/panic.h"
#include "core/types.h"
#include "ir/analysis/analyses.h"
#include "ir/constants.h"
#include "ir/ir.h"
#include "ir/ir_ops.h"
#include "ir/pass.h"
#include "type/type.h"
#include "type/type_ref.h"
#include <deque>
#include <initializer_list>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace z::ir {
class ImmToReg : public IRPass {
    IRFunction* func{};
    FuncAnalyses* analyses{};
    std::deque<InstId> block_insts;

    VReg emit_load_const(u64 c, BlockID block, type::TypeRef type) {
        const auto ldc_inst_id = InstId(func->insts.size());
        analyses->values().grow();
        const auto loaded_const =
            VReg{.id = func->add_reg(ldc_inst_id, block), .type = type};

        const auto const_idx = ConstInt(func->const_pool.intern(c), 16, false);

        func->insts.emplace_back(
            ldc_inst_id, IROp::LoadConst, loaded_const,
            std::initializer_list<Operand>{
                Operand::imm(const_idx, type::builtin::U16),
            });
        block_insts.push_back(ldc_inst_id);

        return loaded_const;
    }

    VReg emit_load_imm(const ConstInt& imm, BlockID block, type::TypeRef type) {
        const auto ldi_inst_id = InstId(func->insts.size());
        analyses->values().grow();
        const auto loaded_const =
            VReg{.id = func->add_reg(ldi_inst_id, block), .type = type};

        func->insts.emplace_back(ldi_inst_id, IROp::Copy, loaded_const,
                                 std::initializer_list<Operand>{
                                     Operand::imm(imm, type),
                                 });
        block_insts.push_back(ldi_inst_id);

        return loaded_const;
    }

    void intern(Operand& op, const Instruction& inst, BlockID block,
                type::TypeRef type) {
        if (!op.is_imm())
            return;

        const auto load_bits = [&](u64 bits) {
            const auto loaded_const = emit_load_const(bits, block, type);
            op = Operand::reg(loaded_const);
            func->add_use(loaded_const, inst.id, block);
        };

        const auto& imm = op.as_imm();
        if (imm.is_int()) {
            const auto& i = imm.as_int();

            if (i.fits_in(16, true)) {
                const auto loaded_imm = emit_load_imm(i, block, type);
                op = Operand::reg(loaded_imm);
                func->add_use(loaded_imm, inst.id, block);
                return;
            }

            load_bits(i.is_signed() ? static_cast<u64>(i.get_signed())
                                    : i.get_bits());
        } else if (imm.is_float()) {
            load_bits(static_cast<u64>(imm.as_float().get_bits()));
        } else if (imm.is_string()) {
            panic("ImmToReg: cannot intern string");
        } else if (imm.is_char()) {
            panic("ImmToReg: cannot intern char");
        }
    }

    void handle_all_operands(Instruction& inst, BlockID block,
                             type::TypeRef type) {
        for (auto& op : inst.operands) {
            intern(op, inst, block, type);
        }
    }

    void handle_with_imm_inst(Instruction& inst, BlockID block) {
        auto& lhs = inst.operands[0];
        auto& rhs = inst.operands[1];

        if (lhs.is_reg() && rhs.is_reg())
            return;

        const auto can_use_imm_inst = [&](const Immediate& imm) {
            if (!imm.is_int())
                return false;

            const auto& imm_int = imm.as_int();
            const bool is_signed =
                matches(inst.op, IROp::Shl, IROp::Lsr, IROp::Asr, IROp::And,
                        IROp::Or, IROp::Xor)
                    ? false
                    : imm_int.is_signed();
            return imm_int.fits_in(16, is_signed);
        };

        if (lhs.is_reg() && rhs.is_imm() && can_use_imm_inst(rhs.as_imm()))
            return;

        if (lhs.is_imm() && rhs.is_reg() && is_commutative(inst.op) &&
            can_use_imm_inst(lhs.as_imm()))
            return;

        intern(lhs, inst, block, inst.dest.value().type);
        intern(rhs, inst, block,
               matches(inst.op, IROp::Shl, IROp::Lsr, IROp::Asr)
                   ? type::builtin::I64
                   : inst.dest.value().type);
    }

    void handle_icmp(Instruction& inst, BlockID block) {
        ASSERT(inst.op == IROp::ICmp);

        auto& lhs = inst.operands[1];
        auto& rhs = inst.operands[2];

        if (lhs.is_reg() && rhs.is_reg())
            return;

        if (lhs.is_reg()) {
            intern(rhs, inst, block, lhs.as_reg().type);
            return;
        }

        if (rhs.is_reg()) {
            intern(lhs, inst, block, rhs.as_reg().type);
            return;
        }

        const auto& lhs_i = lhs.as_imm().as_int();
        const auto& rhs_i = rhs.as_imm().as_int();

        if (lhs_i.is_signed() || rhs_i.is_signed()) {
            intern(lhs, inst, block, type::builtin::I64);
            intern(rhs, inst, block, type::builtin::I64);
        } else {
            intern(lhs, inst, block, type::builtin::U64);
            intern(rhs, inst, block, type::builtin::U64);
        }
    }

    void handle_fcmp(Instruction& inst, BlockID block) {
        ASSERT(inst.op == IROp::FCmp);

        auto& lhs = inst.operands[1];
        auto& rhs = inst.operands[2];

        if (lhs.is_reg() && rhs.is_reg())
            return;

        if (lhs.is_reg()) {
            intern(rhs, inst, block, lhs.as_reg().type);
            return;
        }

        if (rhs.is_reg()) {
            intern(lhs, inst, block, rhs.as_reg().type);
            return;
        }

        intern(lhs, inst, block, type::builtin::F64);
        intern(rhs, inst, block, type::builtin::F64);
    }

    void handle_store(Instruction& inst, BlockID block) {
        ASSERT(inst.op == IROp::Store);
        const auto& ptr = inst.operands[0].as_reg();
        auto& store_val = inst.operands[1];

        if (store_val.is_reg())
            return;

        const auto ptr_type = analyses->ctxt()
                                  .ty->get_as<type::PointerType>(ptr.type)
                                  ->get_type();

        intern(store_val, inst, block, ptr_type);
    }

    void handle_call(Instruction& inst, BlockID block, const IRFile& file) {
        ASSERT(inst.op == IROp::Call);

        const auto func_id = inst.operands.front().as_func();
        const auto& callee = file.funcs[func_id.id];

        for (usize i = 1; i < inst.operands.size(); i++) {
            intern(inst.operands[i], inst, block, callee.params[i - 1].type);
        }
    }

    void handle_copy(Instruction& inst) {
        ASSERT(inst.op == IROp::Copy);

        auto& val = inst.operands.front();
        if (val.is_reg())
            return;
        ASSERT(val.is_imm());

        const auto load_bits = [&](u64 bits) {
            const auto const_idx =
                ConstInt(func->const_pool.intern(bits), 16, false);
            val = Operand::imm(const_idx, type::builtin::U16);
            inst.op = IROp::LoadConst;
        };

        const auto& imm = val.as_imm();
        if (imm.is_int()) {
            const auto& int_imm = imm.as_int();
            if (int_imm.fits_in(16, true))
                return;

            load_bits(int_imm.is_signed()
                          ? static_cast<u64>(int_imm.get_signed())
                          : int_imm.get_bits());
        } else if (imm.is_float()) {
            const auto& float_imm = imm.as_float();

            load_bits(static_cast<u64>(float_imm.get_bits()));
        } else if (imm.is_string()) {
            panic("ImmToReg: cannot intern string");
        } else if (imm.is_char()) {
            panic("ImmToReg: cannot intern char");
        }
    }

public:
    [[nodiscard]] std::string_view name() const override { return "ImmToReg"; }

    bool run(IRFunction& func, FuncAnalyses& analyses,
             const IRFile& file) override {
        this->func = &func;
        this->analyses = &analyses;

        for (auto& block : func.blocks) {
            for (const auto inst_id : block.insts) {
                auto& inst = func.get_inst(inst_id);

                switch (inst.op) {
                case IROp::IAdd: {
                    auto& lhs = inst.operands[0];
                    auto& rhs = inst.operands[1];

                    if (!(lhs.is_imm() && rhs.is_imm())) { // lowers to addq
                        if (lhs.is_imm() && lhs.as_imm().is_int() &&
                            lhs.as_imm().as_int().fits_in(
                                8, lhs.as_imm().as_int().is_signed()))
                            break;

                        if (rhs.is_imm() && rhs.as_imm().is_int() &&
                            rhs.as_imm().as_int().fits_in(
                                8, rhs.as_imm().as_int().is_signed()))
                            break;
                    }
                }
                    [[fallthrough]];
                case IROp::ISub:
                case IROp::IMul:
                case IROp::SDiv:
                case IROp::UDiv:
                case IROp::SRem:
                case IROp::URem:
                case IROp::And:
                case IROp::Or:
                case IROp::Xor:
                case IROp::Shl:
                case IROp::Lsr:
                case IROp::Asr:
                    handle_with_imm_inst(inst, block.id);
                    break;
                case IROp::INeg:
                case IROp::FAdd:
                case IROp::FSub:
                case IROp::FMul:
                case IROp::FDiv:
                case IROp::FNeg:
                case IROp::ZExt:
                case IROp::SExt:
                case IROp::Trunc:
                case IROp::Not:
                    handle_all_operands(inst, block.id, inst.dest.value().type);
                    break;
                case IROp::ICmp:
                    handle_icmp(inst, block.id);
                    break;
                case IROp::FCmp:
                    handle_fcmp(inst, block.id);
                    break;
                case IROp::Store:
                    handle_store(inst, block.id);
                    break;
                case IROp::Call:
                    handle_call(inst, block.id, file);
                    break;
                case IROp::CallIndirect:
                    panic("TODO");
                    break;
                case IROp::Ret:
                    if (!inst.operands.empty())
                        intern(inst.operands.front(), inst, block.id,
                               func.return_type);
                    break;
                case IROp::Copy:
                    handle_copy(inst);
                    break;
                case IROp::Branch:
                    intern(inst.operands.front(), inst, block.id,
                           type::builtin::I1);
                    break;
                case IROp::Alloca:
                case IROp::Load:
                case IROp::LoadConst:
                case IROp::GetElementPtr:
                case IROp::ExtractElement:
                case IROp::InsertElement:
                case IROp::Jump:
                case IROp::Arg:
                case IROp::Phi:
                case IROp::ParallelCopy:
                case IROp::Dead:
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
