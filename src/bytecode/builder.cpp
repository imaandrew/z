#include "builder.h"
#include "bytecode/byte_ops.h"
#include "bytecode/bytecode.h"
#include "core/panic.h"
#include "core/types.h"
#include "ir/analysis/order.h"
#include "ir/condition_codes.h"
#include "ir/ir.h"
#include "ir/ir_ops.h"
#include "type/type.h"
#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace z::bytecode {
std::vector<Function> Builder::lower_file(const ir::IRFile& file) {
    for (const auto& func : file.funcs) {
        lower_func(func);
    }

    return std::move(funcs);
}

void Builder::lower_func(const ir::IRFunction& func) {
    expect(func.num_regs() < 256, "func.num_regs() >= 256");
    expect(func.id.id == funcs.size(), "function id should match order");
    current_func = &funcs.emplace_back(func.name, func.const_pool.values());
    chunk_map = std::vector<u32>(func.blocks.size());
    arg_index = 0;
    current_func->set_nregs(func.num_regs());
    layout = ir::InstOrder(func).rev_postorder();

    for (usize i = 0; i < layout.size(); i++) {
        chunk_map[layout[i].id] = i;
    }

    for (const auto block_id : layout) {
        const auto& block = func.get_block(block_id);
        lower_block(block, func);
    }

    fixup_branches();
}

using ir::IROp;

void Builder::lower_block(const ir::BasicBlock& block,
                          const ir::IRFunction& func) {
    const auto chunk_id = chunk_map[block.id.id];
    current_chunk = &current_func->add_chunk(chunk_id);

    for (const auto inst_id : block.insts) {
        const auto& inst = func.get_inst(inst_id);
        switch (inst.op) {
        case IROp::IAdd:
        case IROp::ISub:
        case IROp::IMul:
        case IROp::SDiv:
        case IROp::UDiv:
        case IROp::SRem:
        case IROp::URem:
            lower_int_arith(inst);
            break;
        case IROp::And:
        case IROp::Or:
        case IROp::Xor:
        case IROp::Shl:
        case IROp::Lsr:
        case IROp::Asr:
            lower_int_logic(inst);
            break;
        case IROp::INeg:
            lower_r_rd(inst);
            break;
        case IROp::Not:
            lower_not(inst);
            break;
        case IROp::Copy:
            lower_copy(inst);
            break;
        case IROp::LoadConst:
            lower_load_const(inst);
            break;
        case IROp::ICmp:
        case IROp::FCmp:
            lower_cmp(inst);
            break;
        case IROp::Call:
            lower_call(inst, func.num_regs());
            break;
        case IROp::Ret:
            lower_ret(inst);
            break;
        case IROp::Arg: {
            const auto k = arg_index++;
            const auto dest = inst.dest.value();
            expect(k < func.params.size() && func.params[k].id == dest.id,
                   "arg {} doesn't match param list", k);
            expect(lower_vreg(dest) == k, "arg {} must lower to r{}, got r{}",
                   k, k, lower_vreg(dest));
            break;
        }
        case IROp::Branch:
            lower_branch(inst);
            break;
        case IROp::Jump:
            lower_jump(inst);
            break;
        default:
            panic("unimplemented op: {}", ir::ir_op_to_string(inst.op));
        }
    }
}

