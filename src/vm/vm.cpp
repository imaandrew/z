#include "vm/vm.h"
#include "bytecode/byte_ops.h"
#include "bytecode/bytecode.h"
#include "core/colour.h"
#include "core/types.h"
#include "vm/alu.h"
#include "vm/trap.h"
#include <bit>
#include <expected>
#include <iostream>
#include <print>
#include <utility>

namespace z::vm {
using namespace bytecode;

#ifdef __has_cpp_attribute
#if __has_cpp_attribute(clang::musttail)
#define TAILCALL [[clang::musttail]]
#elif __has_cpp_attribute(gnu::musttail)
#define TAILCALL [[gnu::musttail]]
#endif
#endif

#ifndef TAILCALL
#define TAILCALL
#warning compiler does not support musttail
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISPATCH_IP(ip)                                                        \
    /* NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index,          \
     * cppcoreguidelines-pro-bounds-pointer-arithmetic) */                     \
    TAILCALL return vm.table[std::to_underlying((ip)->op())](vm, ip, r);       \
    /* NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index,            \
     * cppcoreguidelines-pro-bounds-pointer-arithmetic) */

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISPATCH() DISPATCH_IP(ip + 1)

namespace {
template <typename T> constexpr u64 S64(T x) {
    return static_cast<u64>(static_cast<i64>(x));
}

template <typename T> constexpr u64 U64(T x) { return static_cast<u64>(x); }
} // namespace

i64 VM::run() {
    const auto* ip = cur_func->get_insts().data();
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    const auto status =
        table[std::to_underlying(ip->op())](*this, ip, regs.data());
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

    switch (status) {
    case Status::Halt:
        return static_cast<i64>(regs[0]);
    case Status::Trap:
        report(trap_.value());
        return 1;
    }

    std::unreachable();
}

void VM::report(const Trap& t) {
    std::println(std::cerr, "{}error:{} {}", colour::RED, colour::RESET,
                 err_to_string(t.err));
    std::println(
        std::cerr, "faulting instruction: OP: {}, FUNC: {}, PC: 0x{:0x}",
        byte_op_to_string(t.inst->op()), strings.get_string(t.func->get_name()),
        t.inst - t.func->get_insts().data());
}
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)

