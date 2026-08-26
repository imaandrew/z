#pragma once

#include "ir/ir.h"

namespace z::ir {
class IRVerifier {
    [[nodiscard]] static bool verify_uses_table(const IRFunction& func);
    [[nodiscard]] static bool verify_uses_in_table(const IRFunction& func);
    [[nodiscard]] static bool verify_def_table(const IRFunction& func);
    [[nodiscard]] static bool verify_defs_in_table(const IRFunction& func);
    [[nodiscard]] static bool verify_phi_insts(const IRFunction& func);
    [[nodiscard]] static bool verify_no_unused_insts(const IRFunction& func);
    [[nodiscard]] static bool verify_last_inst_is_term(const IRFunction& func);

public:
    [[nodiscard]] static bool verify(const IRFunction& func);
    [[nodiscard]] static bool verify_out_of_ssa(const IRFunction& func);
};
} // namespace z::ir
