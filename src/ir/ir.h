#pragma once

#include "core/index.h"
#include "core/panic.h"
#include "core/string_pool.h"
#include "core/types.h"
#include "ir/condition_codes.h"
#include "ir/constants.h"
#include "parser/ast.h"
#include "type/type_ref.h"
#include <array>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace z::ir {

enum class IROp : u8 {
    IAdd,
    ISub,
    IMul,
    SDiv,
    UDiv,
    SRem,
    URem,
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
    ExtractField,
    InsertField,
    ExtractElement,
    InsertElement,

    ArrayInit,
    StructInit,
    TupleInit,

    Call,
    CallIndirect,
    Branch,
    Jump,
    Ret,
    Arg,

    Phi,
    Copy,
    Dead
};

using ast::BinOp;
using ast::UnOp;

static constexpr std::optional<ConstInt>
fold_int_op(IROp op, const ConstInt& lhs, const ConstInt& rhs, bool& overflow) {
    auto with_overflow = [&overflow](auto fn) -> std::optional<ConstInt> {
        auto res = fn();
        return overflow ? std::nullopt : std::optional(res);
    };

    switch (op) {
    case IROp::IAdd:
        return with_overflow([&]() { return lhs.add(rhs, overflow); });
    case IROp::ISub:
        return with_overflow([&]() { return lhs.sub(rhs, overflow); });
    case IROp::IMul:
        return with_overflow([&]() { return lhs.mul(rhs, overflow); });
    case IROp::SDiv:
        return with_overflow([&]() { return lhs.sdiv(rhs, overflow); });
    case IROp::UDiv:
        return lhs.udiv(rhs);
    case IROp::SRem:
        return lhs.srem(rhs);
    case IROp::URem:
        return lhs.urem(rhs);
    case IROp::Shl:
        return lhs.shl(rhs);
    case IROp::Lsr:
        return lhs.lshr(rhs);
    case IROp::Asr:
        return lhs.ashr(rhs);
    case IROp::And:
        return lhs.bit_and(rhs);
    case IROp::Or:
        return lhs.bit_or(rhs);
    case IROp::Xor:
        return lhs.bit_xor(rhs);
    default:
        return std::nullopt;
    }
}

static constexpr std::optional<ConstFloat>
fold_float_op(IROp op, const ConstFloat& lhs, const ConstFloat& rhs) {
    switch (op) {
    case IROp::FAdd:
        return lhs.add(rhs);
    case IROp::FSub:
        return lhs.sub(rhs);
    case IROp::FMul:
        return lhs.mul(rhs);
    case IROp::FDiv:
        return lhs.div(rhs);
    default:
        return std::nullopt;
    }
}

static constexpr std::optional<bool> fold_bool_op(BinOp op, bool lhs,
                                                  bool rhs) {
    switch (op) {
    case BinOp::Eq:
        return lhs == rhs;
    case BinOp::Ne:
        return lhs != rhs;
    case BinOp::LogicAnd:
        return lhs && rhs;
    case BinOp::LogicOr:
        return lhs || rhs;
    case BinOp::BitXor:
        return lhs ^ rhs;
    default:
        return std::nullopt;
    }
}

static constexpr IROp get_ir_op(UnOp kind) {
    switch (kind) {
    case UnOp::BitNot:
    case UnOp::LogicNot:
        return IROp::Not;
    case UnOp::Inc:
    case UnOp::Dec:
    case UnOp::Neg:
        panic("get_ir_op can't handle typed operations");
    }

    std::unreachable();
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
    default:
        panic("get_ir_op can't handle typed operations");
    }
}

static constexpr IROp get_int_ir_op(UnOp op) {
    switch (op) {
    case UnOp::Inc:
        return IROp::IAdd;
    case UnOp::Dec:
        return IROp::ISub;
    case UnOp::Neg:
        return IROp::INeg;
    default:
        return get_ir_op(op);
    }
}

static constexpr IROp get_int_ir_op(BinOp kind, bool is_signed) {
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
        return is_signed ? IROp::SRem : IROp::URem;
    case BinOp::EqEq:
    case BinOp::Ne:
    case BinOp::Gt:
    case BinOp::Lt:
    case BinOp::Ge:
    case BinOp::Le:
        return IROp::ICmp;
    case BinOp::Shl:
    case BinOp::ShlEq:
        return IROp::Shl;
    case BinOp::Div:
    case BinOp::DivEq:
        return is_signed ? IROp::SDiv : IROp::UDiv;
    case BinOp::Shr:
    case BinOp::ShrEq:
        return is_signed ? IROp::Asr : IROp::Lsr;
    default:
        return get_ir_op(kind);
    }
}

