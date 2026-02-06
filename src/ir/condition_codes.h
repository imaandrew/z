#pragma once

#include <cstdint>

namespace z::ir {
enum class IntCC : std::uint8_t {
    Equal,
    NotEqual,
    UnsignedGreaterThan,
    UnsignedGreaterEqual,
    UnsignedLessThan,
    UnsignedLessEqual,
    SignedGreaterThan,
    SignedGreaterEqual,
    SignedLessThan,
    SignedLessEqual
};

enum class FloatCC : std::uint8_t {
    Equal,
    NotEqual,
    GreaterThan,
    GreaterEqual,
    LessThan,
    LessEqual,
};
} // namespace z::ir
