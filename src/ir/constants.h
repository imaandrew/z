#pragma once

#include "core/panic.h"
#include "core/types.h"
#include "ir/condition_codes.h"
#include <cassert>
#include <limits>
#include <utility>
namespace z::ir {

inline u64 mask_val(u64 value, u64 width) {
    if (width == 64)
        return value;

    return value & ((1ULL << width) - 1);
}

class ConstInt {
    u64 bits;
    u8 width;
    bool is_signed_;

    [[nodiscard]] u64 mask(u64 v) const {
        if (width == 64)
            return v;

        return v & ((1ULL << width) - 1);
    }

    [[nodiscard]] i64 sign_extend(u64 v) const {
        u64 const sign_bit = 1ULL << static_cast<u64>(width - 1);
        if ((v & sign_bit) != 0U && width < 64) {
            return static_cast<i64>(v | ~((1ULL << width) - 1));
        }

        return static_cast<i64>(v);
    }

    [[nodiscard]] ConstInt add(const ConstInt& other) const {
        ASSERT(width == other.width);
        ASSERT(is_signed() == other.is_signed());
        return ConstInt(mask(bits + other.bits), width, is_signed_);
    }

    [[nodiscard]] ConstInt sub(const ConstInt& other) const {
        ASSERT(width == other.width);
        ASSERT(is_signed() == other.is_signed());
        return {mask(bits - other.bits), width, is_signed_};
    }

    [[nodiscard]] ConstInt mul(const ConstInt& other) const {
        ASSERT(width == other.width);
        ASSERT(is_signed() == other.is_signed());
        return {mask(bits * other.bits), width, is_signed_};
    }

    [[nodiscard]] ConstInt sdiv(const ConstInt& other) const {
        ASSERT(width == other.width);
        ASSERT(is_signed() == other.is_signed());

        auto a = sign_extend(bits);
        auto b = sign_extend(other.bits);

        return {mask(static_cast<u64>(a / b)), width, is_signed_};
    }

public:
    ConstInt(u64 bits, u8 width, bool is_signed)
        : bits(mask_val(bits, width)), width(width), is_signed_(is_signed) {}

    [[nodiscard]] u64 get_bits() const { return bits; }

    [[nodiscard]] i64 get_signed() const { return sign_extend(bits); }

    [[nodiscard]] bool is_signed() const { return is_signed_; }

    [[nodiscard]] u8 get_width() const { return width; }

    [[nodiscard]] bool is_negative() const {
        if (is_signed_) {
            u64 const sign_bit = 1ULL << static_cast<u64>(width - 1);
            return (bits & sign_bit) != 0U;
        }

        return false;
    }

    [[nodiscard]] bool fits_in(u8 new_width, bool new_signed) const {
        if (new_width == 64) {
            if (new_signed)
                return is_signed() || (bits >> 63ULL) == 0;
            return !is_negative();
        }

        if (!new_signed) {
            const u64 max = (1ULL << new_width) - 1;
            if (is_signed()) {
                const auto val = get_signed();
                return std::cmp_greater_equal(val, 0) &&
                       std::cmp_less_equal(val, max);
            }
            return bits <= max;
        }

        const i64 max = static_cast<i64>(1ULL << (new_width - 1ULL)) - 1;
        const i64 min = -static_cast<i64>(1ULL << (new_width - 1ULL));

        if (is_signed()) {
            const i64 val = get_signed();
            return val >= min && val <= max;
        }

        return std::cmp_greater_equal(bits, min) &&
               std::cmp_less_equal(bits, max);
    }

    [[nodiscard]] ConstInt zext(u8 new_width, bool new_signed) const {
        ASSERT(new_width >= width);
        return {bits, new_width, new_signed};
    }

    [[nodiscard]] ConstInt sext(u8 new_width, bool new_signed) const {
        ASSERT(new_width >= width);
        return {mask_val(static_cast<u64>(sign_extend(bits)), new_width),
                new_width, new_signed};
    }

