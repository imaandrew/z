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

        if (r.value() != ExitType::NoReturn) {
            switch (r.value()) {
            case ExitType::RetVoid:
                return 0;
            case ExitType::Ret:
                return static_cast<i64>(regs[0]);
            case ExitType::NoReturn:
                std::unreachable();
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

    auto exit_type = ExitType::NoReturn;

    const auto trap = [&](Error err) {
        return std::unexpected(Trap(inst.op(), old_pc, cur_func, err));
    };

    switch (inst.op()) {
    case Op::SAdd:
        if (const auto v = alu::sadd(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::UAdd:
        if (const auto v = alu::uadd(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::SAddi:
        if (const auto v = alu::sadd(read_reg(rd), imm16)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::UAddi:
        if (const auto v = alu::uadd(read_reg(rd), imm16u)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::SAddq:
        if (const auto v = alu::sadd(read_reg(rs), imm8)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::UAddq:
        if (const auto v = alu::uadd(read_reg(rs), imm8u)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::SSub:
        if (const auto v = alu::ssub(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::USub:
        if (const auto v = alu::usub(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::SSubi:
        if (const auto v = alu::ssub(read_reg(rd), imm16)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::USubi:
        if (const auto v = alu::usub(read_reg(rd), imm16u)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::SMul:
        if (const auto v = alu::smul(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::UMul:
        if (const auto v = alu::umul(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::SMuli:
        if (const auto v = alu::smul(read_reg(rd), imm16)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::UMuli:
        if (const auto v = alu::umul(read_reg(rd), imm16u)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::SDiv:
        if (const auto v = alu::sdiv(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::UDiv:
        if (const auto v = alu::udiv(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::SDivi:
        if (const auto v = alu::sdiv(read_reg(rd), imm16)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::UDivi:
        if (const auto v = alu::udiv(read_reg(rd), imm16u)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::SRem:
        if (const auto v = alu::srem(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::URem:
        if (const auto v = alu::urem(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::SRemi:
        if (const auto v = alu::srem(read_reg(rd), imm16)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::URemi:
        if (const auto v = alu::urem(read_reg(rd), imm16u)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::Neg:
        if (const auto v = alu::neg(read_reg(rs))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
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
        if (const auto v = alu::sll(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::Shli:
        if (const auto v = alu::sll(read_reg(rd), imm16u)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::Lsr:
        if (const auto v = alu::srl(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::Lsri:
        if (const auto v = alu::srl(read_reg(rd), imm16u)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::Asr:
        if (const auto v = alu::sra(read_reg(rs), read_reg(rt))) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::Asri:
        if (const auto v = alu::sra(read_reg(rd), imm16u)) [[likely]] {
            write_reg(rd, v.value());
        } else {
            return trap(v.error());
        }
        break;
    case Op::ChkS8:
        if (!std::in_range<i8>(static_cast<i64>(read_reg(rs))))
            return trap(Error::IntegerOutOfRange);
        break;
    case Op::ChkU8:
        if (!std::in_range<u8>(read_reg(rs)))
            return trap(Error::IntegerOutOfRange);
        break;
    case Op::ChkS16:
        if (!std::in_range<i16>(static_cast<i64>(read_reg(rs))))
            return trap(Error::IntegerOutOfRange);
        break;
    case Op::ChkU16:
        if (!std::in_range<u16>(read_reg(rs)))
            return trap(Error::IntegerOutOfRange);
        break;
    case Op::ChkS32:
        if (!std::in_range<i32>(static_cast<i64>(read_reg(rs))))
            return trap(Error::IntegerOutOfRange);
        break;
    case Op::ChkU32:
        if (!std::in_range<u32>(read_reg(rs)))
            return trap(Error::IntegerOutOfRange);
        break;
    case Op::LoadConst:
        if (auto c = cur_func->get_constant(imm16u)) {
            write_reg(rd, c.value());
        } else {
            return trap(Error::ConstIdxOutOfBounds);
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
        exit_type = ExitType::NoReturn;
        break;
    }
    }

    return exit_type;
}

u64 VM::read_reg(u8 idx) { return regs[base + idx]; }

void VM::write_reg(u8 idx, u64 val) { regs[base + idx] = val; }
} // namespace z::vm
