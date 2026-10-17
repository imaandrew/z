#include "vm/vm.h"
#include "bytecode/byte_ops.h"
#include "core/colour.h"
#include "core/types.h"
#include "vm/alu.h"
#include "vm/trap.h"
#include <bit>
#include <expected>
#include <iostream>
#include <optional>
#include <print>
#include <utility>

namespace z::vm {
using namespace bytecode;

i64 VM::run() {
    while (true) {
        const auto r = exec_inst();
        if (!r) {
            report(r.error());
            return 1;
        }

        if (frames.empty()) {
            switch (r.value()) {
            case ExitType::RetVoid:
                return 0;
            case ExitType::Ret:
                return static_cast<i64>(regs[0]);
            case ExitType::NoReturn:
                break;
            }
        }
    }
}

void VM::report(const Trap& t) {
    std::println(std::cerr, "{}error:{} {}", colour::RED, colour::RESET,
                 err_to_string(t.err));
    std::println(std::cerr, "faulting instruction: OP: {}, FUNC: {}, PC: {}",
                 byte_op_to_string(t.op), t.func->get_name(), t.pc);
}

std::expected<VM::ExitType, Trap> VM::exec_inst() {
    const u64 old_pc = pc;
    const auto& inst = cur_func->get_insts()[pc++];
    const auto rd = inst.rd();
    const auto rs = inst.rs();
    const auto rt = inst.rt();
    const auto imm16 = static_cast<u64>(static_cast<i64>(inst.imm16()));
    const auto imm16u = static_cast<u64>(inst.imm16u());
    const auto imm8 = static_cast<u64>(static_cast<i64>(inst.imm8()));
    const auto imm8u = static_cast<u64>(inst.imm8u());
    bool overflow = false;
    bool div_zero = false;
    bool shift_undefined = false;
    bool int_out_of_range = false;
    bool const_idx_oob = false;

    auto exit_type = ExitType::NoReturn;

    switch (inst.op()) {
    case Op::SAdd:
        write_reg(rd, ALU::iadd(read_reg(rs), read_reg(rt), true, overflow));
        break;
    case Op::UAdd:
        write_reg(rd, ALU::iadd(read_reg(rs), read_reg(rt), false, overflow));
        break;
    case Op::SAddi:
        write_reg(rd, ALU::iadd(read_reg(rd), imm16, true, overflow));
        break;
    case Op::UAddi:
        write_reg(rd, ALU::iadd(read_reg(rd), imm16u, false, overflow));
        break;
    case Op::SAddq:
        write_reg(rd, ALU::iadd(read_reg(rs), imm8, true, overflow));
        break;
    case Op::UAddq:
        write_reg(rd, ALU::iadd(read_reg(rs), imm8u, false, overflow));
        break;
    case Op::SSub:
        write_reg(rd, ALU::isub(read_reg(rs), read_reg(rt), true, overflow));
        break;
    case Op::USub:
        write_reg(rd, ALU::isub(read_reg(rs), read_reg(rt), false, overflow));
        break;
    case Op::SSubi:
        write_reg(rd, ALU::isub(read_reg(rd), imm16, true, overflow));
        break;
    case Op::USubi:
        write_reg(rd, ALU::isub(read_reg(rd), imm16u, false, overflow));
        break;
    case Op::SMul:
        write_reg(rd, ALU::imul(read_reg(rs), read_reg(rt), true, overflow));
        break;
    case Op::UMul:
        write_reg(rd, ALU::imul(read_reg(rs), read_reg(rt), false, overflow));
        break;
    case Op::SMuli:
        write_reg(rd, ALU::imul(read_reg(rd), imm16, true, overflow));
        break;
    case Op::UMuli:
        write_reg(rd, ALU::imul(read_reg(rd), imm16u, false, overflow));
        break;
    case Op::SDiv:
        write_reg(rd, ALU::idiv(read_reg(rs), read_reg(rt), true, overflow,
                                div_zero));
        break;
    case Op::UDiv:
        write_reg(rd, ALU::idiv(read_reg(rs), read_reg(rt), false, overflow,
                                div_zero));
        break;
    case Op::SDivi:
        write_reg(rd, ALU::idiv(read_reg(rd), imm16, true, overflow, div_zero));
        break;
    case Op::UDivi:
        write_reg(rd,
                  ALU::idiv(read_reg(rd), imm16u, false, overflow, div_zero));
        break;
    case Op::SRem:
        write_reg(rd, ALU::irem(read_reg(rs), read_reg(rt), true, overflow,
                                div_zero));
        break;
    case Op::URem:
        write_reg(rd, ALU::irem(read_reg(rs), read_reg(rt), false, overflow,
                                div_zero));
        break;
    case Op::SRemi:
        write_reg(rd, ALU::irem(read_reg(rd), imm16, true, overflow, div_zero));
        break;
    case Op::URemi:
        write_reg(rd,
                  ALU::irem(read_reg(rd), imm16u, false, overflow, div_zero));
        break;
    case Op::Neg:
        write_reg(rd, ALU::ineg(read_reg(rs), overflow));
        break;
    case Op::And:
        write_reg(rd, read_reg(rs) & read_reg(rt));
        break;
    case Op::Andi:
        write_reg(rd, read_reg(rd) & imm16u);
        break;
    case Op::Or:
        write_reg(rd, read_reg(rs) | read_reg(rt));
        break;
    case Op::Ori:
        write_reg(rd, read_reg(rd) | imm16u);
        break;
    case Op::Xor:
        write_reg(rd, read_reg(rs) ^ read_reg(rt));
        break;
    case Op::Xori:
        write_reg(rd, read_reg(rd) ^ imm16u);
        break;
    case Op::Not:
        write_reg(rd, ~read_reg(rs));
        break;
    case Op::Notu:
        write_reg(rd, ~read_reg(rs) & ((1ULL << imm8u) - 1));
        break;
    case Op::Shl:
        write_reg(rd, ALU::sll(read_reg(rs), read_reg(rt), shift_undefined));
        break;
    case Op::Shli:
        write_reg(rd, ALU::sll(read_reg(rd), imm16u, shift_undefined));
        break;
    case Op::Lsr:
        write_reg(rd, ALU::srl(read_reg(rs), read_reg(rt), shift_undefined));
        break;
    case Op::Lsri:
        write_reg(rd, ALU::srl(read_reg(rd), imm16u, shift_undefined));
        break;
    case Op::Asr:
        write_reg(rd, ALU::sra(read_reg(rs), read_reg(rt), shift_undefined));
        break;
    case Op::Asri:
        write_reg(rd, ALU::sra(read_reg(rd), imm16u, shift_undefined));
        break;
    case Op::ChkS8:
        int_out_of_range = !std::in_range<i8>(static_cast<i64>(read_reg(rs)));
        break;
    case Op::ChkU8:
        int_out_of_range = !std::in_range<u8>(read_reg(rs));
        break;
    case Op::ChkS16:
        int_out_of_range = !std::in_range<i16>(static_cast<i64>(read_reg(rs)));
        break;
    case Op::ChkU16:
        int_out_of_range = !std::in_range<u16>(read_reg(rs));
        break;
    case Op::ChkS32:
        int_out_of_range = !std::in_range<i32>(static_cast<i64>(read_reg(rs)));
        break;
    case Op::ChkU32:
        int_out_of_range = !std::in_range<u32>(read_reg(rs));
        break;
    case Op::LoadConst:
        if (auto c = cur_func->get_constant(imm16u)) {
            write_reg(rd, c.value());
        } else {
            const_idx_oob = true;
        }
        break;
    case Op::LoadImm:
        write_reg(rd, imm16);
        break;
    case Op::Mov:
        write_reg(rd, read_reg(rs));
        break;
    case Op::Eq:
        write_reg(rd, read_reg(rs) == read_reg(rt));
        break;
    case Op::EqF:
        write_reg(rd, static_cast<u64>(std::bit_cast<double>(read_reg(rs)) ==
                                       std::bit_cast<double>(read_reg(rt))));
        break;
    case Op::Ne:
        write_reg(rd, read_reg(rs) != read_reg(rt));
        break;
    case Op::NeF:
        write_reg(rd, static_cast<u64>(std::bit_cast<double>(read_reg(rs)) !=
                                       std::bit_cast<double>(read_reg(rt))));
        break;
    case Op::Lt:
        write_reg(rd, static_cast<u64>(static_cast<i64>(read_reg(rs)) <
                                       static_cast<i64>(read_reg(rt))));
        break;
    case Op::Le:
        write_reg(rd, static_cast<u64>(static_cast<i64>(read_reg(rs)) <=
                                       static_cast<i64>(read_reg(rt))));
        break;
    case Op::LtU:
        write_reg(rd, read_reg(rs) < read_reg(rt));
        break;
    case Op::LeU:
        write_reg(rd, read_reg(rs) <= read_reg(rt));
        break;
    case Op::LtF:
        write_reg(rd, static_cast<u64>(std::bit_cast<double>(read_reg(rs)) <
                                       std::bit_cast<double>(read_reg(rt))));
        break;
    case Op::LeF:
        write_reg(rd, static_cast<u64>(std::bit_cast<double>(read_reg(rs)) <=
                                       std::bit_cast<double>(read_reg(rt))));
        break;
    case Op::Bnz:
        if (read_reg(rd) != 0) {
            pc = old_pc + inst.imm16();
        }
        break;
    case Op::Bz:
        if (read_reg(rd) == 0) {
            pc = old_pc + inst.imm16();
        }
        break;
    case Op::Jump:
        pc = old_pc + inst.offset24();
        break;
    case Op::Call:
        frames.emplace_back(cur_func, pc, base);
        base += rd;
        cur_func = &file.get_funcs()[imm16u];
        pc = 0;
        ensure_stack(base + cur_func->get_nregs());
        break;
    case Op::Ret:
        write_reg(0, read_reg(rs));
        exit_type = ExitType::Ret;
        [[fallthrough]];
    case Op::RetVoid: {
        if (exit_type == ExitType::NoReturn)
            exit_type = ExitType::RetVoid;
        if (frames.empty())
            return exit_type;
        const auto f = frames.back();
        frames.pop_back();
        cur_func = f.func;
        pc = f.return_pc;
        base = f.base;
        break;
    }
    }

    const auto err = [&](Error err) {
        return std::unexpected(Trap(inst.op(), old_pc, cur_func, err));
    };

    if (div_zero) {
        return err(Error::DivisionByZero);
    }

    if (shift_undefined) {
        return err(Error::ShiftTooLarge);
    }

    if (overflow) {
        return err(Error::IntegerOverflow);
    }

    if (int_out_of_range) {
        return err(Error::IntegerOutOfRange);
    }

    if (const_idx_oob) {
        return err(Error::ConstIdxOutOfBounds);
    }

    return exit_type;
}

u64 VM::read_reg(u8 idx) { return regs[base + idx]; }

void VM::write_reg(u8 idx, u64 val) { regs[base + idx] = val; }
} // namespace z::vm