static constexpr IROp get_float_ir_op(UnOp op) {
    switch (op) {
    case UnOp::Inc:
        return IROp::FAdd;
    case UnOp::Dec:
        return IROp::FSub;
    case UnOp::Neg:
        return IROp::FNeg;
    default:
        return get_ir_op(op);
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
    case BinOp::EqEq:
    case BinOp::Ne:
    case BinOp::Gt:
    case BinOp::Lt:
    case BinOp::Ge:
    case BinOp::Le:
        return IROp::FCmp;
    default:
        return get_ir_op(op);
    }
}

static constexpr IntCC get_int_cc(BinOp op, bool is_signed) {
    switch (op) {
    case BinOp::EqEq:
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
    case BinOp::EqEq:
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

enum class TerminatorKind : u8 {
    Jump,
    Branch,
    Return,
    Unreachable,
};

struct VReg {
    u32 id;
    type::TypeRef type;

    bool operator==(const VReg&) const = default;
};

struct Immediate {
    std::variant<ConstInt, ConstFloat, bool, StringID, unsigned char> val;
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

    [[nodiscard]] bool is_string() const {
        return std::holds_alternative<StringID>(val);
    }

    [[nodiscard]] bool is_char() const {
        return std::holds_alternative<unsigned char>(val);
    }

    [[nodiscard]] const ConstInt& as_int() const {
        return std::get<ConstInt>(val);
    }

    [[nodiscard]] const ConstFloat& as_float() const {
        return std::get<ConstFloat>(val);
    }

    [[nodiscard]] bool as_bool() const { return std::get<bool>(val); }

    [[nodiscard]] StringID as_string() const { return std::get<StringID>(val); }

    [[nodiscard]] unsigned char as_char() const {
        return std::get<unsigned char>(val);
    }
};

template <typename T>
concept ImmediateVal = requires(decltype(Immediate::val)& variant_ref,
                                T value) { variant_ref = value; };

struct BlockTag {};
using BlockID = Index<BlockTag>;

struct FuncTag {};
using FuncID = Index<FuncTag>;

struct Label {
    BlockID block_id;
};

struct Index {
    u32 idx;
};

struct Operand {
    std::variant<VReg, Immediate, Label, Index, IntCC, FloatCC, FuncID> val;

    static Operand reg(VReg reg) { return Operand{.val = reg}; }

    static Operand imm(ImmediateVal auto val, type::TypeRef type) {
        return Operand{.val = Immediate{.val = val, .type = type}};
    }

    static Operand label(BlockID block_id) {
        return Operand{.val = Label{.block_id = block_id}};
    }

    static Operand field(u32 field_idx) {
        return Operand{.val = Index{.idx = field_idx}};
    }

    static Operand intcc(IntCC intcc) { return Operand{.val = intcc}; }

    static Operand floatcc(FloatCC floatcc) { return Operand{.val = floatcc}; }

    static Operand func(FuncID func) { return Operand{.val = func}; }

    [[nodiscard]] bool is_reg() const {
        return std::holds_alternative<VReg>(val);
    }
    [[nodiscard]] bool is_imm() const {
        return std::holds_alternative<Immediate>(val);
    }
    [[nodiscard]] bool is_label() const {
        return std::holds_alternative<Label>(val);
    }
    [[nodiscard]] bool is_index() const {
        return std::holds_alternative<Index>(val);
    }
    [[nodiscard]] bool is_intcc() const {
        return std::holds_alternative<IntCC>(val);
    }
    [[nodiscard]] bool is_floatcc() const {
        return std::holds_alternative<FloatCC>(val);
    }
    [[nodiscard]] bool is_func() const {
        return std::holds_alternative<FuncID>(val);
    }

    VReg& as_reg() { return std::get<VReg>(val); }
    [[nodiscard]] const VReg& as_reg() const { return std::get<VReg>(val); }

    Immediate& as_imm() { return std::get<Immediate>(val); }
    [[nodiscard]] const Immediate& as_imm() const {
        return std::get<Immediate>(val);
    }

    Label& as_label() { return std::get<Label>(val); }
    [[nodiscard]] const Label& as_label() const { return std::get<Label>(val); }

    Index& as_index() { return std::get<Index>(val); }
    [[nodiscard]] const Index& as_index() const { return std::get<Index>(val); }

    [[nodiscard]] IntCC as_intcc() const { return std::get<IntCC>(val); }

    [[nodiscard]] FloatCC as_floatcc() const { return std::get<FloatCC>(val); }

    [[nodiscard]] FuncID as_func() const { return std::get<FuncID>(val); }
};

struct InstTag {};
using InstId = z::Index<InstTag>;

struct Instruction {
    InstId id;
    IROp op;
    std::optional<VReg> dest;
    std::vector<Operand> operands;

    Instruction(InstId id, IROp op, VReg dest,
                std::initializer_list<Operand> operands)
        : id(id), op(op), dest(dest), operands(operands) {}

    Instruction(InstId id, IROp op, VReg dest, std::vector<Operand> operands)
        : id(id), op(op), dest(dest), operands(std::move(operands)) {}
    Instruction(InstId id, IROp op, std::initializer_list<Operand> operands)
        : id(id), op(op), operands(operands) {}
};

struct BasicBlock {
    BlockID id;
    std::vector<InstId> insts;
    std::vector<InstId> phis;
    std::optional<TerminatorKind> term;
    std::vector<BlockID> predecessors;
    std::vector<BlockID> successors;

    explicit BasicBlock(BlockID id) : id(id) {};

    void add_predecessor(BlockID pred) {
        for (const auto p : predecessors) {
            if (p == pred)
                return;
        }

        predecessors.push_back(pred);
    }

    void add_successor(BlockID successor) {
        for (const auto s : successors) {
            if (s == successor)
                return;
        }

        successors.push_back(successor);
    }

    void replace_predecessor(BlockID old_pred, BlockID new_pred) {
        for (auto& p : predecessors) {
            if (p == old_pred) {
                p = new_pred;
                return;
            }
        }
    }

    void replace_successor(BlockID old_succ, BlockID new_succ) {
        for (auto& s : successors) {
            if (s == old_succ) {
                s = new_succ;
                return;
            }
        }
    }

    [[nodiscard]] auto all_insts() const {
        return std::array<std::span<const InstId>, 2>{phis, insts} |
               std::views::join;
    }

    [[nodiscard]] InstId terminator() const {
        expect(!insts.empty(), "terminator() on BasicBlock with no insts");
        return insts.back();
    }
};

struct IRFunction {
    FuncID id;
    StringID name;
    type::TypeRef return_type;
    std::vector<VReg> params;
    std::vector<BasicBlock> blocks;
    std::vector<Instruction> insts;

    struct InstRef {
        InstId inst;
        BlockID block;

        InstRef(InstId inst, BlockID block) : inst(inst), block(block) {}
    };

    class VRegInfo {
        struct DefUseList {
            InstRef def;
            std::vector<InstRef> uses;

            DefUseList(InstId def_inst, BlockID def_block)
                : def(def_inst, def_block) {}
        };

        std::vector<DefUseList> def_use;

    public:
        [[nodiscard]] usize num_regs() const { return def_use.size(); }

        u32 add_reg(InstId def_inst, BlockID def_block) {
            auto id = def_use.size();
            def_use.emplace_back(def_inst, def_block);
            return id;
        }

        void add_use(VReg reg, InstId inst, BlockID block) {
            def_use[reg.id].uses.emplace_back(inst, block);
        }

        [[nodiscard]] const InstRef& get_def(VReg reg) const {
            return def_use[reg.id].def;
        }

        [[nodiscard]] std::vector<InstRef>& get_uses(VReg reg) {
            return def_use[reg.id].uses;
        }

        [[nodiscard]] const std::vector<InstRef>& get_uses(VReg reg) const {
            return def_use[reg.id].uses;
        }
    };

    VRegInfo vreg_info;

    [[nodiscard]] usize num_regs() const { return vreg_info.num_regs(); }

    u32 add_reg(InstId def_inst, BlockID def_block) {
        return vreg_info.add_reg(def_inst, def_block);
    }

    void add_use(VReg reg, InstId inst, BlockID block) {
        vreg_info.add_use(reg, inst, block);
    }

    [[nodiscard]] const InstRef& get_def(VReg reg) const {
        return vreg_info.get_def(reg);
    }

    [[nodiscard]] std::vector<InstRef>& get_uses(VReg reg) {
        return vreg_info.get_uses(reg);
    }

    [[nodiscard]] const std::vector<InstRef>& get_uses(VReg reg) const {
        return vreg_info.get_uses(reg);
    }

    BasicBlock& get_block(BlockID id) { return blocks[id.id]; }

    [[nodiscard]] const BasicBlock& get_block(BlockID id) const {
        return blocks[id.id];
    }

    IRFunction(FuncID id, StringID name, type::TypeRef return_type,
               std::vector<VReg> params)
        : id(id), name(name), return_type(return_type),
          params(std::move(params)) {}
};

} // namespace z::ir

template <> struct std::hash<z::ir::VReg> {
    usize operator()(const z::ir::VReg& reg) const noexcept {
        return std::hash<u32>{}(reg.id);
    }
};
