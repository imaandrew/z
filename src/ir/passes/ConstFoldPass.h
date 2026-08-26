#pragma once

#include "core/panic.h"
#include "ir/analysis/analyses.h"
#include "ir/ir.h"
#include "ir/pass.h"
#include "type/type_ref.h"
#include <string_view>
#include <utility>

namespace z::ir {
class ConstFoldPass : public IRPass {
    static constexpr bool is_foldable(IROp op) {
        switch (op) {
        case IROp::IAdd:
        case IROp::ISub:
        case IROp::IMul:
        case IROp::SDiv:
        case IROp::UDiv:
        case IROp::SRem:
        case IROp::URem:
        case IROp::INeg:
        case IROp::FAdd:
        case IROp::FSub:
        case IROp::FMul:
        case IROp::FDiv:
        case IROp::FNeg:
        case IROp::And:
        case IROp::Or:
        case IROp::Xor:
        case IROp::Not:
        case IROp::Shl:
        case IROp::Lsr:
        case IROp::Asr:
        case IROp::ICmp:
        case IROp::FCmp:
            return true;
        default:
            return false;
        }
    }

public:
    [[nodiscard]] std::string_view name() const override {
        return "ConstFoldPass";
    }

    bool run(IRFunction& func, FuncAnalyses& /*analyses*/) override {
        bool changed = false;

        for (auto& inst : func.insts) {
            if (!is_foldable(inst.op))
                continue;

            bool all_imm = true;
            for (const auto& op : inst.operands) {
                if (!op.is_imm()) {
                    all_imm = false;
                    break;
                }
            }

            if (!all_imm)
                continue;

            if (inst.operands.size() == 1) {
                auto imm = inst.operands[0].as_imm();
                if (inst.op == IROp::INeg) {
                    inst.operands[0] =
                        Operand::imm(imm.as_int().neg(), imm.type);
                } else if (inst.op == IROp::FNeg) {
                    inst.operands[0] =
                        Operand::imm(imm.as_float().neg(), imm.type);
                } else if (inst.op == IROp::Not) {
                    inst.operands[0] =
                        Operand::imm(imm.as_int().bit_not(), imm.type);
                } else {
                    panic("Invalid number of operands for inst");
                }
            } else if (inst.operands.size() == 2) {
                auto lhs = inst.operands[0].as_imm();
                auto rhs = inst.operands[1].as_imm();

                switch (inst.op) {
                case IROp::IAdd:
                case IROp::ISub:
                case IROp::IMul:
                case IROp::SDiv:
                case IROp::UDiv:
                case IROp::SRem:
                case IROp::URem:
                case IROp::Shl:
                case IROp::Lsr:
                case IROp::Asr:
                case IROp::And:
                case IROp::Or:
                case IROp::Xor: {
                    bool overflow = false;
                    auto res = fold_int_op(inst.op, lhs.as_int(), rhs.as_int(),
                                           overflow);
                    if (overflow) {
                        // error
                    }

                    expect(res.has_value(), "Result should have value");
                    inst.operands = {Operand::imm(*res, lhs.type)};
                    break;
                }
                case IROp::FAdd:
                case IROp::FSub:
                case IROp::FMul:
                case IROp::FDiv: {
                    auto res =
                        fold_float_op(inst.op, lhs.as_float(), rhs.as_float());

                    expect(res.has_value(), "Result should have value");
                    inst.operands = {Operand::imm(*res, lhs.type)};
                    break;
                }
                default:
                    std::unreachable();
                }
            } else if (inst.operands.size() == 3) {
                auto lhs = inst.operands[1].as_imm();
                auto rhs = inst.operands[2].as_imm();

                if (inst.op == IROp::ICmp) {
                    auto cc = inst.operands[0].as_intcc();
                    auto lhs_int = lhs.as_int();
                    auto rhs_int = rhs.as_int();

                    auto res = lhs_int.cmp(rhs_int, cc);

                    inst.operands = {Operand::imm(res, type::builtin::BOOL)};
                } else if (inst.op == IROp::FCmp) {
                    auto cc = inst.operands[0].as_floatcc();
                    auto lhs_float = lhs.as_float();
                    auto rhs_float = rhs.as_float();

                    auto res = lhs_float.cmp(rhs_float, cc);

                    inst.operands = {Operand::imm(res, type::builtin::BOOL)};
                } else {
                    panic("Invalid number of operands for inst");
                }
            }

            changed = true;
            inst.op = IROp::LoadConst;
        }

        return changed;
    }
};
} // namespace z::ir
