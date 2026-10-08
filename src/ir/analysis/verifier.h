#pragma once

#include "core/types.h"
#include "ir/ir.h"
#include "ir/ir_ops.h"
#include "type/type_arena.h"
#include <array>
#include <string>
#include <type_traits>

namespace z::ir {

inline constexpr int max_ir_operands = 3;
struct OperandsList {
    std::array<OpC, max_ir_operands> operands;

    [[nodiscard]] constexpr usize size() const {
        auto s = 0;
        for (const auto op : operands) {
            if (op.kind == OpKind::Err)
                break;
            s++;
        }
        return s;
    }

    constexpr OpC operator[](usize i) const { return operands.at(i); }
};

struct OpInfo {
    IROp op;
    std::string mnemonic;
    DestType ret;
    OperandsList operands;
};

template <typename... Ts> constexpr OperandsList make_operands(Ts... operands) {
    static_assert(sizeof...(operands) <= max_ir_operands);
    return {operands...};
}

inline constexpr std::array<OpInfo, 42> op_info_arr = {
#undef IR_OP
#define IR_OP(name, mnemonic, class, dest, ...)                                \
    OpInfo{IROp::name, mnemonic, DestType::dest, make_operands(__VA_ARGS__)},
#include "ir/ir_ops.def"
};

constexpr const OpInfo& op_info(IROp op) {
    return op_info_arr[static_cast<std::underlying_type_t<IROp>>(op)];
}

class IRVerifier {
    [[nodiscard]] static bool verify_uses_table(const IRFunction& func);
    [[nodiscard]] static bool verify_uses_in_table(const IRFunction& func);
    [[nodiscard]] static bool verify_def_table(const IRFunction& func);
    [[nodiscard]] static bool verify_defs_in_table(const IRFunction& func);
    [[nodiscard]] static bool verify_phi_insts(const IRFunction& func);
    [[nodiscard]] static bool verify_no_unused_insts(const IRFunction& func);
    [[nodiscard]] static bool verify_last_inst_is_term(const IRFunction& func);
    [[nodiscard]] static bool verify_inst_operands(const IRFunction& func,
                                                   const type::TypeArena& ty);

public:
    [[nodiscard]] static bool verify(const IRFunction& func,
                                     const type::TypeArena& ty);
    [[nodiscard]] static bool verify_out_of_ssa(const IRFunction& func);
};
} // namespace z::ir