void Builder::lower_int_arith(const ir::Instruction& inst) {
    const auto chk = [](u32 width, bool is_signed) {
        switch (width) {
        case 8:
            return is_signed ? Op::ChkS8 : Op::ChkU8;
        case 16:
            return is_signed ? Op::ChkS16 : Op::ChkU16;
        case 32:
            return is_signed ? Op::ChkS32 : Op::ChkU32;
        default:
            std::unreachable();
        }
    };

    const auto reg_op = [](IROp op_, bool is_signed) {
        switch (op_) {
        case IROp::IAdd:
            return is_signed ? Op::SAdd : Op::UAdd;
        case IROp::ISub:
            return is_signed ? Op::SSub : Op::USub;
        case IROp::IMul:
            return is_signed ? Op::SMul : Op::UMul;
        case IROp::SDiv:
            return Op::SDiv;
        case IROp::UDiv:
            return Op::UDiv;
        case IROp::SRem:
            return Op::SRem;
        case IROp::URem:
            return Op::URem;
        default:
            std::unreachable();
        }
    };

    const auto imm_op = [](IROp op_, bool is_signed) {
        switch (op_) {
        case IROp::IAdd:
            return is_signed ? Op::SAddi : Op::UAddi;
        case IROp::ISub:
            return is_signed ? Op::SSubi : Op::USubi;
        case IROp::IMul:
            return is_signed ? Op::SMuli : Op::UMuli;
        case IROp::SDiv:
            return Op::SDivi;
        case IROp::UDiv:
            return Op::UDivi;
        case IROp::SRem:
            return Op::SRemi;
        case IROp::URem:
            return Op::URemi;
        default:
            std::unreachable();
        }
    };

    expect(inst.operands.size() == 2, "");
    expect(inst.dest.has_value(), "");

    const auto* lhs = inst.operands.data();
    const auto* rhs = &inst.operands[1];
    const auto dest = inst.dest.value();

    if (lhs->is_reg() && rhs->is_reg()) {
        const auto* op_type =
            ctxt->ty->get_as<type::IntegerType>(lhs->as_reg().type);

        emit_inst(Instruction::R(reg_op(inst.op, op_type->is_signed()),
                                 lower_vreg(dest), lower_vreg(lhs->as_reg()),
                                 lower_vreg(rhs->as_reg())));

        if (op_type->get_width() != 64) {
            emit_inst(
                Instruction::R(chk(op_type->get_width(), op_type->is_signed()),
                               0, lower_vreg(dest), 0));
        }

        return;
    }

    if (!lhs->is_reg() && !rhs->is_reg())
        panic("ImmToReg should have moved immediate into register");

    if (lhs->is_imm() && ir::is_commutative(inst.op))
        std::swap(lhs, rhs);
    expect(rhs->is_imm(), "ImmToReg should have moved immediate into register");

    const auto reg = lhs->as_reg();
    const auto& imm = rhs->as_imm().as_int();
    const auto* op_type =
        ctxt->ty->get_as<type::IntegerType>(lhs->as_reg().type);

    if (inst.op == IROp::IAdd && imm.fits_in(8, op_type->is_signed())) {
        if (op_type->is_signed()) {
            emit_inst(Instruction::RI(Op::SAddq, lower_vreg(dest),
                                      lower_vreg(reg),
                                      static_cast<i8>(imm.get_signed())));
        } else {
            emit_inst(Instruction::RIU(Op::UAddq, lower_vreg(dest),
                                       lower_vreg(reg),
                                       static_cast<u8>(imm.get_bits())));
        }

        if (op_type->get_width() != 64) {
            emit_inst(
                Instruction::R(chk(op_type->get_width(), op_type->is_signed()),
                               0, lower_vreg(dest), 0));
        }
        return;
    }

    if (imm.fits_in(16, op_type->is_signed())) {
        if (reg.id != dest.id)
            emit_inst(
                Instruction::R(Op::Mov, lower_vreg(dest), lower_vreg(reg), 0));

        if (op_type->is_signed())
            emit_inst(Instruction::I(imm_op(inst.op, true), lower_vreg(dest),
                                     static_cast<i16>(imm.get_signed())));
        else
            emit_inst(Instruction::IU(imm_op(inst.op, false), lower_vreg(dest),
                                      static_cast<u16>(imm.get_bits())));

        if (op_type->get_width() != 64) {
            emit_inst(
                Instruction::R(chk(op_type->get_width(), op_type->is_signed()),
                               0, lower_vreg(dest), 0));
        }
    } else {
        panic("ImmToReg should have moved the immediate into a register");
    }
}

