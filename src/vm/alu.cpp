#include "vm/alu.h"
#include "core/types.h"
#include <cstdint>

namespace z::vm {
constexpr u64 ALU::checked_addi(u64 a, u64 b, bool is_signed, bool& overflow) {
    if (is_signed) {
        i64 res = 0;
        overflow = __builtin_add_overflow(static_cast<i64>(a),
                                          static_cast<i64>(b), &res);
        return static_cast<u64>(res);
    }

    u64 res = 0;
    overflow = __builtin_add_overflow(a, b, &res);
    return res;
}

constexpr u64 ALU::checked_subi(u64 a, u64 b, bool is_signed, bool& overflow) {
    if (is_signed) {
        i64 res = 0;
        overflow = __builtin_sub_overflow(static_cast<i64>(a),
                                          static_cast<i64>(b), &res);
        return static_cast<u64>(res);
    }

    u64 res = 0;
    overflow = __builtin_sub_overflow(a, b, &res);

    return res;
}

constexpr u64 ALU::checked_muli(u64 a, u64 b, bool is_signed, bool& overflow) {
    if (is_signed) {
        i64 res = 0;
        overflow = __builtin_mul_overflow(static_cast<i64>(a),
                                          static_cast<i64>(b), &res);
        return static_cast<u64>(res);
    }

    u64 res = 0;
    overflow = __builtin_mul_overflow(a, b, &res);
    return res;
}

constexpr u64 ALU::checked_divi(u64 a, u64 b, bool is_signed, bool& overflow) {
    u64 res = 0;
    overflow = false;
    if (is_signed) {
        const i64 a_signed = static_cast<i64>(a);
        const i64 b_signed = static_cast<i64>(b);
        if (a_signed == INT64_MIN && b_signed == -1) {
            overflow = true;
            return -1;
        }
        res = static_cast<u64>(a_signed / b_signed);
    } else {
        res = a / b;
    }

    return res;
}

constexpr u64 ALU::checked_remi(u64 a, u64 b, bool is_signed, bool& overflow) {
    u64 res = 0;
    overflow = false;
    if (is_signed) {
        const i64 a_signed = static_cast<i64>(a);
        const i64 b_signed = static_cast<i64>(b);
        if (a_signed == INT64_MIN && b_signed == -1) {
            overflow = true;
            return -1;
        }
        res = static_cast<u64>(a_signed % b_signed);
    } else {
        res = a % b;
    }

    return res;
}

u64 ALU::iadd(u64 a, u64 b, bool is_signed, bool& overflow) {
    return checked_addi(a, b, is_signed, overflow);
}

u64 ALU::isub(u64 a, u64 b, bool is_signed, bool& overflow) {
    return checked_subi(a, b, is_signed, overflow);
}

u64 ALU::imul(u64 a, u64 b, bool is_signed, bool& overflow) {
    return checked_muli(a, b, is_signed, overflow);
}

u64 ALU::idiv(u64 a, u64 b, bool is_signed, bool& overflow, bool& undefined) {
    overflow = false;
    if (b == 0) {
        undefined = true;
        return -1;
    }
    return checked_divi(a, b, is_signed, overflow);
}

u64 ALU::irem(u64 a, u64 b, bool is_signed, bool& overflow, bool& undefined) {
    overflow = false;
    if (b == 0) {
        undefined = true;
        return -1;
    }
    return checked_remi(a, b, is_signed, overflow);
}

u64 ALU::ineg(u64 a, bool& overflow) {
    const auto a_ = static_cast<i64>(a);
    if (a_ == INT64_MIN) {
        overflow = true;
        return -1;
    }
    return static_cast<u64>(-a_);
}

u64 ALU::sll(u64 a, u64 b, bool& undefined) {
    if (b >= 64) {
        undefined = true;
        return -1;
    }

    return a << b;
}

u64 ALU::srl(u64 a, u64 b, bool& undefined) {
    if (b >= 64) {
        undefined = true;
        return -1;
    }

    return a >> b;
}
u64 ALU::sra(u64 a, u64 b, bool& undefined) {
    if (b >= 64) {
        undefined = true;
        return -1;
    }

    return static_cast<u64>(static_cast<i64>(a) >> b);
}
} // namespace z::vm