    [[nodiscard]] ConstInt trunc(u8 new_width, bool new_signed) const {
        ASSERT(new_width <= width);
        return {mask_val(bits, new_width), new_width, new_signed};
    }

    [[nodiscard]] ConstInt cast_to(u8 new_width, bool new_signed) const {
        if (new_width < width)
            return trunc(new_width, new_signed);
        if (new_width > width)
            return new_signed ? sext(new_width, new_signed)
                              : zext(new_width, new_signed);

        return {bits, new_width, new_signed};
    }

    [[nodiscard]] ConstInt neg(bool& overflow, bool& undefined) const {
        undefined = !is_signed();
        overflow = bits == 1ULL << (width - 1ULL);
        return {mask(-bits), width, is_signed_};
    }

    [[nodiscard]] ConstInt add(const ConstInt& other, bool& overflow) const {
        auto result = add(other);

        if (result.is_signed_) {
            auto rhs_is_negative = other.is_negative();
            auto result_lt_lhs = result.cmp(*this, IntCC::SignedLessThan);
            overflow = rhs_is_negative ^ result_lt_lhs;
        } else {
            overflow = result.cmp(*this, IntCC::UnsignedLessThan);
        }

        return result;
    }

    [[nodiscard]] ConstInt sub(const ConstInt& other, bool& overflow) const {
        auto result = sub(other);

        if (result.is_signed_) {
            auto rhs_is_negative = other.is_negative();
            auto result_gt_lhs = result.cmp(*this, IntCC::SignedGreaterThan);
            overflow = rhs_is_negative ^ result_gt_lhs;
        } else {
            overflow = other.cmp(*this, IntCC::UnsignedGreaterThan);
        }

        return result;
    }

    [[nodiscard]] ConstInt mul(const ConstInt& other, bool& overflow) const {
        auto result = mul(other);
        overflow = false;

        if (result.is_signed_) {
            if (bits != 0) {
                auto lhs_signed = sign_extend(bits);
                auto rhs_signed = sign_extend(other.bits);
                auto result_signed = sign_extend(result.bits);

                if (lhs_signed != -1) {
                    overflow = (result_signed / lhs_signed) != rhs_signed;
                }
            }
        } else if (bits != 0) {
            bool undefined = false;
            overflow = (result.udiv(*this, undefined).bits != other.bits);
            expect(!undefined,
                   "only undefined if denom == 0 but denom bits != 0");
        }

        return result;
    }

    [[nodiscard]] ConstInt udiv(const ConstInt& other, bool& undefined) const {
        ASSERT(width == other.width);
        ASSERT(is_signed() == other.is_signed());

        undefined = other.bits == 0;
        if (undefined)
            return {0, width, is_signed_};

        return {mask(bits / other.bits), width, is_signed_};
    }

    [[nodiscard]] ConstInt sdiv(const ConstInt& other, bool& overflow,
                                bool& undefined) const {
        undefined = other.bits == 0;
        if (undefined)
            return {0, width, is_signed_};

        auto lhs_signed = sign_extend(bits);
        auto rhs_signed = sign_extend(other.bits);

        i64 const min_val =
            -static_cast<i64>(1ULL << static_cast<u64>(width - 1));

        overflow = lhs_signed == min_val && rhs_signed == -1;
        if (overflow)
            return {0, width, is_signed_};

        auto result = sdiv(other);

        return result;
    }

    [[nodiscard]] ConstInt urem(const ConstInt& other, bool& undefined) const {
        ASSERT(width == other.width);
        ASSERT(is_signed() == other.is_signed());

        undefined = other.bits == 0;
        if (undefined)
            return {0, width, is_signed_};

        return {mask(bits % other.bits), width, is_signed_};
    }

    [[nodiscard]] ConstInt srem(const ConstInt& other, bool& undefined) const {
        ASSERT(width == other.width);
        ASSERT(is_signed() == other.is_signed());

        undefined = other.bits == 0;
        if (undefined)
            return {0, width, is_signed_};

        auto a = sign_extend(bits);
        auto b = sign_extend(other.bits);

        if (a == std::numeric_limits<i64>::min() && b == -1)
            return {0, width, is_signed_};

        return {mask(static_cast<u64>(a % b)), width, is_signed_};
    }

