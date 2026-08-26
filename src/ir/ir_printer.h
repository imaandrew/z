#pragma once

#include "core/zctxt.h"
#include "ir/ir.h"
#include "ir/ir_builder.h"
#include <ostream>
#include <string>
#include <string_view>

namespace z::ir {

class IRPrinter {
    const ZContext* ctxt;
    bool show_inst_ids_;

    void dump_inst(const Instruction& inst, const IRFile& ir,
                   std::ostream& os) const;
    void dump_operand(const Operand& op, const IRFile& ir,
                      std::ostream& os) const;
    void dump_immediate(const Immediate& imm, std::ostream& os) const;
    static void dump_terminator(TerminatorKind term, std::ostream& os);
    static constexpr std::string ir_op_to_string(IROp op);
    static std::string tolower(std::string_view s);

public:
    explicit IRPrinter(const ZContext& ctxt, bool show_inst_ids = false)
        : ctxt(&ctxt), show_inst_ids_(show_inst_ids) {}
    void dump_file(const IRFile& ir, std::ostream& os) const;
    void dump_func(const IRFunction& func, const IRFile& ir,
                   std::ostream& os) const;
};

} // namespace z::ir
