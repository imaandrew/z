#include "printer.h"
#include "bytecode/byte_ops.h"
#include "bytecode/bytecode.h"
#include "core/colour.h"
#include "core/types.h"
#include <algorithm>
#include <print>
#include <vector>

namespace z::bytecode {

void Printer::make_labels(const Function& func) {
    auto inst_ids = std::vector<i32>();
    const auto& insts = func.get_insts();
    for (usize i = 0; i < insts.size(); i++) {
        if (op_info(insts[i].op()).i_class == InstClass::B) {
            inst_ids.push_back(static_cast<i32>(i) + insts[i].imm16());
        } else if (op_info(insts[i].op()).i_class == InstClass::J) {
            inst_ids.push_back(static_cast<i32>(i) + insts[i].offset24());
        }
    }

    std::ranges::sort(inst_ids);
    labels = std::vector<i32>(func.get_insts().size(), -1);
    for (usize i = 0; i < inst_ids.size(); i++) {
        labels[inst_ids[i]] = static_cast<i32>(i);
    }
}

void Printer::print_file(const File& file) {
    this->file = &file;
    for (const auto& func : file.get_funcs()) {
        print_func(func);
    }
}
void Printer::print_func(const Function& func) {
    std::println(os, "{}{}:{}", colour::BOLD_YELLOW,
                 ctxt->strings->get_string(func.get_name()), colour::RESET);

    make_labels(func);
    for (usize i = 0; i < func.get_insts().size(); i++) {
        if (labels[i] != -1)
            std::println(os, ".L{}", labels[i]);
        print_inst(func.get_insts()[i], i);
    }

    os << "\n";
}

void Printer::print_inst(const Instruction& inst, usize id) {
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
        if (info.i_class == InstClass::B)
            std::print(os, ".L{}", labels[id + inst.imm16()]);
        else if (inst.op() == Op::Call)
            std::print(os, "{}",
                       ctxt->strings->get_string(
                           file->get_funcs()[inst.imm16u()].get_name()));
        else
            print_imm16(inst.imm16(), is_signed(src2));
        break;
    case InstClass::J: // op offset
        sep();
        std::print(os, ".L{}", labels[id + inst.offset24()]);
        break;
    }

    os << "\n";
}

void Printer::print_op(Op op) {
    std::print(os, "  {}{:8}{}", colour::MAGENTA, byte_op_to_string(op),
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
