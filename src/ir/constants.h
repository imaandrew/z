#pragma once

#include "core/types.h"
#include "ir/condition_codes.h"
#include <cassert>
#include <limits>
#include <utility>
namespace z::ir {

class ConstInt {
    u64 bits;
    u8 width;
    bool is_signed;

    [[nodiscard]] u64 mask(u64 v) const {
        if (width == 64)
            return v;

        return v & ((1ULL << width) - 1);
    }

    [[nodiscard]] i64 sign_extend(u64 v) const {
        u64 const sign_bit = 1ULL << static_cast<u64>(width - 1);
        if ((v & sign_bit) != 0U) {
            assert(width < 64);
            return static_cast<i64>(v | ~((1ULL << width) - 1));
        }

        return static_cast<i64>(v);
    }

public:
    ConstInt(u64 bits, u8 width, bool is_signed)
        : bits(bits), width(width), is_signed(is_signed) {}

    [[nodiscard]] u64 get_bits() const { return bits; }

    [[nodiscard]] i64 get_signed() const { return sign_extend(bits); }

    [[nodiscard]] bool is_negative() const {
        if (is_signed) {
            u64 const sign_bit = 1ULL << static_cast<u64>(width - 1);
            return (bits & sign_bit) != 0U;
        }

        return false;
    }

    [[nodiscard]] ConstInt neg() const {
        return {mask(-bits), width, !is_signed};
    }

    [[nodiscard]] ConstInt add(const ConstInt& other) const {
        assert(width == other.width);
        return ConstInt(mask(bits + other.bits), width, is_signed);
    }

    [[nodiscard]] ConstInt add(const ConstInt& other, bool& overflow) const {
        auto result = add(other);

        if (result.is_signed) {
            auto rhs_is_negative = other.is_negative();
            auto result_lt_lhs = result.cmp(*this, IntCC::SignedLessThan);
            overflow = rhs_is_negative ^ result_lt_lhs;
        } else {
            overflow = result.cmp(*this, IntCC::UnsignedLessThan);
        }

        return result;
    }

    [[nodiscard]] ConstInt sub(const ConstInt& other) const {
        assert(width == other.width);
        return {mask(bits - other.bits), width, is_signed};
    }

    [[nodiscard]] ConstInt sub(const ConstInt& other, bool& overflow) const {
        auto result = sub(other);

        if (result.is_signed) {
            auto rhs_is_negative = other.is_negative();
            auto result_gt_lhs = result.cmp(*this, IntCC::SignedGreaterThan);
            overflow = rhs_is_negative ^ result_gt_lhs;
        } else {
            overflow = result.cmp(*this, IntCC::UnsignedLessThan);
        }

        return result;
    }

    [[nodiscard]] ConstInt mul(const ConstInt& other) const {
        assert(width == other.width);
        return {mask(bits * other.bits), width, is_signed};
    }

    [[nodiscard]] ConstInt mul(const ConstInt& other, bool& overflow) const {
        auto result = mul(other);

        if (result.is_signed) {
            if (bits != 0) {
                auto lhs_signed = sign_extend(bits);
                auto rhs_signed = sign_extend(other.bits);
                auto result_signed = sign_extend(result.bits);

                if (lhs_signed == -1) {
                    overflow = false;
                } else {
                    overflow = (result_signed / lhs_signed) != rhs_signed;
                }
            } else {
                overflow = false;
            }
        } else {
            if (bits != 0) {
                overflow = result.udiv(*this).bits != other.bits;
            } else {
                overflow = false;
            }
        }

        return result;
    }

    [[nodiscard]] ConstInt udiv(const ConstInt& other) const {
        assert(width == other.width);
        assert(other.bits != 0);
        return {mask(bits / other.bits), width, is_signed};
    }

    [[nodiscard]] ConstInt sdiv(const ConstInt& other) const {
        assert(width == other.width);
        assert(other.bits != 0);

        auto a = sign_extend(bits);
        auto b = sign_extend(other.bits);

        return {mask(static_cast<u64>(a / b)), width, is_signed};
    }

    [[nodiscard]] ConstInt sdiv(const ConstInt& other, bool& overflow) const {
        auto result = sdiv(other);

        auto lhs_signed = sign_extend(bits);
        auto rhs_signed = sign_extend(bits);

        i64 const min_val =
            -static_cast<i64>(1ULL << static_cast<u64>(width - 1));
        overflow = lhs_signed == min_val && rhs_signed == -1;

        return result;
    }

    [[nodiscard]] ConstInt urem(const ConstInt& other) const {
        assert(width == other.width);
        assert(other.bits != 0);

        return {mask(bits % other.bits), width, is_signed};
    }

    [[nodiscard]] ConstInt srem(const ConstInt& other) const {
        assert(width == other.width);
        assert(other.bits != 0);

        auto a = sign_extend(bits);
        auto b = sign_extend(other.bits);

        if (a == std::numeric_limits<i64>::min() && b == -1)
            return ConstInt{0, width, is_signed};

        return {mask(static_cast<u64>(a % b)), width, is_signed};
    }

