#pragma once

#include "core/zctxt.h"
#include "ir/ir.h"
#include "ir/ir_builder.h"
#include <ostream>
#include <string>
#include <string_view>

namespace z::ir {

class IRPrinter {
    const IRFile* ir;
    const ZContext* ctxt;

    void dump_ir(const IRFunction& func, std::ostream& os) const;
    void dump_inst(const Instruction& inst, std::ostream& os) const;
    void dump_operand(const Operand& op, std::ostream& os) const;
    static void dump_immediate(const Immediate& imm, std::ostream& os);
    static void dump_terminator(TerminatorKind term, std::ostream& os);
    static constexpr std::string ir_op_to_string(IROp op);
    static std::string tolower(std::string_view s);

public:
    void dump(const IRFile& ir, const ZContext& ctxt, std::ostream& os);
};

} // namespace z::ir
