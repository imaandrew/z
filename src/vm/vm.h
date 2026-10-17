#pragma once

#include "bytecode/byte_ops.h"
#include "bytecode/bytecode.h"
#include "core/string_pool.h"
#include "core/types.h"
#include "vm/trap.h"
#include <array>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace z::vm {
using namespace bytecode;

class VM {
    const File& file;
    const StringPool& strings;
    const Function* cur_func = nullptr;

    struct RegStack {
        static constexpr usize N = 1000000;
        std::unique_ptr<std::array<u64, N>> p =
            std::make_unique_for_overwrite<std::array<u64, N>>();
        [[nodiscard]] u64* data() const { return p->data(); }
        static usize size() { return N; }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        u64& operator[](usize i) const { return (*p)[i]; }
    };
    RegStack regs;

    struct Frame {
        const Function* func;
        const Instruction* return_ip;
        u64* base;
    };
    std::vector<Frame> frames;

    void call(const Instruction* return_ip, u64* base, u16 callee) {
        frames.emplace_back(cur_func, return_ip, base);
        cur_func = &file.get_funcs()[callee];
    }

    void pop_frame() {
        cur_func = frames.back().func;
        frames.pop_back();
    }

    void report(const Trap& t);

    enum class Status : u8 {
        Halt,
        Trap,
    };

    std::optional<Trap> trap_;

    [[nodiscard]] Status trap(Error err, const Instruction* ip) {
        trap_ = {.inst = ip, .func = cur_func, .err = err};
        return Status::Trap;
    };

#ifdef __has_cpp_attribute
#if __has_cpp_attribute(clang::preserve_none)
#define PRESERVE_NONE [[clang::preserve_none]]
#endif
#endif

#ifndef PRESERVE_NONE
#define PRESERVE_NONE
#endif

#define BYTE_OP(name, mnemonic, ...)                                           \
    PRESERVE_NONE static Status op_##name(VM& vm, const Instruction* ip,       \
                                          u64* r);
#include "bytecode/byte_ops.def"
    PRESERVE_NONE static Status op_invalid(VM& vm, const Instruction* ip,
                                           u64* r);

    using Handler = Status (*)(VM& vm, const Instruction* ip,
                               u64* r) PRESERVE_NONE;

    static constexpr auto table = [] {
        std::array<Handler, 256> t{};
        for (auto& i : t) {
            i = &op_invalid;
        }
#define BYTE_OP(name, mnemonic, class, ...)                                    \
    t[std::to_underlying(Op::name)] = &VM::op_##name;
#include "bytecode/byte_ops.def"

        return t;
    }();

public:
    explicit VM(const File& file, const StringPool& strings)
        : file(file), strings(strings),
          cur_func(&file.get_funcs()[file.get_entry()]), regs() {}
    i64 run();
};
} // namespace z::vm
