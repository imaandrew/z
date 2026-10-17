#pragma once

#include "bytecode/bytecode.h"
#include "core/types.h"
#include "vm/trap.h"
#include <expected>
#include <vector>

namespace z::vm {

class VM {
    const bytecode::File& file;
    std::vector<u64> regs;
    u64 pc = 0;
    u32 base = 0;
    const bytecode::Function* cur_func = nullptr;

    struct Frame {
        const bytecode::Function* func;
        u64 return_pc;
        u32 base;
    };

    std::vector<Frame> frames;

    void ensure_stack(usize size) {
        if (size > regs.size())
            regs.resize(size);
    }

    static void report(const Trap& t);

    enum class ExitType : u8 {
        RetVoid,
        Ret,
        NoReturn,
    };

    std::expected<ExitType, Trap> exec_inst();
    u64 read_reg(u8 idx);
    void write_reg(u8 idx, u64 val);

public:
    explicit VM(const bytecode::File& file)
        : file(file), cur_func(&file.get_funcs()[file.get_entry()]) {}
    i64 run();
};
} // namespace z::vm
