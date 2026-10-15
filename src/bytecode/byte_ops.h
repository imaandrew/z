#pragma once

#include "core/types.h"
#include <string_view>
#include <utility>

// NOLINTBEGIN(cppcoreguidelines-macro-usage, readability-trailing-comma,
// cppcoreguidelines-avoid-c-arrays,
// cppcoreguidelines-pro-bounds-constant-array-index)
namespace z::bytecode {
enum class InstClass : u8 { R, RI, RIU, I, IU, B, J };

enum class Op : u8 {
#define BYTE_OP(name, mnemonic, class, ...) name,
#include "bytecode/byte_ops.def"
};

constexpr std::string_view byte_op_to_string(Op op) {
    static constexpr std::string_view ops[] = {
#define BYTE_OP(name, mnemonic, class, ...) mnemonic,
#include "bytecode/byte_ops.def"
    };

    return ops[std::to_underlying(op)];
}
} // namespace z::bytecode
// NOLINTEND(cppcoreguidelines-macro-usage, readability-trailing-comma,
// cppcoreguidelines-avoid-c-arrays,
// cppcoreguidelines-pro-bounds-constant-array-index)