void Builder::lower_int_logic(const ir::Instruction& inst) {
    const auto reg_op = [](IROp op_) {
        switch (op_) {
        case IROp::And:
            return Op::And;
        case IROp::Or:
            return Op::Or;
        case IROp::Xor:
            return Op::Xor;
        case IROp::Shl:
            return Op::Shl;
        case IROp::Lsr:
            return Op::Lsr;
        case IROp::Asr:
            return Op::Asr;
        default:
            std::unreachable();
        }
    };

    const auto imm_op = [](IROp op_) {
        switch (op_) {
        case IROp::And:
            return Op::Andi;
        case IROp::Or:
            return Op::Ori;
        case IROp::Xor:
            return Op::Xori;
        case IROp::Shl:
            return Op::Shli;
        case IROp::Lsr:
            return Op::Lsri;
        case IROp::Asr:
            return Op::Asri;
        default:
            std::unreachable();
        }
    };

    expect(inst.operands.size() == 2, "");
    expect(inst.dest.has_value(), "");

    const auto* lhs = inst.operands.data();
    const auto* rhs = &inst.operands[1];
    const auto dest = inst.dest.value();

    if (lhs->is_reg() && rhs->is_reg()) {
        emit_inst(Instruction::R(reg_op(inst.op), lower_vreg(dest),
                                 lower_vreg(lhs->as_reg()),
                                 lower_vreg(rhs->as_reg())));
        return;
    }

    if (!lhs->is_reg() && !rhs->is_reg())
        panic("ImmToReg should have moved immediate into register");

    if (lhs->is_imm() && ir::is_commutative(inst.op))
        std::swap(lhs, rhs);
    expect(rhs->is_imm(), "ImmToReg should have moved immediate into register");

    const auto reg = lhs->as_reg();
    const auto& imm = rhs->as_imm().as_int();

    if (imm.fits_in(16, false)) {
        if (reg.id != dest.id)
            emit_inst(
                Instruction::R(Op::Mov, lower_vreg(dest), lower_vreg(reg), 0));

        emit_inst(Instruction::IU(imm_op(inst.op), lower_vreg(dest),
                                  static_cast<u16>(imm.get_bits())));
    } else {
        panic("ImmToReg should have moved immediate into register");
    }
}

void Builder::lower_not(const ir::Instruction& inst) {
    expect(inst.operands.size() == 1, "not should have one operand");
    expect(inst.dest.has_value(), "not should have dest");

    const auto& val = inst.operands.front();
    expect(val.is_reg(), "ImmToReg should have moved immediate into register");

    const auto* type = ctxt->ty->get_as<type::IntegerType>(val.as_reg().type);
    switch (type->get_width()) {
    case 1:
    case 8:
    case 16:
    case 32:
        if (!type->is_signed()) {
            emit_inst(Instruction::RIU(Op::Notu, lower_vreg(inst.dest.value()),
                                       lower_vreg(val.as_reg()),
                                       type->get_width()));
            return;
        }
        [[fallthrough]];
    case 64:
        emit_inst(Instruction::R(Op::Not, lower_vreg(inst.dest.value()),
                                 lower_vreg(val.as_reg()), 0));
        break;
    default:
        std::unreachable();
    }
}

void Builder::lower_r_rd(const ir::Instruction& inst) {
    expect(inst.operands.size() == 1, "instruction should have one operand");
    expect(inst.dest.has_value(), "instruction should have dest");

    const auto op = [](IROp op_) {
        switch (op_) {
        case IROp::INeg:
            return Op::Neg;
        default:
            std::unreachable();
        }
    };

    const auto& v = inst.operands.front();
    const auto dest = inst.dest.value();
    expect(v.is_reg(), "ImmToReg should have moved immediate into register");

    emit_inst(Instruction::R(op(inst.op), lower_vreg(dest),
                             lower_vreg(v.as_reg()), 0));
}

