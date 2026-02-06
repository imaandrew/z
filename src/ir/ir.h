#pragma once

#include "core/panic.h"
#include "core/string_pool.h"
#include "ir/condition_codes.h"
#include "ir/constants.h"
#include "parser/ast.h"
#include "type/type_ref.h"
#include <cstdint>
#include <initializer_list>
#include <utility>
#include <variant>
#include <vector>

namespace z::ir {

enum class IROp : std::uint8_t {
    IAdd,
    ISub,
    IMul,
    SDiv,
    UDiv,
    IMod,
    INeg,

    FAdd,
    FSub,
    FMul,
    FDiv,
    FNeg,

    And,
    Or,
    Xor,
    Not,
    Shl,
    Lsr,
    Asr,

    ICmp,
    FCmp,

    Alloca,
    Load,
    Store,
    LoadConst,
    StoreConst,

    GetElementPtr,
    GetFieldPtr,

    ArrayInit,
    StructInit,
    TupleInit,

    Call,
    CallMethod,
};

using ast::BinOp;
using ast::UnOp;

static constexpr IROp get_ir_op(UnOp kind) {
    switch (kind) {
    case UnOp::BitNot:
    case UnOp::LogicNot:
        return IROp::Not;
    default:
        panic("Invalid op");
    }
}

static constexpr IROp get_ir_op(BinOp kind) {
    switch (kind) {
    case BinOp::BitXor:
    case BinOp::BitXorEq:
        return IROp::Xor;
    case BinOp::BitAnd:
    case BinOp::BitAndEq:
        return IROp::And;
    case BinOp::BitOr:
    case BinOp::BitOrEq:
        return IROp::Or;
    case BinOp::Shl:
    case BinOp::ShlEq:
        return IROp::Shl;
    default:
        panic("Invalid op");
    }
}

static constexpr IROp get_int_ir_op(UnOp op) {
    switch (op) {
    case UnOp::Inc:
        return IROp::IAdd;
    case UnOp::Dec:
        return IROp::ISub;
    default:
        panic("Invalid op");
    }
}

static constexpr IROp get_int_ir_op(BinOp kind) {
    switch (kind) {
    case BinOp::Add:
    case BinOp::AddEq:
        return IROp::IAdd;
    case BinOp::Sub:
    case BinOp::SubEq:
        return IROp::ISub;
    case BinOp::Mul:
    case BinOp::MulEq:
        return IROp::IMul;
    case BinOp::Mod:
    case BinOp::ModEq:
        return IROp::IMod;
    default:
        panic("Invalid op");
    }
}

static constexpr IROp get_float_ir_op(UnOp op) {
    switch (op) {
    case UnOp::Inc:
        return IROp::FAdd;
    case UnOp::Dec:
        return IROp::FSub;
    default:
        panic("Invalid op");
    }
}

static constexpr IROp get_float_ir_op(BinOp op) {
    switch (op) {
    case BinOp::Add:
    case BinOp::AddEq:
        return IROp::FAdd;
    case BinOp::Sub:
    case BinOp::SubEq:
        return IROp::FSub;
    case BinOp::Mul:
    case BinOp::MulEq:
        return IROp::FMul;
    case BinOp::Div:
    case BinOp::DivEq:
        return IROp::FDiv;
    default:
        panic("Invalid op");
    }
}

static constexpr IntCC get_int_cc(BinOp op, bool is_signed) {
    switch (op) {
        case BinOp::Eq:
            return IntCC::Equal;
        case BinOp::Ne:
            return IntCC::NotEqual;
        case BinOp::Gt:
        return is_signed ? IntCC::SignedGreaterThan
                         : IntCC::UnsignedGreaterThan;
        case BinOp::Ge:
        return is_signed ? IntCC::SignedGreaterEqual
                         : IntCC::UnsignedGreaterEqual;
        case BinOp::Lt:
            return is_signed ? IntCC::SignedLessThan : IntCC::UnsignedLessThan;
        case BinOp::Le:
            return is_signed ? IntCC::SignedLessEqual : IntCC::UnsignedLessEqual;
        default:
            panic("Invalid BinOp for IntCC");
    }
}

static constexpr FloatCC get_float_cc(BinOp op) {
    switch (op) {
        case BinOp::Eq:
            return FloatCC::Equal;
        case BinOp::Ne:
            return FloatCC::NotEqual;
        case BinOp::Gt:
            return FloatCC::GreaterThan;
        case BinOp::Ge:
            return FloatCC::GreaterEqual;
        case BinOp::Lt:
            return FloatCC::LessThan;
        case BinOp::Le:
            return FloatCC::LessEqual;
        default:
            panic("Invalid BinOp for FloatCC");
    }
}

enum class TerminatorKind : std::uint8_t {
    Jump,
    Branch,
    Return,
    Unreachable,
};

struct VReg {
    std::uint32_t id;
    type::TypeRef type;
};

struct Immediate {
    std::variant<ConstInt, ConstFloat, bool> val;
    type::TypeRef type;

