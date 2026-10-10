#include "builder.h"
#include "bytecode/byte_ops.h"
#include "bytecode/bytecode.h"
#include "core/panic.h"
#include "core/types.h"
#include "ir/condition_codes.h"
#include "ir/ir.h"
#include "ir/ir_builder.h"
#include "ir/ir_ops.h"
#include "type/type.h"
#include <utility>
#include <variant>
#include <vector>

namespace z::bytecode {
std::vector<Function> Builder::lower_file(const ir::IRFile& file) {
    for (const auto& func : file.funcs) {
        current_func = &funcs.emplace_back(func.name);
        lower_func(func);
    }

    return std::move(funcs);
}

void Builder::lower_func(const ir::IRFunction& func) {
    expect(func.num_regs() < 256, "func.num_regs() >= 256");

    for (const auto& block : func.blocks) {
        const auto chunk_id = current_func->get_chunks().size();
        chunk_map[block.id.id] = chunk_id;
        current_chunk = &current_func->add_chunk(chunk_id);
        lower_block(block, func);
    }
}

using ir::IROp;

void Builder::lower_block(const ir::BasicBlock& block,
                          const ir::IRFunction& func) {
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
            lower_int_logic(inst);
            break;
        case IROp::INeg:
            lower_ineg(inst);
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
                               lower_vreg(dest), 0, 0));
        }

        return;
    }

    if (lhs->is_reg() || rhs->is_reg()) {
        if (lhs->is_imm())
            std::swap(lhs, rhs);
        expect(rhs->is_imm(), "");

        const auto reg = lhs->as_reg();
        const auto& imm = rhs->as_imm().as_int();
        const auto* op_type =
            ctxt->ty->get_as<type::IntegerType>(lhs->as_reg().type);

        if (inst.op == IROp::IAdd && imm.get_width() <= 8) {
            emit_inst(Instruction::RI(
                op_type->is_signed() ? Op::SAddq : Op::UAddq, lower_vreg(dest),
                lower_vreg(reg), static_cast<i8>(imm.get_bits())));
            return;
        }

        if (reg.id == dest.id && imm.get_width() <= 16) {
            emit_inst(Instruction::I(imm_op(inst.op, op_type->is_signed()),
                                     lower_vreg(dest),
                                     static_cast<i16>(imm.get_bits())));
            return;
        }

        panic("ImmToReg should have moved the immediate into a register");
    }

    panic("unfolded arithmetic expression");
}

void Builder::lower_int_logic(const ir::Instruction& inst) {
    const auto reg_op = [](IROp op_, bool is_signed) {
        switch (op_) {
        case IROp::And:
            return Op::And;
        case IROp::Or:
            return Op::Or;
        case IROp::Xor:
            return Op::Xor;
        default:
            std::unreachable();
        }
    };

    const auto imm_op = [](IROp op_, bool is_signed) {
        switch (op_) {
        case IROp::And:
            return Op::Andi;
        case IROp::Or:
            return Op::Ori;
        case IROp::Xor:
            return Op::Xori;
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
        return;
    }

    if (lhs->is_reg() || rhs->is_reg()) {
        if (lhs->is_imm())
            std::swap(lhs, rhs);
        expect(rhs->is_imm(), "");

        const auto reg = lhs->as_reg();
        const auto& imm = rhs->as_imm().as_int();
        const auto* op_type =
            ctxt->ty->get_as<type::IntegerType>(lhs->as_reg().type);

        if (reg.id == dest.id && imm.get_width() <= 16) {
            emit_inst(Instruction::I(imm_op(inst.op, op_type->is_signed()),
                                     lower_vreg(dest),
                                     static_cast<i16>(imm.get_bits())));
            return;
        }

        panic("ImmToReg should have moved the immediate into a register");
    }

    panic("unfolded logical expression");
}

void Builder::lower_ineg(const ir::Instruction& inst) {
    expect(inst.operands.size() == 1, "ineg should have one operand");
    expect(inst.dest.has_value(), "ineg should have dest");

    const auto& v = inst.operands.front();
    const auto dest = inst.dest.value();
    expect(v.is_reg(), "ImmToReg should have moved immediate into register");

    emit_inst(
        Instruction::R(Op::Neg, lower_vreg(dest), lower_vreg(v.as_reg()), 0));
}

void Builder::lower_copy(const ir::Instruction& inst) {
    expect(inst.operands.size() == 1, "mov should have one operand");
    expect(inst.dest.has_value(), "mov should have dest");

    const auto& v = inst.operands.front();
    const auto dest = inst.dest.value();
    expect(v.is_reg(), "ImmToReg should have moved immediate into register");

    emit_inst(
        Instruction::R(Op::Mov, lower_vreg(dest), lower_vreg(v.as_reg()), 0));
}

void Builder::lower_load_const(const ir::Instruction& inst) {
    expect(inst.operands.size() == 1, "ldc should have one operand");
    expect(inst.dest.has_value(), "ldc should have dest");

    const auto& v = inst.operands.front();
    const auto dest = inst.dest.value();
    expect(v.is_imm() && v.as_imm().is_int(), "ldc operand should be integer");

    emit_inst(Instruction::I(Op::LoadConst, lower_vreg(dest),
                             static_cast<i16>(v.as_imm().as_int().get_bits())));
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
        case ir::IntCC::SignedLessThan:
            return Op::Lt;
        case ir::IntCC::UnsignedGreaterThan:
            std::swap(lhs, rhs);
        case ir::IntCC::UnsignedLessThan:
            return Op::LtU;
        case ir::IntCC::SignedGreaterEqual:
            std::swap(lhs, rhs);
        case ir::IntCC::SignedLessEqual:
            return Op::Le;
        case ir::IntCC::UnsignedGreaterEqual:
            std::swap(lhs, rhs);
        case ir::IntCC::UnsignedLessEqual:
            return Op::LeU;
        }
    };

    const auto get_float_op = [&lhs, &rhs](ir::FloatCC cc) {
        switch (cc) {
        case ir::FloatCC::Equal:
            return Op::Eq;
        case ir::FloatCC::NotEqual:
            return Op::Ne;
        case ir::FloatCC::GreaterThan:
            std::swap(lhs, rhs);
        case ir::FloatCC::LessThan:
            return Op::LtF;
        case ir::FloatCC::GreaterEqual:
            std::swap(lhs, rhs);
        case ir::FloatCC::LessEqual:
            return Op::LeF;
        }
    };

    emit_inst(Instruction::R(
        inst.op == ir::IROp::ICmp
            ? get_int_op(inst.operands.front().as_intcc())
            : get_float_op(inst.operands.front().as_floatcc()),
        lower_vreg(inst.dest.value()), lower_vreg(lhs), lower_vreg(rhs)));
}

void Builder::lower_branch(const ir::Instruction& inst) {
    const auto cond = inst.operands[0];
    const auto lhs = inst.operands[1].as_label();
    const auto rhs = inst.operands[2].as_label();

    if (cond.as_imm().is_bool())
        panic("TODO");

    pending_branches.emplace_back(current_chunk->get_idx(), cond.as_reg(),
                                  lhs.block_id, rhs.block_id);
    emit_inst(Instruction::B(Op::Bnz, lower_vreg(cond.as_reg()), 0));
}

void Builder::fixup_branches() {
    for (const auto& branch : pending_branches) {
    }
}
} // namespace z::bytecode
