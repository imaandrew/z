#pragma once

#include "core/types.h"
#include <string_view>
#include <type_traits>
#include <utility>
namespace z::ir {
enum class OpKind : u8 {
    Err = 0,
    Reg = 1U << 0U,
    Imm = 1U << 1U,
    Label = 1U << 2U,
    Index = 1U << 3U,
    IntCC = 1U << 4U,
    FloatCC = 1U << 5U,
    FuncID = 1U << 6U,
    Type = 1U << 7U,
};

consteval OpKind operator|(OpKind lhs, OpKind rhs) {
    using T = std::underlying_type_t<OpKind>;
    return static_cast<OpKind>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

consteval OpKind operator&(OpKind lhs, OpKind rhs) {
    using T = std::underlying_type_t<OpKind>;
    return static_cast<OpKind>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

enum class Type : u8 { None, Int, Float, Bool, Type, Pointer, Any, Aggregate };

struct OpC {
    OpKind kind;
    Type type;
};

enum class DestType : u8 {
    None,
    SameArgs,
    Bool,
    Int,
    SameOp0,
    PtrToOp0,
    PointeeOp0,
    ElemOp0,
    PtrToElem0,
    RetOp0,
    Custom,
};

namespace Op {
constexpr OpC IntConst = {.kind = OpKind::Imm, .type = Type::Int};
constexpr OpC IntType = {.kind = OpKind::Imm | OpKind::Reg, .type = Type::Int};
constexpr OpC FloatType = {.kind = OpKind::Imm | OpKind::Reg,
                           .type = Type::Float};
constexpr OpC BoolType = {.kind = OpKind::Imm | OpKind::Reg,
                          .type = Type::Bool};
constexpr OpC AnyType = {.kind = OpKind::Reg | OpKind::Imm, .type = Type::Any};
constexpr OpC Ptr = {.kind = OpKind::Reg, .type = Type::Pointer};
constexpr OpC AggregateType = {.kind = OpKind::Reg, .type = Type::Aggregate};
constexpr OpC ICond = {.kind = OpKind::IntCC, .type = Type::None};
constexpr OpC FCond = {.kind = OpKind::FloatCC, .type = Type::None};
constexpr OpC BlockLabel = {.kind = OpKind::Label, .type = Type::None};
constexpr OpC MetaType = {.kind = OpKind::Type, .type = Type::Type};
} // namespace Op

enum class IROp : u8 {
#define IR_OP(name, mnemonic, class, dest, ...) name,
#include "ir/ir_ops.def"
};

constexpr std::string_view ir_op_to_string(IROp op) {
    static constexpr std::string_view ops[] = {
        #ifdef IR_OP
        #undef IR_OP
        #define IR_OP(name, mnemonic, class, dest, ...) mnemonic,
        #include "ir/ir_ops.def"
        #endif
    };

    return ops[std::to_underlying(op)];
}
} // namespace z::ir