void Builder::lower_copy(const ir::Instruction& inst) {
    expect(inst.operands.size() == 1, "mov should have one operand");
    expect(inst.dest.has_value(), "mov should have dest");

    const auto& v = inst.operands.front();
    const auto dest = inst.dest.value();

    if (v.is_imm()) {
        expect(v.as_imm().is_int() && v.as_imm().as_int().fits_in(16, true),
               "ImmToReg should have moved to reg");
        emit_inst(
            Instruction::I(Op::LoadImm, lower_vreg(dest),
                           static_cast<i16>(v.as_imm().as_int().get_signed())));
        return;
    }

    emit_inst(
        Instruction::R(Op::Mov, lower_vreg(dest), lower_vreg(v.as_reg()), 0));
}

void Builder::lower_load_const(const ir::Instruction& inst) {
    expect(inst.operands.size() == 1, "ldc should have one operand");
    expect(inst.dest.has_value(), "ldc should have dest");

    const auto& v = inst.operands.front();
    const auto dest = inst.dest.value();
    expect(v.is_imm() && v.as_imm().is_int(), "ldc operand should be integer");

    emit_inst(
        Instruction::IU(Op::LoadConst, lower_vreg(dest),
                        static_cast<u16>(v.as_imm().as_int().get_bits())));
}

void Builder::lower_cmp(const ir::Instruction& inst) {
    auto lhs = inst.operands[1].as_reg();
    auto rhs = inst.operands[2].as_reg();

    const auto get_int_op = [&lhs, &rhs](ir::IntCC cc) {
        switch (cc) {
        case ir::IntCC::Equal:
            return Op::Eq;
        case ir::IntCC::NotEqual:
            return Op::Ne;
        case ir::IntCC::SignedGreaterThan:
            std::swap(lhs, rhs);
            [[fallthrough]];
        case ir::IntCC::SignedLessThan:
            return Op::Lt;
        case ir::IntCC::UnsignedGreaterThan:
            std::swap(lhs, rhs);
            [[fallthrough]];
        case ir::IntCC::UnsignedLessThan:
            return Op::LtU;
        case ir::IntCC::SignedGreaterEqual:
            std::swap(lhs, rhs);
            [[fallthrough]];
        case ir::IntCC::SignedLessEqual:
            return Op::Le;
        case ir::IntCC::UnsignedGreaterEqual:
            std::swap(lhs, rhs);
            [[fallthrough]];
        case ir::IntCC::UnsignedLessEqual:
            return Op::LeU;
        }

        std::unreachable();
    };

    const auto get_float_op = [&lhs, &rhs](ir::FloatCC cc) {
        switch (cc) {
        case ir::FloatCC::Equal:
            return Op::EqF;
        case ir::FloatCC::NotEqual:
            return Op::NeF;
        case ir::FloatCC::GreaterThan:
            std::swap(lhs, rhs);
            [[fallthrough]];
        case ir::FloatCC::LessThan:
            return Op::LtF;
        case ir::FloatCC::GreaterEqual:
            std::swap(lhs, rhs);
            [[fallthrough]];
        case ir::FloatCC::LessEqual:
            return Op::LeF;
        }

        std::unreachable();
    };

    const auto op = inst.op == ir::IROp::ICmp
                        ? get_int_op(inst.operands.front().as_intcc())
                        : get_float_op(inst.operands.front().as_floatcc());
    emit_inst(Instruction::R(op, lower_vreg(inst.dest.value()), lower_vreg(lhs),
                             lower_vreg(rhs)));
}

void Builder::lower_branch(const ir::Instruction& inst) {
    const auto cond = inst.operands[0];
    const auto true_chunk = chunk_map[inst.operands[1].as_label().block_id.id];
    const auto false_chunk = chunk_map[inst.operands[2].as_label().block_id.id];

    if (cond.is_imm()) {
        const auto b = cond.as_imm().as_int();
        expect(b.get_width() == 1, "branch condition should be bool");
        const bool bool_cond = static_cast<bool>(b.get_bits());
        if (bool_cond && current_chunk->get_idx() + 1 != true_chunk) {
            emit_jump(true_chunk);
        } else if (!bool_cond && current_chunk->get_idx() + 1 != false_chunk) {
            emit_jump(false_chunk);
        }
        return;
    }

    if (current_chunk->get_idx() + 1 == true_chunk) {
        emit_branch(Op::Bz, cond.as_reg(), false_chunk);
    } else if (current_chunk->get_idx() + 1 == false_chunk) {
        emit_branch(Op::Bnz, cond.as_reg(), true_chunk);
    } else {
        emit_branch(Op::Bnz, cond.as_reg(), true_chunk);
        emit_jump(false_chunk);
    }
}

