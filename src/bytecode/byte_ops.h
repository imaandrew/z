#pragma once

#include "core/types.h"
#include <string_view>
#include <utility>

namespace z::bytecode {
enum class InstClass : u8 { R, RI, RIU, I, IU };

enum class Op : u8 {
#define BYTE_OP(name, mnemonic, class) name,
#include "bytecode/byte_ops.def"
};

constexpr std::string_view byte_op_to_string(Op op) {
    static constexpr std::string_view ops[] = {
#ifdef BYTE_OP
#undef BYTE_OP
#define BYTE_OP(name, mnemonic, class) mnemonic,
#include "bytecode/byte_ops.def"
#endif
    };

    return ops[std::to_underlying(op)];
}
} // namespace z::bytecode