    [[nodiscard]] ConstInt shl(const ConstInt& other) const {
        assert(width == other.width);

        return {mask(bits << other.bits), width, is_signed};
    }

    [[nodiscard]] ConstInt lshr(const ConstInt& other) const {
        assert(width == other.width);

        return {mask(bits >> other.bits), width, is_signed};
    }

    [[nodiscard]] ConstInt ashr(const ConstInt& other) const {
        assert(width == other.width);

        return {mask(static_cast<u64>(sign_extend(bits)) >> other.bits), width,
                is_signed};
    }

    [[nodiscard]] ConstInt bit_not() const {
        return ConstInt{mask(~bits), width, is_signed};
    }

    [[nodiscard]] ConstInt bit_and(const ConstInt& other) const {
        assert(width == other.width);
        return ConstInt{mask(bits & other.bits), width, is_signed};
    }

    [[nodiscard]] ConstInt bit_or(const ConstInt& other) const {
        assert(width == other.width);
        return ConstInt{mask(bits | other.bits), width, is_signed};
    }

    [[nodiscard]] ConstInt bit_xor(const ConstInt& other) const {
        assert(width == other.width);
        return ConstInt{mask(bits ^ other.bits), width, is_signed};
    }

    [[nodiscard]] bool cmp(const ConstInt& other, IntCC cc) const {
        assert(width == other.width);

        const auto lhs_signed = sign_extend(bits);
        const auto rhs_signed = sign_extend(other.bits);

        switch (cc) {
        case IntCC::Equal:
            return bits == other.bits;
        case IntCC::NotEqual:
            return bits != other.bits;
        case IntCC::UnsignedGreaterThan:
            return bits > other.bits;
        case IntCC::UnsignedGreaterEqual:
            return bits >= other.bits;
        case IntCC::UnsignedLessThan:
            return bits < other.bits;
        case IntCC::UnsignedLessEqual:
            return bits <= other.bits;
        case IntCC::SignedGreaterThan:
            return lhs_signed > rhs_signed;
        case IntCC::SignedGreaterEqual:
            return lhs_signed >= rhs_signed;
        case IntCC::SignedLessThan:
            return lhs_signed < rhs_signed;
        case IntCC::SignedLessEqual:
            return lhs_signed <= rhs_signed;
        }

        std::unreachable();
    }

    [[nodiscard]] bool cmp_imm(i64 other, IntCC cc) const {

        const auto lhs_signed = sign_extend(bits);
        const auto rhs_bits = static_cast<u64>(other);

        switch (cc) {
        case IntCC::Equal:
            return bits == rhs_bits;
        case IntCC::NotEqual:
            return bits != rhs_bits;
        case IntCC::UnsignedGreaterThan:
            return bits > rhs_bits;
        case IntCC::UnsignedGreaterEqual:
            return bits >= rhs_bits;
        case IntCC::UnsignedLessThan:
            return bits < rhs_bits;
        case IntCC::UnsignedLessEqual:
            return bits <= rhs_bits;
        case IntCC::SignedGreaterThan:
            return lhs_signed > other;
        case IntCC::SignedGreaterEqual:
            return lhs_signed >= other;
        case IntCC::SignedLessThan:
            return lhs_signed < other;
        case IntCC::SignedLessEqual:
            return lhs_signed <= other;
        }

        std::unreachable();
    }
};

class ConstFloat {
    double bits;
    u8 width;

public:
    ConstFloat(double bits, u8 width) : bits(bits), width(width) {}

    [[nodiscard]] double get_bits() const { return bits; }

    [[nodiscard]] ConstFloat neg() const { return {-bits, width}; }

    [[nodiscard]] ConstFloat add(const ConstFloat& other) const {
        assert(width == other.width);
        return {bits + other.bits, width};
    }

    [[nodiscard]] ConstFloat sub(const ConstFloat& other) const {
        assert(width == other.width);
        return {bits - other.bits, width};
    }

    [[nodiscard]] ConstFloat mul(const ConstFloat& other) const {
        assert(width == other.width);
        return {bits * other.bits, width};
    }

    [[nodiscard]] ConstFloat div(const ConstFloat& other) const {
        assert(width == other.width);
        return {bits / other.bits, width};
    }

    [[nodiscard]] bool cmp(const ConstFloat& other, FloatCC cc) const {
        assert(width == other.width);

        switch (cc) {
        case FloatCC::Equal:
            return bits == other.bits;
        case FloatCC::NotEqual:
            return bits != other.bits;
        case FloatCC::GreaterThan:
            return bits > other.bits;
        case FloatCC::GreaterEqual:
            return bits >= other.bits;
        case FloatCC::LessThan:
            return bits < other.bits;
        case FloatCC::LessEqual:
            return bits <= other.bits;
        }

        std::unreachable();
    }
};
} // namespace z::ir