    [[nodiscard]] bool is_int() const {
        return std::holds_alternative<ConstInt>(val);
    }

    [[nodiscard]] bool is_float() const {
        return std::holds_alternative<ConstFloat>(val);
    }

    [[nodiscard]] bool is_bool() const {
        return std::holds_alternative<bool>(val);
    }

    [[nodiscard]] const ConstInt& as_int() const {
        return std::get<ConstInt>(val);
    }

    [[nodiscard]] const ConstFloat& as_float() const {
        return std::get<ConstFloat>(val);
    }

    [[nodiscard]] bool as_bool() const { return std::get<bool>(val); }
};

template <typename T>
concept ImmediateVal = requires(decltype(Immediate::val)& variant_ref,
                                T value) { variant_ref = value; };

struct Label {
    std::uint32_t block_id;
};

struct Field {
    StringID name;
};

struct Operand {
    std::variant<VReg, Immediate, Label, Field> val;

    static Operand reg(VReg reg) { return Operand{.val = reg}; }

    static Operand imm(ImmediateVal auto val, type::TypeRef type) {
        return Operand{.val = Immediate{.val = val, .type = type}};
    }

    static Operand label(std::uint32_t block_id) {
        return Operand{.val = Label{.block_id = block_id}};
    }

    static Operand field(StringID field_name) {
        return Operand{.val = Field{.name = field_name}};
    }

    [[nodiscard]] bool is_reg() const {
        return std::holds_alternative<VReg>(val);
    }
    [[nodiscard]] bool is_imm() const {
        return std::holds_alternative<Immediate>(val);
    }
    [[nodiscard]] bool is_label() const {
        return std::holds_alternative<Label>(val);
    }
    [[nodiscard]] bool is_field() const {
        return std::holds_alternative<Field>(val);
    }

    VReg& as_reg() { return std::get<VReg>(val); }
    [[nodiscard]] const VReg& as_reg() const { return std::get<VReg>(val); }

    Immediate& as_imm() { return std::get<Immediate>(val); }
    [[nodiscard]] const Immediate& as_imm() const {
        return std::get<Immediate>(val);
    }
};

struct Instruction {
    IROp op;
    VReg dest;
    std::vector<Operand> operands;
    type::TypeRef type;

    Instruction(IROp op, VReg dest, std::initializer_list<Operand> operands,
                type::TypeRef type)
        : op(op), dest(dest), operands(operands), type(type) {}
};

struct BasicBlock {
    std::uint32_t id;
    std::uint32_t inst_start;
    std::uint16_t inst_count;
    TerminatorKind term;
};

struct IRFunction {
    StringID name;
    type::TypeRef return_type;
    std::vector<VReg> params;
    std::uint32_t block_start{};
    std::uint16_t block_count{};

    IRFunction(StringID name, type::TypeRef return_type,
               std::vector<VReg> params, std::uint32_t /*block_start*/,
               std::uint16_t /*block_count*/)
        : name(name), return_type(return_type), params(std::move(params)) {}
};

} // namespace z::ir
