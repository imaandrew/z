#include "verifier.h"
#include "core/panic.h"
#include "core/types.h"
#include "ir/bit_vec.h"
#include "ir/ir.h"
#include "ir/ir_ops.h"
#include "type/type.h"
#include "type/type_arena.h"
#include "type/type_ref.h"
#include <algorithm>
#include <optional>
#include <print>
#include <ranges>
#include <set>
#include <type_traits>
#include <utility>

namespace z::ir {

bool IRVerifier::verify_uses_table(const IRFunction& func) {
    const auto check_use = [&func](VReg v, IRFunction::InstRef use) {
        const auto& inst = func.insts[use.inst.id];
        return std::ranges::any_of(inst.operands, [v](auto op) {
            return op.is_reg() && op.as_reg().id == v.id;
        });
    };

    bool ret = true;

    for (u32 i = 0; i < func.num_regs(); i++) {
        auto v = VReg{.id = i, .type = {}};
        const auto& uses = func.get_uses(v);
        for (const auto use : uses) {
            if (!check_use(v, use)) {
                ret = false;
                std::println("func {}, vreg {}: declared nonexistant use at "
                             "inst {} block {} ",
                             func.id, v.id, use.inst.id, use.block.id);
            }
        }
    }

    return ret;
}

bool IRVerifier::verify_uses_in_table(const IRFunction& func) {
    const auto check_use = [&func](const VReg r, const BlockID block,
                                   const InstId inst) {
        const auto& uses = func.get_uses(r);
        return std::ranges::any_of(uses, [block, inst](const auto& use) {
            return use.block == block && use.inst == inst;
        });
    };

    bool ret = true;

    for (const auto& block : func.blocks) {
        for (const auto inst_id : block.insts) {
            const auto& inst = func.insts[inst_id.id];
            if (inst.op == IROp::Dead)
                continue;

            if (inst.op == IROp::Phi) {
                for (const auto& [v, block] : inst.phi_operands()) {
                    if (!check_use(v, block.block_id, inst_id)) {
                        ret = false;
                        std::println(
                            "func {}, vreg {}: missing use in table at "
                            "inst {} block {}",
                            func.id, v.id, inst_id.id, block.block_id.id);
                    }
                }
            } else if (inst.op == IROp::ParallelCopy) {
                for (const auto& [dest, src] : inst.copy_pairs()) {
                    if (!check_use(src, block.id, inst_id)) {
                        ret = false;
                        std::println(
                            "func {}, vreg {}: missing use in table at "
                            "inst {} block {}",
                            func.id, src.id, inst_id.id, block.id);
                    }
                }
            } else {
                for (const auto& op : inst.operands) {
                    if (op.is_reg() &&
                        !check_use(op.as_reg(), block.id, inst_id)) {
                        ret = false;
                        std::println(
                            "func {}, vreg {}: missing use in table at "
                            "inst {} block {}",
                            func.id, op.as_reg().id, inst_id.id, block.id);
                    }
                }
            }
        }
    }

    return ret;
}

bool IRVerifier::verify_def_table(const IRFunction& func) {
    bool ret = true;

    for (u32 i = 0; i < func.num_regs(); i++) {
        auto v = VReg{.id = i, .type = {}};
        if (!func.has_def(v))
            continue;
        const auto& def = func.get_def(v);
        const auto& inst = func.insts[def.inst.id];
        const auto defines =
            inst.op == IROp::ParallelCopy
                ? std::ranges::any_of(
                      inst.copy_pairs(),
                      [&](const auto p) { return p.first.id == v.id; })
                : (inst.dest && inst.dest.value().id == v.id);
        if (!defines) {
            ret = false;
            std::println("func {}, vreg {}: declared nonexistant def at "
                         "inst {} block {} ",
                         func.id, v.id, def.inst.id, def.block.id);
        }
    }

    return ret;
}

bool IRVerifier::verify_defs_in_table(const IRFunction& func) {
    bool ret = true;

    for (const auto& block : func.blocks) {
        for (const auto inst_id : block.insts) {
            const auto& inst = func.insts[inst_id.id];

            const auto check_def = [&](VReg v) {
                const auto defines = std::ranges::any_of(
                    func.get_defs(v), [&](const IRFunction::InstRef def) {
                        return def.inst == inst_id && def.block == block.id;
                    });
                if (!defines) {
                    ret = false;
                    std::println("func {}, vreg {}: missing def in table at "
                                 "inst {} block {}",
                                 func.id, v.id, inst_id.id, block.id);
                }
            };

            if (inst.op == IROp::ParallelCopy) {
                for (const auto& [dest, _] : inst.copy_pairs()) {
                    check_def(dest);
                }
            } else if (inst.dest) {
                check_def(inst.dest.value());
            }
        }
    }

    return ret;
}

bool IRVerifier::verify_phi_insts(const IRFunction& func) {
    bool ret = true;

    for (const auto& block : func.blocks) {
        auto phis_left = block.num_phis;

        for (const auto phi_id : block.insts) {
            std::set<u32> preds;
            for (const auto p : block.predecessors) {
                preds.insert(p.id);
            }

            const auto& phi = func.insts[phi_id.id];

            if (phi.op != IROp::Phi) {
                if (phis_left != 0) {
                    ret = false;
                    std::println(
                        "func {}, block {}: non phi inst with {} phis left",
                        func.id, block.id, phis_left);
                }
                continue;
            }

            if (phis_left == 0) {
                ret = false;
                std::println("func {}, block {}: phi inst with 0 phis left",
                             func.id, block.id);
            } else {
                phis_left--;
            }

            if (phi.operands.size() != block.predecessors.size() * 2) {
                ret = false;
                std::println("func {}, block {}: phi should have one set "
                             "of operands for each pred block",
                             func.id, block.id);
            }
            for (const auto& [_, phi_label] : phi.phi_operands()) {
                const auto it = preds.extract(phi_label.block_id.id);
                if (it.empty()) {
                    ret = false;
                    std::println("func {}, block {}: phi references block {} "
                                 "but not in preds list",
                                 func.id, block.id, phi_label.block_id.id);
                }
            }
        }
    }

    return ret;
}

bool IRVerifier::verify_no_unused_insts(const IRFunction& func) {
    bool ret = true;

    auto insts = BitVector(func.insts.size());
    for (const auto& inst : func.insts) {
        if (inst.op != IROp::Dead)
            insts.set(inst.id.id);
    }

    for (const auto& block : func.blocks) {
        for (const auto inst_id : block.insts) {
            const auto& inst = func.get_inst(inst_id);
            if (inst.op == IROp::Dead)
                continue;

            if (!insts.test(inst_id.id)) {
                ret = false;
                if (inst_id.id >= func.insts.size())
                    std::println("func {}, block {}: contains inst id {} that "
                                 "does not belong to func",
                                 func.id, block.id, inst_id.id);
                else
                    std::println("func {}, block {}: contains inst id {} that "
                                 "belongs to another block",
                                 func.id, block.id, inst_id.id);
                continue;
            }

            insts.unset(inst_id.id);
        }
    }

    if (insts.count() > 0) {
        ret = false;
        insts.for_each_set([&func](auto id) {
            std::println(
                "func {}: contains inst id {} that isn't used in any block",
                func.id, id);
        });
    }

    return ret;
}

bool IRVerifier::verify_last_inst_is_term(const IRFunction& func) {
    bool ret = true;

    for (const auto& block : func.blocks) {
        for (const auto [i, inst_id] : std::views::enumerate(block.insts)) {
            const auto& inst = func.get_inst(inst_id);

            if (inst.op == IROp::Jump || inst.op == IROp::Branch ||
                inst.op == IROp::Ret) {
                if (inst_id != block.terminator() ||
                    static_cast<usize>(i) != block.insts.size() - 1) {
                    ret = false;
                    std::println("func {}, block {}: terminator instruction {} "
                                 "not at end of block",
                                 func.id, block.id, inst_id.id);
                }
                continue;
            }

            if (inst_id == block.terminator() ||
                static_cast<usize>(i) == block.insts.size() - 1) {
                ret = false;
                std::println("func {}, block {}: block doesn't contain "
                             "terminator, last instruction: {}",
                             func.id, block.id, inst_id.id);
            }
        }
    }

    return ret;
}

bool IRVerifier::verify_inst_operands(const IRFunction& func,
                                      const type::TypeArena& ty) {
    bool ret = true;

    const auto kind_of = [](const Operand& op) {
        if (op.is_reg())
            return OpKind::Reg;
        if (op.is_imm())
            return OpKind::Imm;
        if (op.is_label())
            return OpKind::Label;
        if (op.is_index())
            return OpKind::Index;
        if (op.is_intcc())
            return OpKind::IntCC;
        if (op.is_floatcc())
            return OpKind::FloatCC;
        if (op.is_func())
            return OpKind::FuncID;
        if (op.is_type())
            return OpKind::Type;
        return OpKind::Err;
    };

    const auto type_of = [](const Operand& op) {
        if (op.is_reg())
            return op.as_reg().type;
        if (op.is_imm())
            return op.as_imm().type;
        return type::builtin::UNINITALIZED;
    };

    const auto kind_matches = [](OpKind allowed, OpKind kind) {
        using T = std::underlying_type_t<OpKind>;
        return (static_cast<T>(allowed) & static_cast<T>(kind)) != 0;
    };

    const auto type_matches = [&ty](Type expected, type::TypeRef type) {
        switch (expected) {
        case Type::None:
        case Type::Any:
            return true;
        case Type::Int:
            return ty.isa<type::IntegerType>(type);
        case Type::Float:
            return ty.isa<type::FloatType>(type);
        case Type::Bool:
            return ty.isa<type::BooleanType>(type);
        case Type::Type:
            return ty.isa<type::TypeType>(type);
        case Type::Pointer:
            return ty.isa<type::PointerType>(type);
        case Type::Aggregate:
            return ty.isa<type::ArrayType>(type) ||
                   ty.isa<type::StructType>(type) ||
                   ty.isa<type::TupleType>(type);
        }
        std::unreachable();
    };

    const auto elem_type =
        [&ty](type::TypeRef type) -> std::optional<type::TypeRef> {
        if (const auto* arr = ty.get_as<type::ArrayType>(type))
            return arr->get_type();
        return std::nullopt;
    };

    for (const auto& inst : func.insts) {
        if (inst.op == IROp::Dead)
            continue;

        const auto& info = op_info(inst.op);

        const auto check_operand = [&](const Operand& op) {
            if (op.is_reg()) {
                if (op.as_reg().type != inst.dest->type) {
                    ret = false;
                    std::println("func {}, inst {}: operand type != dest type",
                                 func.id, inst.id);
                }
            } else if (op.is_imm()) {
                if (op.as_imm().type != inst.dest->type) {
                    ret = false;
                    std::println("func {}, inst {}: operand type != dest type",
                                 func.id, inst.id);
                }
            } else {
                panic("must have reg or imm operands");
            }
        };

        const auto check_operand_list = [&] {
            if (inst.op == IROp::Ret && inst.operands.empty())
                return true;

            if (inst.operands.size() != info.operands.size()) {
                std::println("func {}, inst {}: expected {} operands, got {}",
                             func.id, inst.id, info.operands.size(),
                             inst.operands.size());
                return false;
            }

            bool ok = true;

            for (usize i = 0; i < inst.operands.size(); i++) {
                const auto& op = inst.operands[i];

                if (!kind_matches(info.operands[i].kind, kind_of(op))) {
                    ok = false;
                    std::println("func {}, inst {}: operand {} isn't a valid "
                                 "kind of operand for this op",
                                 func.id, inst.id, i);
                    continue;
                }

                if (!type_matches(info.operands[i].type, type_of(op))) {
                    ok = false;
                    std::println("func {}, inst {}: operand {} has the wrong "
                                 "type for this op",
                                 func.id, inst.id, i);
                }
            }

            return ok;
        };

        if (info.ret == DestType::Custom) {
            if (inst.op == IROp::Phi) {
                if (!inst.dest) {
                    ret = false;
                    std::println("func {}, inst {}: phi should return a value",
                                 func.id, inst.id);
                    continue;
                }

                const auto dest_type = inst.dest->type;
                for (const auto& [v, _] : inst.phi_operands()) {
                    if (v.type != dest_type) {
                        ret = false;
                        std::println("func {}, inst {}: phi operand {} type != "
                                     "dest type",
                                     func.id, inst.id, v.id);
                    }
                }
            } else if (inst.op == IROp::ParallelCopy) {
                for (const auto& [dest, src] : inst.copy_pairs()) {
                    if (dest.type != src.type) {
                        ret = false;
                        std::println("func {}, inst {}: copy of vreg {} to "
                                     "vreg {} changes type",
                                     func.id, inst.id, src.id, dest.id);
                    }
                }
            }

            continue;
        }

        if (!check_operand_list()) {
            ret = false;
            continue;
        }

        if (info.ret == DestType::None) {
            if (inst.dest) {
                ret = false;
                std::println("func {}, inst {}: should not return value",
                             func.id, inst.id);
            }
            continue;
        }

        if (!inst.dest) {
            ret = false;
            std::println("func {}, inst {}: should return a value", func.id,
                         inst.id);
            continue;
        }

        if (info.ret == DestType::SameArgs) {
            for (const auto& op : inst.operands) {
                check_operand(op);
            }
        } else if (info.ret == DestType::Bool) {
            if (inst.dest->type != type::builtin::BOOL) {
                ret = false;
                std::println("func {}, inst {}: dest type should be bool",
                             func.id, inst.id);
            }
        } else if (info.ret == DestType::Int) {
            if (!ty.get_as<type::IntegerType>(inst.dest->type)) {
                ret = false;
                std::println("func {}, inst {}: dest type should be an integer",
                             func.id, inst.id);
            }
        } else if (info.ret == DestType::SameOp0) {
            const auto& op = inst.operands.front();
            check_operand(op);
        } else if (info.ret == DestType::PtrToOp0) {
            const auto* t = ty.get_as<type::PointerType>(inst.dest->type);
            if (!t || !inst.operands.front().is_type() ||
                t->get_type() != inst.operands.front().as_type()) {
                ret = false;
                std::println("func {}, inst {}: dest type should be a pointer "
                             "to the first operand",
                             func.id, inst.id);
            }
        } else if (info.ret == DestType::PointeeOp0) {
            const auto* t =
                ty.get_as<type::PointerType>(type_of(inst.operands.front()));
            if (!t || t->get_type() != inst.dest->type) {
                ret = false;
                std::println("func {}, inst {}: dest type should be the type "
                             "pointed to by the first operand",
                             func.id, inst.id);
            }
        } else if (info.ret == DestType::ElemOp0) {
            const auto elem = elem_type(type_of(inst.operands.front()));
            if (!elem || *elem != inst.dest->type) {
                ret = false;
                std::println("func {}, inst {}: dest type should be the "
                             "element type of the first operand",
                             func.id, inst.id);
            }
        } else if (info.ret == DestType::PtrToElem0) {
            const auto elem = elem_type(type_of(inst.operands.front()));
            const auto* t = ty.get_as<type::PointerType>(inst.dest->type);
            if (!elem || !t || t->get_type() != *elem) {
                ret = false;
                std::println("func {}, inst {}: dest type should be a pointer "
                             "to the element type of the first operand",
                             func.id, inst.id);
            }
        } else if (info.ret == DestType::RetOp0) {
            const auto* t =
                ty.get_as<type::FunctionType>(type_of(inst.operands.front()));
            if (!t || t->get_return_val() != inst.dest->type) {
                ret = false;
                std::println("func {}, inst {}: dest type should be the return "
                             "type of the first operand",
                             func.id, inst.id);
            }
        }
    }

    return ret;
}

bool IRVerifier::verify(const IRFunction& func, const type::TypeArena& ty) {
    bool ret = verify_uses_table(func);
    ret &= verify_uses_in_table(func);
    ret &= verify_def_table(func);
    ret &= verify_defs_in_table(func);
    ret &= verify_phi_insts(func);
    ret &= verify_no_unused_insts(func);
    ret &= verify_last_inst_is_term(func);
    ret &= verify_inst_operands(func, ty);

    return ret;
}

bool IRVerifier::verify_out_of_ssa(const IRFunction& func) {
    bool ret = true;

    for (const auto& inst : func.insts) {
        if (inst.op == IROp::Phi) {
            ret = false;
            std::println("func {}: still contains phi inst {} after out of ssa",
                         func.id, inst.id);
        } else if (inst.op == IROp::ParallelCopy) {
            ret = false;
            std::println("func {}: still contains parallel copy inst {} after "
                         "out of ssa",
                         func.id, inst.id);
        }
    }

    return ret;
}

} // namespace z::ir
