#pragma once

#include "core/types.h"

namespace z::vm {
class ALU {
    static constexpr u64 checked_addi(u64 a, u64 b, bool is_signed,
                                      bool& overflow);
    static constexpr u64 checked_subi(u64 a, u64 b, bool is_signed,
                                      bool& overflow);
    static constexpr u64 checked_muli(u64 a, u64 b, bool is_signed,
                                      bool& overflow);
    static constexpr u64 checked_divi(u64 a, u64 b, bool is_signed,
                                      bool& overflow);
    static constexpr u64 checked_remi(u64 a, u64 b, bool is_signed,
                                      bool& overflow);

public:
    static u64 iadd(u64 a, u64 b, bool is_signed, bool& overflow);
    static u64 isub(u64 a, u64 b, bool is_signed, bool& overflow);
    static u64 imul(u64 a, u64 b, bool is_signed, bool& overflow);
    static u64 idiv(u64 a, u64 b, bool is_signed, bool& overflow,
                    bool& undefined);
    static u64 irem(u64 a, u64 b, bool is_signed, bool& overflow,
                    bool& undefined);
    static u64 ineg(u64 a, bool& overflow);
    static u64 sll(u64 a, u64 b, bool& undefined);
    static u64 srl(u64 a, u64 b, bool& undefined);
    static u64 sra(u64 a, u64 b, bool& undefined);
};
} // namespace z::vm
