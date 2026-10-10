#pragma once

#include "bytecode/bytecode.h"
#include "core/panic.h"
#include "core/types.h"
#include "core/zctxt.h"
#include "ir/ir.h"
#include "ir/ir_builder.h"
#include <cstdint>
#include <variant>
#include <vector>
namespace z::bytecode {
using namespace z;

class Builder {
    std::vector<Function> funcs;
    Function* current_func = nullptr;
    Chunk* current_chunk = nullptr;
    ZContext* ctxt;

    /// BlockID to chunk idx
    std::vector<u32> chunk_map;

    struct PendingBranch {
        u32 chunk_idx;
        ir::VReg cond;
        ir::BlockID b_true;
        ir::BlockID b_false;
    };

    /// MUST be sorted in ascending order by chunk_idx
    std::vector<PendingBranch> pending_branches;

    void lower_func(const ir::IRFunction& func);
    void lower_block(const ir::BasicBlock& block, const ir::IRFunction& func);
    void lower_int_arith(const ir::Instruction& inst);
    void lower_int_logic(const ir::Instruction& inst);
    void lower_ineg(const ir::Instruction& inst);
    void lower_copy(const ir::Instruction& inst);
    void lower_load_const(const ir::Instruction& inst);
    void lower_cmp(const ir::Instruction& inst);
    void lower_branch(const ir::Instruction& inst);

    void emit_inst(Instruction inst) { current_chunk->add_inst(inst); }

    void fixup_branches();

    static u8 lower_vreg(ir::VReg vreg) {
        expect(vreg.id <= UINT8_MAX, "");
        return static_cast<u8>(vreg.id);
    }

public:
    explicit Builder(ZContext& ctxt) : ctxt(&ctxt) {}
    std::vector<Function> lower_file(const ir::IRFile& file);
};
} // namespace z::bytecode
