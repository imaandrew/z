#pragma once

#include "core/types.h"
#include "trap.h"
#include <expected>
#include <limits>

namespace z::vm::alu {

inline std::expected<u64, Error> sadd(u64 a, u64 b) {
    i64 res = 0;
    if (__builtin_add_overflow(static_cast<i64>(a), static_cast<i64>(b), &res))
        return std::unexpected(Error::IntegerOverflow);
    return static_cast<u64>(res);
}

inline std::expected<u64, Error> uadd(u64 a, u64 b) {
    u64 res = 0;
    if (__builtin_add_overflow(a, b, &res))
        return std::unexpected(Error::IntegerOverflow);
    return res;
}

inline std::expected<u64, Error> ssub(u64 a, u64 b) {
    i64 res = 0;
    if (__builtin_sub_overflow(static_cast<i64>(a), static_cast<i64>(b), &res))
        return std::unexpected(Error::IntegerOverflow);
    return static_cast<u64>(res);
}

inline std::expected<u64, Error> usub(u64 a, u64 b) {
    u64 res = 0;
    if (__builtin_sub_overflow(a, b, &res))
        return std::unexpected(Error::IntegerOverflow);
    return res;
}

inline std::expected<u64, Error> smul(u64 a, u64 b) {
    i64 res = 0;
    if (__builtin_mul_overflow(static_cast<i64>(a), static_cast<i64>(b), &res))
        return std::unexpected(Error::IntegerOverflow);
    return static_cast<u64>(res);
}

inline std::expected<u64, Error> umul(u64 a, u64 b) {
    u64 res = 0;
    if (__builtin_mul_overflow(a, b, &res))
        return std::unexpected(Error::IntegerOverflow);
    return res;
}

inline std::expected<u64, Error> sdiv(u64 a, u64 b) {
    if (b == 0)
        return std::unexpected(Error::DivisionByZero);

    const i64 a_signed = static_cast<i64>(a);
    const i64 b_signed = static_cast<i64>(b);
    if (a_signed == std::numeric_limits<i64>::min() && b_signed == -1)
        return std::unexpected(Error::IntegerOverflow);

    return static_cast<u64>(a_signed / b_signed);
}

inline std::expected<u64, Error> udiv(u64 a, u64 b) {
    if (b == 0)
        return std::unexpected(Error::DivisionByZero);

    return a / b;
}

inline std::expected<u64, Error> srem(u64 a, u64 b) {
    if (b == 0)
        return std::unexpected(Error::DivisionByZero);

    const i64 a_signed = static_cast<i64>(a);
    const i64 b_signed = static_cast<i64>(b);
    if (a_signed == std::numeric_limits<i64>::min() && b_signed == -1) {
        return std::unexpected(Error::IntegerOverflow);
    }

    return static_cast<u64>(a_signed % b_signed);
}

inline std::expected<u64, Error> urem(u64 a, u64 b) {
    if (b == 0)
        return std::unexpected(Error::DivisionByZero);

    return a % b;
}

inline std::expected<u64, Error> neg(u64 a) {
    const i64 a_signed = static_cast<i64>(a);
    if (a_signed == std::numeric_limits<i64>::min())
        return std::unexpected(Error::IntegerOverflow);

    return static_cast<u64>(-a_signed);
}

inline std::expected<u64, Error> sll(u64 a, u64 b) {
    if (b >= 64) {
        return std::unexpected(Error::ShiftTooLarge);
    }
    return a << b;
}

inline std::expected<u64, Error> srl(u64 a, u64 b) {
    if (b >= 64) {
        return std::unexpected(Error::ShiftTooLarge);
    }
    return a >> b;
}

inline std::expected<u64, Error> sra(u64 a, u64 b) {
    if (b >= 64) {
        return std::unexpected(Error::ShiftTooLarge);
    }
    // NOLINTNEXTLINE(bugprone-signed-bitwise)
    return static_cast<u64>(static_cast<i64>(a) >> b);
}
} // namespace z::vm::alu
