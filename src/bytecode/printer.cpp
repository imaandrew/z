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

    const auto& info = op_info(inst.op());
    const auto& [dest, src1, src2] = info.operands.operands;

    bool first = true;
    const auto sep = [&] {
        if (!first)
            os << ", ";
        first = false;
    };
    const auto reg = [&](u8 r) {
        sep();
        print_reg(r);
    };
    const auto is_signed = [](OperandType t) {
        return t == OperandType::i8 || t == OperandType::i16 ||
               t == OperandType::i32 || t == OperandType::i64;
    };

    switch (info.i_class) {
    case InstClass::R: // op rd, rs, rt
        if (dest != OperandType::_)
            reg(inst.rd());
        if (src1 != OperandType::_)
            reg(inst.rs());
        if (src2 != OperandType::_)
            reg(inst.rt());
        break;
    case InstClass::RI:  // op rd, rs, i8
    case InstClass::RIU: // op rd, rs, u8
        reg(inst.rd());
        reg(inst.rs());
        sep();
        print_imm8(inst.imm8(), is_signed(src2));
        break;
    case InstClass::I:  // op rd, i16
    case InstClass::IU: // op rd, u16
    case InstClass::B:  // op rd, offset
        reg(inst.rd());
        sep();
        print_imm16(inst.imm16(), is_signed(src2));
        break;
    case InstClass::J: // op offset
        sep();
        print_offset(inst.offset24());
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

void Printer::print_offset(i32 offset) {
    std::print(os, "{}{}{}", colour::YELLOW, offset, colour::RESET);
}
} // namespace z::bytecode
