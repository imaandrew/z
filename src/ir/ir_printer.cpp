#include "ir_printer.h"
#include "core/colour.h"
#include "core/zctxt.h"
#include "ir/condition_codes.h"
#include "ir/ir.h"
#include "ir/ir_builder.h"
#include <cstddef>
#include <ostream>
#include <print>
#include <string>
#include <utility>

namespace z::ir {

void IRPrinter::dump(const IRFile& ir, const ZContext& ctxt, std::ostream& os) {
    this->ir = &ir;
    this->ctxt = &ctxt;
    for (const auto& func : ir.funcs) {
        dump_ir(func, os);
    }
}

void IRPrinter::dump_ir(const IRFunction& func, std::ostream& os) const {
    std::print(os, "{}func{} {}@{}{}(", colour::BOLD, colour::RESET,
               colour::BOLD_GREEN, ctxt->strings->get_string(func.name),
               colour::RESET);

    for (std::size_t i = 0; i < func.params.size(); i++) {
        if (i > 0)
            os << ", ";

        std::print(os, "{}%{}{}: {}", colour::CYAN, func.params[i].id,
                   colour::RESET,
                   ctxt->ty->get(func.params[i].type)->basic_name(ctxt));
    }

    std::println(os, ") -> {}{{",
                 ctxt->ty->get(func.return_type)->basic_name(ctxt));

    for (const auto& block : func.blocks) {
        std::print(os, "{}bb{}:{}", colour::BOLD_YELLOW, block.id.id,
                   colour::RESET);

        if (!block.predecessors.empty()) {
            std::print("    {}; preds:", colour::GRAY);
            for (auto pred : block.predecessors)
                std::print(os, " bb{}", pred.id);
            os << colour::RESET;
        }

        os << "\n";

        for (auto inst_id : block.insts) {
            const auto& inst = func.insts[inst_id.id];
            os << "  ";
            dump_inst(inst, os);
            os << "\n";
        }

        os << "\n";
    }
    os << "}\n\n";
}

void IRPrinter::dump_inst(const Instruction& inst, std::ostream& os) const {
    if (inst.dest) {
        std::print(os, "{}%{}{}: {} = ", colour::CYAN, inst.dest.value().id,
                   colour::RESET,
                   ctxt->ty->get(inst.dest->type)->basic_name(ctxt));
    }

    os << colour::BOLD_MAGENTA << ir_op_to_string(inst.op) << colour::RESET;

    if (inst.op == IROp::Phi) {
        for (std::size_t i = 0; i < inst.operands.size(); i += 2) {
            os << (i == 0 ? " " : ", ");
            os << "[ ";
            dump_operand(inst.operands[i], os);
            os << ", ";
            dump_operand(inst.operands[i + 1], os);
            os << " ]";
        }
        return;
    }

    if (inst.op == IROp::Call || inst.op == IROp::CallIndirect) {
        os << " ";
        dump_operand(inst.operands.front(), os);

        os << "(";

        for (std::size_t i = 1; i < inst.operands.size(); i++) {
            os << (i == 1 ? "" : ", ");
            dump_operand(inst.operands[i], os);
        }

        os << ")";
        return;
    }

    for (std::size_t i = 0; i < inst.operands.size(); i++) {
        os << (i == 0 ? " " : ", ");
        dump_operand(inst.operands[i], os);
    }
}

void IRPrinter::dump_operand(const Operand& op, std::ostream& os) const {
    if (op.is_reg())
        std::print(os, "{}%{}{}", colour::CYAN, op.as_reg().id, colour::RESET);
    else if (op.is_imm())
        dump_immediate(op.as_imm(), os);
    else if (op.is_label())
        std::print(os, "{}bb{}{}", colour::YELLOW, op.as_label().block_id.id,
                   colour::RESET);
    else if (op.is_field())
        std::print(os, "{}#{}{}", colour::YELLOW, op.as_field().idx,
                   colour::RESET);
    else if (op.is_intcc()) {
        const auto* string = [&] {
            switch (op.as_intcc()) {
            case IntCC::Equal:
                return "eq";
            case IntCC::NotEqual:
                return "ne";
            case IntCC::UnsignedGreaterThan:
                return "ugt";
            case IntCC::UnsignedGreaterEqual:
                return "uge";
            case IntCC::UnsignedLessThan:
                return "ult";
            case IntCC::UnsignedLessEqual:
                return "ule";
            case IntCC::SignedGreaterThan:
                return "sgt";
            case IntCC::SignedGreaterEqual:
                return "sge";
            case IntCC::SignedLessThan:
                return "slt";
            case IntCC::SignedLessEqual:
                return "sle";
            }

            std::unreachable();
        }();
        os << string;
    } else if (op.is_floatcc()) {
        const auto* string = [&] {
            switch (op.as_floatcc()) {
            case FloatCC::Equal:
                return "eq";
            case FloatCC::NotEqual:
                return "ne";
            case FloatCC::GreaterThan:
                return "gt";
            case FloatCC::GreaterEqual:
                return "ge";
            case FloatCC::LessThan:
                return "lt";
            case FloatCC::LessEqual:
                return "le";
            }
            std::unreachable();
        }();
        os << string;
    } else if (op.is_func()) {
        std::print(os, "{}@{}{}", colour::BOLD_GREEN,
                   ctxt->strings->get_string(ir->funcs[op.as_func().id].name),
                   colour::RESET);
    }
}

void IRPrinter::dump_immediate(const Immediate& imm, std::ostream& os) const {
    if (imm.is_int()) {
        const auto& i = imm.as_int();
        std::print(os, "{}{}{}", colour::YELLOW,
                   i.is_negative() ? i.get_signed() : i.get_bits(),
                   colour::RESET);
    } else if (imm.is_float()) {
        const auto& f = imm.as_float();
        std::print(os, "{}{}{}", colour::YELLOW, f.get_bits(), colour::RESET);
    } else if (imm.is_bool()) {
        auto b = imm.as_bool();
        os << colour::YELLOW << (b ? "true" : "false") << colour::RESET;
    } else if (imm.is_string()) {
        std::print(os, "{}\"{}\"{}", colour::YELLOW,
                   ctxt->strings->get_string(imm.as_string()), colour::RESET);
    } else if (imm.is_char()) {
        std::print(os, "{}'{}'{}", colour::YELLOW, imm.as_char(),
                   colour::RESET);
    }
}

void IRPrinter::dump_terminator(TerminatorKind term, std::ostream& os) {
    switch (term) {
    case TerminatorKind::Branch:
        os << "branch";
        break;
    case TerminatorKind::Return:
        os << "return";
        break;
    case TerminatorKind::Jump:
        os << "jump";
        break;
    case TerminatorKind::Unreachable:
        os << "unreachable";
        break;
    }
}

constexpr std::string IRPrinter::ir_op_to_string(IROp op) {
    switch (op) {
    case IROp::IAdd:
        return "iadd";
    case IROp::ISub:
        return "isub";
    case IROp::IMul:
        return "imul";
    case IROp::SDiv:
        return "sdiv";
    case IROp::UDiv:
        return "udiv";
    case IROp::IMod:
        return "imod";
    case IROp::INeg:
        return "ineg";
    case IROp::FAdd:
        return "fadd";
    case IROp::FSub:
        return "fsub";
    case IROp::FMul:
        return "fmul";
    case IROp::FDiv:
        return "fdiv";
    case IROp::FNeg:
        return "fneg";
    case IROp::And:
        return "and";
    case IROp::Or:
        return "or";
    case IROp::Xor:
        return "xor";
    case IROp::Not:
        return "not";
    case IROp::Shl:
        return "shl";
    case IROp::Lsr:
        return "lsr";
    case IROp::Asr:
        return "asr";
    case IROp::ICmp:
        return "icmp";
    case IROp::FCmp:
        return "fcmp";
    case IROp::Alloca:
        return "alloca";
    case IROp::Load:
        return "load";
    case IROp::Store:
        return "store";
    case IROp::LoadConst:
        return "loadconst";
    case IROp::StoreConst:
        return "storeconst";
    case IROp::GetElementPtr:
        return "getelementptr";
    case IROp::GetFieldPtr:
        return "getfieldptr";
    case IROp::ExtractField:
        return "extractfield";
    case IROp::InsertField:
        return "insertfield";
    case IROp::ArrayInit:
        return "arrayinit";
    case IROp::StructInit:
        return "structinit";
    case IROp::TupleInit:
        return "tupleinit";
    case IROp::Call:
        return "call";
    case IROp::CallIndirect:
        return "callindirect";
    case IROp::Branch:
        return "br";
    case IROp::Jump:
        return "jump";
    case IROp::Ret:
        return "ret";
    case IROp::Arg:
        return "arg";
    case IROp::Phi:
        return "phi";
    case IROp::Dead:
        return "dead";
    }
    std::unreachable();
}

} // namespace z::ir
