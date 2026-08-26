#include "verifier.h"
#include "core/types.h"
#include "ir/bit_vec.h"
#include "ir/ir.h"
#include <algorithm>
#include <print>
#include <ranges>
#include <set>

namespace z::ir {

bool IRVerifier::verify_uses_table(const IRFunction& func) {
    const auto check_use = [func](VReg v, IRFunction::InstRef use) {
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
                             func.id.id, v.id, use.inst.id, use.block.id);
            }
        }
    }

    return ret;
}

bool IRVerifier::verify_uses_in_table(const IRFunction& func) {
    const auto check_use = [func](const VReg r, const BlockID block,
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
                            func.id.id, v.id, inst_id.id, block.block_id.id);
                    }
                }
            } else if (inst.op == IROp::ParallelCopy) {
                for (const auto& [dest, src] : inst.copy_pairs()) {
                    if (!check_use(src, block.id, inst_id)) {
                        ret = false;
                        std::println(
                            "func {}, vreg {}: missing use in table at "
                            "inst {} block {}",
                            func.id.id, src.id, inst_id.id, block.id.id);
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
                            func.id.id, op.as_reg().id, inst_id.id,
                            block.id.id);
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
                         func.id.id, v.id, def.inst.id, def.block.id);
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
                                 func.id.id, v.id, inst_id.id, block.id.id);
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
                        func.id.id, block.id.id, phis_left);
                }
                continue;
            }

            if (phis_left == 0) {
                ret = false;
                std::println("func {}, block {}: phi inst with 0 phis left",
                             func.id.id, block.id.id);
            } else {
                phis_left--;
            }

            if (phi.operands.size() != block.predecessors.size() * 2) {
                ret = false;
                std::println("func {}, block {}: phi should have one set "
                             "of operands for each pred block",
                             func.id.id, block.id.id);
            }
            for (const auto& [_, phi_label] : phi.phi_operands()) {
                auto it = preds.extract(phi_label.block_id.id);
                if (it.empty()) {
                    ret = false;
                    std::println("func {}, block {}: phi references block {} "
                                 "but not in preds list",
                                 func.id.id, block.id.id,
                                 phi_label.block_id.id);
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
                                 func.id.id, block.id.id, inst_id.id);
                else
                    std::println("func {}, block {}: contains inst id {} that "
                                 "belongs to another block",
                                 func.id.id, block.id.id, inst_id.id);
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
                func.id.id, id);
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
                                 func.id.id, block.id.id, inst_id.id);
                }
                continue;
            }

            if (inst_id == block.terminator() ||
                static_cast<usize>(i) == block.insts.size() - 1) {
                ret = false;
                std::println("func {}, block {}: block doesn't contain "
                             "terminator, last instruction: {}",
                             func.id.id, block.id.id, inst_id.id);
            }
        }
    }

    return ret;
}

bool IRVerifier::verify(const IRFunction& func) {
    bool ret = verify_uses_table(func);
    ret &= verify_uses_in_table(func);
    ret &= verify_def_table(func);
    ret &= verify_defs_in_table(func);
    ret &= verify_phi_insts(func);
    ret &= verify_no_unused_insts(func);
    ret &= verify_last_inst_is_term(func);

    return ret;
}

bool IRVerifier::verify_out_of_ssa(const IRFunction& func) {
    bool ret = true;

    for (const auto& inst : func.insts) {
        if (inst.op == IROp::Phi) {
            ret = false;
            std::println("func {}: still contains phi inst {} after out of ssa",
                         func.id.id, inst.id.id);
        } else if (inst.op == IROp::ParallelCopy) {
            ret = false;
            std::println("func {}: still contains parallel copy inst {} after "
                         "out of ssa",
                         func.id.id, inst.id.id);
        }
    }

    return ret;
}

} // namespace z::ir
