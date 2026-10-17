#pragma once

#include "bytecode/byte_ops.h"
#include "bytecode/bytecode.h"
#include "core/types.h"
#include <string_view>
#include <utility>

namespace z::vm {
enum class Error : u8 {
    DivisionByZero,
    IntegerOverflow,
    ShiftTooLarge,
    IntegerOutOfRange,
    ConstIdxOutOfBounds,
};

static constexpr std::string_view err_to_string(Error e) {
    switch (e) {
    case Error::DivisionByZero:
        return "division by zero";
    case Error::IntegerOverflow:
        return "integer overflow";
    case Error::ShiftTooLarge:
        return "shift amount too large";
    case Error::IntegerOutOfRange:
        return "integer out of range";
    case Error::ConstIdxOutOfBounds:
        return "const index out of bounds";
    }

    std::unreachable();
}

struct Trap {
    bytecode::Op op;
    u64 pc;
    const bytecode::Function* func;
    Error err;
    Trap(bytecode::Op op, u64 pc, const bytecode::Function* func, Error err)
        : op(op), pc(pc), func(func), err(err) {}
};
} // namespace z::vm
