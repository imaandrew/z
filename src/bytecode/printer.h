#pragma once

#include "bytecode/bytecode.h"
#include "core/types.h"
#include "core/zctxt.h"
#include <ostream>
namespace z::bytecode {
class Printer {
    const ZContext* ctxt;
    std::ostream& os;

public:
    explicit Printer(const ZContext& ctxt, std::ostream& os)
        : ctxt(&ctxt), os(os) {}

    void print_func(const Function& func);
    void print_chunk(const Chunk& chunk);
    void print_inst(const Instruction& inst);
    void print_op(Op op);
    void print_reg(u8 reg);
    void print_imm8(i8 imm, bool is_signed);
    void print_imm16(i16 imm, bool is_signed);
};
} // namespace z::bytecode
