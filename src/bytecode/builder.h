#pragma once

#include "bytecode/byte_ops.h"
#include "bytecode/bytecode.h"
#include "core/panic.h"
#include "core/types.h"
#include "core/zctxt.h"
#include "ir/ir.h"
#include <cstdint>
#include <vector>
namespace z::bytecode {
using namespace z;

class Builder {
    std::vector<Function> funcs;
    Function* current_func = nullptr;
    Chunk* current_chunk = nullptr;
    ZContext* ctxt;
    u32 arg_index = 0;
    std::vector<ir::BlockID> layout;

    /// BlockID to chunk idx
    std::vector<u32> chunk_map;

    struct PendingBranch {
        u32 chunk_idx;
        u32 inst_idx;
    };

    /// MUST be sorted in ascending order by chunk_idx
    std::vector<PendingBranch> pending_branches;
    std::vector<PendingBranch> pending_jumps;

    void lower_func(const ir::IRFunction& func);
    void lower_block(const ir::BasicBlock& block, const ir::IRFunction& func);
    void lower_int_arith(const ir::Instruction& inst);
    void lower_int_logic(const ir::Instruction& inst);
    void lower_not(const ir::Instruction& inst);
    void lower_r_rd(const ir::Instruction& inst);
    void lower_copy(const ir::Instruction& inst);
    void lower_load_const(const ir::Instruction& inst);
    void lower_cmp(const ir::Instruction& inst);
    void lower_branch(const ir::Instruction& inst);
    void lower_jump(const ir::Instruction& inst);
    void lower_call(const ir::Instruction& inst, u32 num_regs);
    void lower_ret(const ir::Instruction& inst);

    void emit_inst(Instruction inst) { current_chunk->add_inst(inst); }

    void fixup_branches();

    static u8 lower_vreg(ir::VReg vreg) {
        expect(vreg.id <= UINT8_MAX, "");
        return static_cast<u8>(vreg.id);
    }

    void emit_branch(Op op, ir::VReg cond, u32 chunk) {
        pending_branches.emplace_back(current_chunk->get_idx(),
                                      current_chunk->get_insts().size());
        emit_inst(
            Instruction::B(op, lower_vreg(cond), static_cast<i16>(chunk)));
    }

    void emit_jump(u32 chunk) {
        pending_jumps.emplace_back(current_chunk->get_idx(),
                                   current_chunk->get_insts().size());
        emit_inst(Instruction::J(Op::Jump, static_cast<i32>(chunk)));
    }

public:
    explicit Builder(ZContext& ctxt) : ctxt(&ctxt) {}
    std::vector<Function> lower_file(const ir::IRFile& file);
};
} // namespace z::bytecode
