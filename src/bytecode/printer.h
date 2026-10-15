#pragma once

#include "bytecode/byte_ops.h"
#include "bytecode/bytecode.h"
#include "core/types.h"
#include "core/zctxt.h"
#include <array>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <utility>
namespace z::bytecode {
// unused operand slot in byte_ops.def
struct _ {};

enum class OperandType : u8 {
    _,
    i64,
    u64,
    i32,
    u32,
    i16,
    u16,
    i8,
    u8,
    f64,
};

template <typename T> constexpr OperandType operand_type() {
    if constexpr (std::is_same_v<T, _>)
        return OperandType::_;
    else if constexpr (std::is_same_v<T, i64>)
        return OperandType::i64;
    else if constexpr (std::is_same_v<T, u64>)
        return OperandType::u64;
    else if constexpr (std::is_same_v<T, i32>)
        return OperandType::i32;
    else if constexpr (std::is_same_v<T, u32>)
        return OperandType::u32;
    else if constexpr (std::is_same_v<T, i16>)
        return OperandType::i16;
    else if constexpr (std::is_same_v<T, u16>)
        return OperandType::u16;
    else if constexpr (std::is_same_v<T, i8>)
        return OperandType::i8;
    else if constexpr (std::is_same_v<T, u8>)
        return OperandType::u8;
    else if constexpr (std::is_same_v<T, double>)
        return OperandType::f64;
    else
        static_assert(false, "unsupported operand type in byte_ops.def");
}

struct OperandsList {
    std::array<OperandType, 3> operands;
};

template <typename... Ts> constexpr OperandsList make_operands() {
    static_assert(sizeof...(Ts) == 3,
                  "byte_ops.def entries need 3 operand types");
    return {{operand_type<Ts>()...}};
}

struct OpInfo {
    Op op;
    std::string_view mnemonic;
    InstClass i_class;
    OperandsList operands;
};

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
inline constexpr auto op_info_arr = std::to_array<OpInfo>({
#define BYTE_OP(name, mnemonic, i_class, ...)                                  \
    OpInfo{Op::name, mnemonic, i_class, make_operands<__VA_ARGS__>()},
#include "bytecode/byte_ops.def"
});
// NOLINTEND(cppcoreguidelines-macro-usage)

static_assert([] {
    usize i = 0;
    for (const auto& info : op_info_arr) {
        if (std::to_underlying(info.op) != i++)
            return false;
    }
    return true;
}());

constexpr const OpInfo& op_info(Op op) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return op_info_arr[std::to_underlying(op)];
}

class Printer {
    const ZContext* ctxt;
    std::ostream& os;

public:
    explicit Printer(const ZContext& ctxt, std::ostream& os)
        : ctxt(&ctxt), os(os) {}

    void print_func(const Function& func);
    void print_chunk(const Chunk& chunk);
    void print_inst(const Instruction& inst);
    void print_op(Op op);
    void print_reg(u8 reg);
    void print_imm8(i8 imm, bool is_signed);
    void print_imm16(i16 imm, bool is_signed);
    void print_offset(i32 offset);
};
} // namespace z::bytecode