    [[nodiscard]] ConstInt shl(const ConstInt& other, bool& undefined) const {
        ASSERT(width == other.width);

        undefined = other.bits >= width || other.is_negative();
        if (undefined)
            return {0, width, is_signed_};

        return {mask(bits << other.bits), width, is_signed_};
    }

    [[nodiscard]] ConstInt lshr(const ConstInt& other, bool& undefined) const {
        ASSERT(width == other.width);

        undefined = other.bits >= width || other.is_negative();
        if (undefined)
            return {0, width, is_signed_};

        return {mask(bits >> other.bits), width, is_signed_};
    }

    [[nodiscard]] ConstInt ashr(const ConstInt& other, bool& undefined) const {
        ASSERT(width == other.width);

        undefined = other.bits >= width || other.is_negative();
        if (undefined)
            return {0, width, is_signed_};

        return {mask(static_cast<u64>(sign_extend(bits)) >> other.bits), width,
                is_signed_};
    }

    [[nodiscard]] ConstInt bit_not() const {
        return ConstInt{mask(~bits), width, is_signed_};
    }

    [[nodiscard]] ConstInt bit_and(const ConstInt& other) const {
        ASSERT(width == other.width);
        ASSERT(is_signed() == other.is_signed());
        return ConstInt{mask(bits & other.bits), width, is_signed_};
    }

    [[nodiscard]] ConstInt bit_or(const ConstInt& other) const {
        ASSERT(width == other.width);
        ASSERT(is_signed() == other.is_signed());
        return ConstInt{mask(bits | other.bits), width, is_signed_};
    }

    [[nodiscard]] ConstInt bit_xor(const ConstInt& other) const {
        ASSERT(width == other.width);
        ASSERT(is_signed() == other.is_signed());
        return ConstInt{mask(bits ^ other.bits), width, is_signed_};
    }

    [[nodiscard]] bool cmp(const ConstInt& other, IntCC cc) const {
        ASSERT(width == other.width);
        ASSERT(is_signed() == other.is_signed());

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
        const auto rhs_bits = mask_val(static_cast<u64>(other), width);

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

    [[nodiscard]] u8 get_width() const { return width; }

    [[nodiscard]] bool fits_in(u8 new_width) const {
        if (new_width == 32)
            return static_cast<float>(bits) == bits;

        if (new_width == 64)
            return static_cast<double>(bits) == bits;

        panic("ConstFloat: invalid width: {}", new_width);
    }

    [[nodiscard]] ConstFloat cast_to(u8 new_width) const {
        if (width == new_width)
            return *this;

        if (new_width == 32) {
            ASSERT(width == 64);
            return {static_cast<float>(bits), new_width};
        }
        if (new_width == 64) {
            ASSERT(width == 32);
            return {bits, new_width};
        }
        panic("ConstFloat: invalid width: {}", new_width);
    }

    [[nodiscard]] ConstFloat neg() const { return {-bits, width}; }

    [[nodiscard]] ConstFloat add(const ConstFloat& other) const {
        ASSERT(width == other.width);
        return {bits + other.bits, width};
    }

    [[nodiscard]] ConstFloat sub(const ConstFloat& other) const {
        ASSERT(width == other.width);
        return {bits - other.bits, width};
    }

    [[nodiscard]] ConstFloat mul(const ConstFloat& other) const {
        ASSERT(width == other.width);
        return {bits * other.bits, width};
    }

    [[nodiscard]] ConstFloat div(const ConstFloat& other,
                                 bool& undefined) const {
        ASSERT(width == other.width);
        undefined = other.bits == 0;
        if (undefined)
            return {0, width};
        return {bits / other.bits, width};
    }

    [[nodiscard]] bool cmp(const ConstFloat& other, FloatCC cc) const {
        ASSERT(width == other.width);

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
