#include "printer.h"
#include "bytecode/byte_ops.h"
#include "bytecode/bytecode.h"
#include "core/colour.h"
#include "core/types.h"
#include <print>

namespace z::bytecode {

void Printer::print_func(const Function& func) {
    std::println(os, "{}{}:{}", colour::BOLD_YELLOW,
                 ctxt->strings->get_string(func.get_name()), colour::RESET);

    for (const auto& chunk : func.get_chunks()) {
        print_chunk(chunk);
    }

    os << "\n";
}

void Printer::print_chunk(const Chunk& chunk) {
    for (const auto& inst : chunk.get_insts()) {
        print_inst(inst);
    }
}

void Printer::print_inst(const Instruction& inst) {
    print_op(inst.op());

    switch (inst.op()) {
    case Op::SAdd: // op rd, rs, rt
    case Op::UAdd:
    case Op::SSub:
    case Op::USub:
    case Op::SMul:
    case Op::UMul:
    case Op::SDiv:
    case Op::UDiv:
    case Op::SRem:
    case Op::URem:
        print_reg(inst.rd());
        os << ", ";
        print_reg(inst.rs());
        os << ", ";
        print_reg(inst.rt());
        break;
    case Op::Neg: // op rd, rs
        print_reg(inst.rd());
        os << ", ";
        print_reg(inst.rs());
        break;
    case Op::ChkS8: // op rd
    case Op::ChkU8:
    case Op::ChkS16:
    case Op::ChkU16:
    case Op::ChkS32:
    case Op::ChkU32:
        print_reg(inst.rd());
        break;
    case Op::SAddq: // op rd, i8
        print_reg(inst.rd());
        os << ", ";
        print_imm8(inst.imm8(), true);
        break;
    case Op::UAddq: // op rd, u8
        print_reg(inst.rd());
        os << ", ";
        print_imm8(inst.imm8(), false);
        break;
    case Op::SAddi: // op rd, i16
    case Op::SSubi:
    case Op::SMuli:
    case Op::SDivi:
    case Op::SRemi:
        print_reg(inst.rd());
        os << ", ";
        print_imm16(inst.imm16(), true);
        break;
    case Op::UAddi: // op rd, u16
    case Op::USubi:
    case Op::UMuli:
    case Op::UDivi:
    case Op::URemi:
    case Op::LoadConst:
        print_reg(inst.rd());
        os << ", ";
        print_imm16(inst.imm16(), false);
        break;
    }

    os << "\n";
}

void Printer::print_op(Op op) {
    std::print(os, "{}{:12}{}", colour::MAGENTA, byte_op_to_string(op),
               colour::RESET);
}

void Printer::print_reg(u8 reg) {
    std::print(os, "{}%{}{}", colour::CYAN, reg, colour::RESET);
}

void Printer::print_imm8(i8 imm, bool is_signed) {
    std::print(os, "{}{}{}", colour::YELLOW,
               is_signed ? imm : static_cast<u8>(imm), colour::RESET);
}

void Printer::print_imm16(i16 imm, bool is_signed) {
    std::print(os, "{}{}{}", colour::YELLOW,
               is_signed ? imm : static_cast<u16>(imm), colour::RESET);
}
} // namespace z::bytecode