void Builder::lower_jump(const ir::Instruction& inst) {
    const auto target = chunk_map[inst.operands[0].as_label().block_id.id];

    if (current_chunk->get_idx() + 1 != target)
        emit_jump(target);
}

void Builder::lower_call(const ir::Instruction& inst, u32 num_regs) {
    const auto func_id = inst.operands[0].as_func().id;
    const u32 base = num_regs;
    const u32 nargs = inst.operands.size() - 1;

    expect(base + std::max(nargs, 1U) - 1 <= UINT8_MAX,
           "frame setup exceeds register bank");
    for (usize i = 0; i < nargs; i++) {
        const auto& op = inst.operands[i + 1];
        expect(op.is_reg(), "ImmToReg should have moved immediate to register");
        emit_inst(Instruction::R(Op::Mov, static_cast<u8>(base + i),
                                 lower_vreg(op.as_reg()), 0));
    }

    const auto nregs = current_func->get_nregs();
    current_func->set_nregs(std::max(nregs, base + std::max(nargs, 1U)));

    expect(func_id <= UINT16_MAX, "func id cannot fit in u16");
    emit_inst(Instruction::IU(Op::Call, static_cast<u8>(base),
                              static_cast<u16>(func_id)));

    if (inst.dest.has_value())
        emit_inst(Instruction::R(Op::Mov, lower_vreg(inst.dest.value()),
                                 static_cast<u8>(base), 0));
}

void Builder::lower_ret(const ir::Instruction& inst) {
    if (inst.operands.empty()) {
        emit_inst(Instruction::R(Op::RetVoid, 0, 0, 0));
        return;
    }

    const auto& ret_val = inst.operands[0];
    expect(ret_val.is_reg(), "IRBuilder forces return val to be vreg");

    emit_inst(Instruction::R(Op::Ret, 0, lower_vreg(ret_val.as_reg()), 0));
}

void Builder::fixup_branches() {
    auto& chunks = current_func->get_chunks();
    auto chunk_insts = std::vector<u32>(chunks.size());

    const auto calc_branch_offset = [&](u32 branch_chunk, u32 branch_inst,
                                        i64 target_chunk) {
        return static_cast<i64>(chunk_insts[target_chunk]) -
               static_cast<i64>(chunk_insts[branch_chunk] + branch_inst);
    };

    for (usize i = 1; i < chunks.size(); i++) {
        chunk_insts[i] = chunk_insts[i - 1] + chunks[i - 1].get_insts().size();
    }

    for (const auto [chunk_idx, inst_idx] : pending_branches) {
        auto& insts = chunks[chunk_idx].get_insts();
        auto& inst = insts[inst_idx];
        const auto target_chunk = inst.imm16();
        const auto branch_offset =
            calc_branch_offset(chunk_idx, inst_idx, target_chunk);
        expect(branch_offset <= INT16_MAX && branch_offset >= INT16_MIN,
               "oob branch");
        inst = Instruction::B(inst.op(), inst.rd(),
                              static_cast<i16>(branch_offset));
    }

    for (const auto [chunk_idx, inst_idx] : pending_jumps) {
        auto& insts = chunks[chunk_idx].get_insts();
        auto& inst = insts[inst_idx];
        const auto target_chunk = inst.offset24();
        const auto jump_offset =
            calc_branch_offset(chunk_idx, inst_idx, target_chunk);
        inst = Instruction::J(inst.op(), static_cast<i32>(jump_offset));
    }
    pending_branches.clear();
    pending_jumps.clear();
}
} // namespace z::bytecode
