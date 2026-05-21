#pragma once

#include "core/types.h"

namespace z::ir {
enum class IntCC : u8 {
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

enum class FloatCC : u8 {
    Equal,
    NotEqual,
    GreaterThan,
    GreaterEqual,
    LessThan,
    LessEqual,
};
} // namespace z::ir
