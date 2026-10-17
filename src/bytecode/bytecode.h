#pragma once

#include "bytecode/byte_ops.h"
#include "core/string_pool.h"
#include "core/types.h"
#include <optional>
#include <utility>
#include <vector>
namespace z::bytecode {
struct Instruction {
    u32 raw;

    explicit Instruction(u32 inst) : raw(inst) {}

    static Instruction R(Op op, u8 rd, u8 rs, u8 rt) {
        return Instruction(static_cast<u32>(rt) << 24U |
                           static_cast<u32>(rs) << 16U |
                           static_cast<u32>(rd) << 8U | static_cast<u32>(op));
    }

    static Instruction I(Op op, u8 rd, i16 imm) {
        return Instruction(static_cast<u32>(imm) << 16U |
                           static_cast<u32>(rd) << 8U | static_cast<u32>(op));
    }

    static Instruction IU(Op op, u8 rd, u16 imm) {
        return Instruction(static_cast<u32>(imm) << 16U |
                           static_cast<u32>(rd) << 8U | static_cast<u32>(op));
    }

    static Instruction RI(Op op, u8 rd, u8 rs, i8 imm) {
        return Instruction(static_cast<u32>(imm) << 24U |
                           static_cast<u32>(rs) << 16U |
                           static_cast<u32>(rd) << 8U | static_cast<u32>(op));
    }

    static Instruction RIU(Op op, u8 rd, u8 rs, u8 imm) {
        return Instruction(static_cast<u32>(imm) << 24U |
                           static_cast<u32>(rs) << 16U |
                           static_cast<u32>(rd) << 8U | static_cast<u32>(op));
    }

    static Instruction J(Op op, i32 offset) {
        return Instruction(static_cast<u32>(offset) << 8U |
                           static_cast<u32>(op));
    }

    static Instruction B(Op op, u8 rd, i16 offset) {
        return Instruction(static_cast<u32>(static_cast<u16>(offset)) << 16U |
                           static_cast<u32>(rd) << 8U | static_cast<u32>(op));
    }

    [[nodiscard]] Op op() const { return static_cast<Op>(raw & 0xFFU); }
    [[nodiscard]] u8 rd() const { return (raw >> 8U) & 0xFFU; }
    [[nodiscard]] u8 rs() const { return (raw >> 16U) & 0xFFU; }
    [[nodiscard]] u8 rt() const { return (raw >> 24U) & 0xFFU; }
    [[nodiscard]] i16 imm16() const { return static_cast<i16>(raw >> 16U); }
    [[nodiscard]] u16 imm16u() const { return static_cast<u16>(raw >> 16U); }
    [[nodiscard]] i8 imm8() const { return static_cast<i8>(raw >> 24U); }
    [[nodiscard]] u8 imm8u() const { return static_cast<u8>(raw >> 24U); }
    // NOLINTNEXTLINE(bugprone-signed-bitwise, hicpp-signed-bitwise)
    [[nodiscard]] i32 offset24() const { return static_cast<i32>(raw) >> 8U; }
};

class Function {
    StringID name;
    std::vector<Instruction> insts;
    std::vector<u64> constants;
    u32 nregs = 0;
    u32 nparams = 0;

public:
    explicit Function(StringID name, std::vector<u64> constants, u32 nparams)
        : name(name), constants(std::move(constants)), nparams(nparams) {}

    void set_nregs(u32 n) { nregs = n; }

    [[nodiscard]] u32 get_nregs() const { return nregs; }

    [[nodiscard]] u32 get_nparams() const { return nparams; }

    [[nodiscard]] StringID get_name() const { return name; }

    [[nodiscard]] std::vector<Instruction>& get_insts() { return insts; }

    [[nodiscard]] const std::vector<Instruction>& get_insts() const {
        return insts;
    }

    [[nodiscard]] std::optional<u64> get_constant(u16 idx) const {
        if (idx < constants.size())
            return constants[idx];
        return std::nullopt;
    }
};

class File {
    std::vector<Function> funcs;
    usize entry;

public:
    File(std::vector<Function> funcs, usize entry)
        : funcs(std::move(funcs)), entry(entry) {}

    [[nodiscard]] const std::vector<Function>& get_funcs() const {
        return funcs;
    }

    [[nodiscard]] usize get_entry() const { return entry; }
};

} // namespace z::bytecode
