#pragma once

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
    InvalidOp,
    StackOverflow,
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
    case Error::InvalidOp:
        return "invalid opcode";
    case Error::StackOverflow:
        return "register stack overflow";
    }

    std::unreachable();
}

struct Trap {
    const bytecode::Instruction* inst;
    const bytecode::Function* func;
    Error err;
};
} // namespace z::vm