PRESERVE_NONE VM::Status VM::op_SAdd(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::sadd(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_UAdd(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::uadd(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_SAddi(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (const auto v = alu::sadd(r[ip->rd()], S64(ip->imm16()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_UAddi(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (const auto v = alu::uadd(r[ip->rd()], U64(ip->imm16u()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_SAddq(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (const auto v = alu::sadd(r[ip->rs()], S64(ip->imm8()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_UAddq(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (const auto v = alu::uadd(r[ip->rs()], U64(ip->imm8u()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_SSub(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::ssub(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_USub(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::usub(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_SSubi(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (const auto v = alu::ssub(r[ip->rd()], S64(ip->imm16()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_USubi(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (const auto v = alu::usub(r[ip->rd()], U64(ip->imm16u()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_SMul(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::smul(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_UMul(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::umul(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_SMuli(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (const auto v = alu::smul(r[ip->rd()], S64(ip->imm16()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_UMuli(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (const auto v = alu::umul(r[ip->rd()], U64(ip->imm16u()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_SDiv(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::sdiv(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_UDiv(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::udiv(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_SDivi(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (const auto v = alu::sdiv(r[ip->rd()], S64(ip->imm16()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_UDivi(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (const auto v = alu::udiv(r[ip->rd()], U64(ip->imm16u()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_SRem(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::srem(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_URem(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::urem(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_SRemi(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (const auto v = alu::srem(r[ip->rd()], S64(ip->imm16()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_URemi(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (const auto v = alu::urem(r[ip->rd()], U64(ip->imm16u()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Neg(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    if (const auto v = alu::neg(r[ip->rs()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_And(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    r[ip->rd()] = r[ip->rs()] & r[ip->rt()];
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Andi(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {

    r[ip->rd()] = r[ip->rd()] & U64(ip->imm16u());
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Or(VM& vm, const bytecode::Instruction* ip,
                                   u64* r) {
    r[ip->rd()] = r[ip->rs()] | r[ip->rt()];
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Ori(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    r[ip->rd()] = r[ip->rd()] | U64(ip->imm16u());
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Xor(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    r[ip->rd()] = r[ip->rs()] ^ r[ip->rt()];
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Xori(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    r[ip->rd()] = r[ip->rd()] ^ U64(ip->imm16u());
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Not(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    r[ip->rd()] = ~r[ip->rs()];
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Notu(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    r[ip->rd()] = ~r[ip->rs()] & ((1ULL << ip->imm8u()) - 1);
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Shl(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    if (const auto v = alu::sll(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Shli(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::sll(r[ip->rd()], U64(ip->imm16u()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Lsr(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    if (const auto v = alu::srl(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Lsri(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::srl(r[ip->rd()], U64(ip->imm16u()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Asr(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    if (const auto v = alu::sra(r[ip->rs()], r[ip->rt()])) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Asri(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (const auto v = alu::sra(r[ip->rd()], U64(ip->imm16u()))) [[likely]] {
        r[ip->rd()] = v.value();
    } else {
        return vm.trap(v.error(), ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_ChkS8(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (!std::in_range<i8>(static_cast<i64>(r[ip->rs()])))
        return vm.trap(Error::IntegerOutOfRange, ip);

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_ChkU8(VM& vm, const bytecode::Instruction* ip,
                                      u64* r) {
    if (!std::in_range<u8>(r[ip->rs()]))
        return vm.trap(Error::IntegerOutOfRange, ip);

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_ChkS16(VM& vm, const bytecode::Instruction* ip,
                                       u64* r) {
    if (!std::in_range<i16>(static_cast<i64>(r[ip->rs()])))
        return vm.trap(Error::IntegerOutOfRange, ip);

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_ChkU16(VM& vm, const bytecode::Instruction* ip,
                                       u64* r) {
    if (!std::in_range<u16>(r[ip->rs()]))
        return vm.trap(Error::IntegerOutOfRange, ip);

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_ChkS32(VM& vm, const bytecode::Instruction* ip,
                                       u64* r) {
    if (!std::in_range<i32>(static_cast<i64>(r[ip->rs()])))
        return vm.trap(Error::IntegerOutOfRange, ip);

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_ChkU32(VM& vm, const bytecode::Instruction* ip,
                                       u64* r) {
    if (!std::in_range<u32>(r[ip->rs()]))
        return vm.trap(Error::IntegerOutOfRange, ip);

    DISPATCH();
}

PRESERVE_NONE VM::Status
VM::op_LoadConst(VM& vm, const bytecode::Instruction* ip, u64* r) {
    if (auto c = vm.cur_func->get_constant(ip->imm16u())) {
        r[ip->rd()] = c.value();
    } else {
        return vm.trap(Error::ConstIdxOutOfBounds, ip);
    }

    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_LoadImm(VM& vm, const bytecode::Instruction* ip,
                                        u64* r) {
    r[ip->rd()] = S64(ip->imm16());
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Mov(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    r[ip->rd()] = r[ip->rs()];
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Eq(VM& vm, const bytecode::Instruction* ip,
                                   u64* r) {
    r[ip->rd()] = r[ip->rs()] == r[ip->rt()];
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_EqF(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    r[ip->rd()] = U64(std::bit_cast<double>(r[ip->rs()]) ==
                      std::bit_cast<double>(r[ip->rt()]));
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Ne(VM& vm, const bytecode::Instruction* ip,
                                   u64* r) {
    r[ip->rd()] = r[ip->rs()] != r[ip->rt()];
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_NeF(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    r[ip->rd()] = U64(std::bit_cast<double>(r[ip->rs()]) !=
                      std::bit_cast<double>(r[ip->rt()]));
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Lt(VM& vm, const bytecode::Instruction* ip,
                                   u64* r) {
    r[ip->rd()] =
        U64(static_cast<i64>(r[ip->rs()]) < static_cast<i64>(r[ip->rt()]));
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Le(VM& vm, const bytecode::Instruction* ip,
                                   u64* r) {
    r[ip->rd()] =
        U64(static_cast<i64>(r[ip->rs()]) <= static_cast<i64>(r[ip->rt()]));
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_LtU(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    r[ip->rd()] = r[ip->rs()] < r[ip->rt()];
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_LeU(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    r[ip->rd()] = r[ip->rs()] <= r[ip->rt()];
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_LtF(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    r[ip->rd()] = U64(std::bit_cast<double>(r[ip->rs()]) <
                      std::bit_cast<double>(r[ip->rt()]));
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_LeF(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    r[ip->rd()] = U64(std::bit_cast<double>(r[ip->rs()]) <=
                      std::bit_cast<double>(r[ip->rt()]));
    DISPATCH();
}

PRESERVE_NONE VM::Status VM::op_Bnz(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    if (r[ip->rd()] != 0)
        ip += ip->imm16();
    else
        ip++;
    DISPATCH_IP(ip);
}

PRESERVE_NONE VM::Status VM::op_Bz(VM& vm, const bytecode::Instruction* ip,
                                   u64* r) {
    if (r[ip->rd()] == 0)
        ip += ip->imm16();
    else
        ip++;
    DISPATCH_IP(ip);
}

PRESERVE_NONE VM::Status VM::op_Jump(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    DISPATCH_IP(ip + ip->offset24());
}

PRESERVE_NONE VM::Status VM::op_Call(VM& vm, const bytecode::Instruction* ip,
                                     u64* r) {
    if (r + ip->rd() + vm.file.get_funcs()[ip->imm16u()].get_nregs() >
        vm.regs.data() + z::vm::VM::RegStack::size()) [[unlikely]]
        return vm.trap(Error::StackOverflow, ip);
    vm.call(ip + 1, r, ip->imm16u());
    r += ip->rd();
    DISPATCH_IP(vm.cur_func->get_insts().data());
}

PRESERVE_NONE VM::Status VM::op_Ret(VM& vm, const bytecode::Instruction* ip,
                                    u64* r) {
    r[0] = r[ip->rs()];
    if (vm.frames.empty())
        return VM::Status::Halt;
    r = vm.frames.back().base;
    ip = vm.frames.back().return_ip;
    vm.pop_frame();
    DISPATCH_IP(ip);
}

PRESERVE_NONE VM::Status VM::op_RetVoid(VM& vm, const bytecode::Instruction* ip,
                                        u64* r) {
    if (vm.frames.empty()) {
        r[0] = 0;
        return VM::Status::Halt;
    }
    r = vm.frames.back().base;
    ip = vm.frames.back().return_ip;
    vm.pop_frame();
    DISPATCH_IP(ip);
}

PRESERVE_NONE VM::Status VM::op_invalid(VM& vm, const bytecode::Instruction* ip,
                                        u64* /*r*/) {
    return vm.trap(Error::InvalidOp, ip);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
} // namespace z::vm
