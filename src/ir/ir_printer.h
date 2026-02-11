#pragma once

#include "core/zctxt.h"
#include "ir/ir.h"

namespace z::ir {
void dump_ir(const IRFunction& func, ZContext& ctxt, std::ostream& os);
void dump_inst(const Instruction& inst, ZContext& ctxt, std::ostream& os);
void dump_operand(const Operand& op, std::ostream& os);
void dump_immediate(const Immediate& imm, std::ostream& os);
void dump_terminator(TerminatorKind term, std::ostream& os);
constexpr std::string ir_op_to_string(IROp op);
std::string tolower(std::string_view s);

} // namespace z::ir
