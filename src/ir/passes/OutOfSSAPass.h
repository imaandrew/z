#pragma once

#include "ir/dom_tree.h"
#include "ir/ir.h"
#include "ir/liveness.h"
#include "ir/merge_set.h"
#include "ir/pass.h"
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace z::ir {

/*
    OutOfSSA pass implements algorithms described in:
    Boissinot, Benoit & Darte, Alain & Rastello, Fabrice & Dinechin, Benoît &
    Guillon, Christophe. (2009). Revisiting Out-of-SSA Translation for
    Correctness, Code Quality, and Efficiency. Proceedings of the 2009 CGO - 7th
    International Symposium on Code Generation and
    Optimization. 10.1109/CGO.2009.19.
    */

class OutOfSSAPass : public IRPass {
    IRFunction* func;

    struct Copy {
        VReg dest;
        VReg src;
    };

    struct BlockCopies {
        std::vector<Copy> start;
        std::vector<Copy> end;
    };

    std::vector<BlockCopies> block_copies;

    VReg new_reg(VReg like, BlockID block) {
        auto reg_id = func->vreg_info.size();
        func->vreg_info.emplace_back(InstId(UINT32_MAX), block);
        return VReg{.id = static_cast<std::uint32_t>(reg_id),
                    .type = like.type};
    }

    void insert_phi_copies() {
        for (auto& block : func->blocks) {
            for (auto p : block.phis) {
                auto& phi = func->insts[p.id];

                for (std::size_t i = 0; i + 1 < phi.operands.size(); i += 2) {
                    auto phi_reg = phi.operands[i].as_reg();
                    auto pred_id = phi.operands[i + 1].as_label().block_id;

                    auto phi_reg_copy = new_reg(phi_reg, pred_id);

                    block_copies[pred_id.id].end.emplace_back(phi_reg_copy,
                                                              phi_reg);

                    func->get_reg_info(phi_reg_copy)
                        .uses.emplace_back(phi.id, block.id);
                    phi.operands[i] = Operand::reg(phi_reg_copy);
                    for (auto& use : func->get_reg_info(phi_reg).uses) {
                        if (use.inst == phi.id && use.block == pred_id) {
                            use = IRFunction::InstRef(InstId(UINT32_MAX),
                                                      pred_id);
                            break;
                        }
                    }
                }

                auto phi_dest = phi.dest.value();
                auto phi_copy = new_reg(phi_dest, block.id);

                phi.dest.emplace(phi_copy);
                func->get_reg_info(phi_copy).def.inst = phi.id;

                block_copies[block.id.id].start.emplace_back(phi_dest,
                                                             phi_copy);
                func->get_reg_info(phi_dest).def.inst = InstId(UINT32_MAX);
            }
        }
    }

    void calc_inst_order(std::vector<std::uint32_t>& inst_pos,
                         std::vector<std::uint32_t>& vreg_pos,
                         std::vector<std::uint32_t>& block_end_pos) {
        for (auto& b : func->blocks) {
            std::uint32_t pos = 0;

            for (auto i : b.phis) {
                auto& inst = func->insts[i.id];
                inst_pos[i.id] = pos;
                if (inst.dest)
                    vreg_pos[inst.dest->id] = pos;
                pos++;
            }

            for (auto& c : block_copies[b.id.id].start)
                vreg_pos[c.dest.id] = pos++;

            for (auto i : b.insts) {
                auto& inst = func->insts[i.id];
                inst_pos[i.id] = pos;
                if (inst.dest)
                    vreg_pos[inst.dest->id] = pos;
                pos++;
            }

            block_end_pos[b.id.id] = pos;

            for (auto& c : block_copies[b.id.id].end)
                vreg_pos[c.dest.id] = pos++;

            if (b.terminator) {
                inst_pos[b.terminator->id] = pos;
                pos++;
            }
        }
    }

    void coalesce(std::vector<std::uint32_t>& inst_pos,
                  std::vector<std::uint32_t>& vreg_pos,
                  std::vector<std::uint32_t>& block_end_pos) {
        auto dom = DominatorTree::build(func->blocks);
        auto live = LiveCheck(*func, dom);

        calc_inst_order(inst_pos, vreg_pos, block_end_pos);

        auto def_before = [&](VReg a, VReg b) -> bool {
            auto block_a = func->vreg_info[a.id].def.block;
            auto block_b = func->vreg_info[b.id].def.block;
            if (block_a != block_b)
                return dom.pre[block_a.id] < dom.pre[block_b.id];
            return vreg_pos[a.id] < vreg_pos[b.id];
        };

        auto get_use_pos =
            [&](const IRFunction::InstRef& use) -> std::uint32_t {
            if (use.inst.id == UINT32_MAX)
                return block_end_pos[use.block.id];
            return inst_pos[use.inst.id];
        };

        auto is_live_at = [&](VReg a, VReg b) -> bool {
            auto a_block = func->vreg_info[a.id].def.block;
            auto b_block = func->vreg_info[b.id].def.block;

            if (a_block != b_block) {
                if (!live.is_live_in(a, b_block))
                    return false;

                auto b_pos = vreg_pos[b.id];
                for (auto& use : func->vreg_info[a.id].uses) {
                    if (use.block == b_block && get_use_pos(use) > b_pos)
                        return true;
                }
                return live.is_live_out(a, b_block);
            }

            auto a_pos = vreg_pos[a.id];
            auto b_pos = vreg_pos[b.id];
            if (a_pos >= b_pos)
                return false;

            for (auto& use : func->vreg_info[a.id].uses) {
                if (use.block != a_block)
                    return true;
                if (get_use_pos(use) > b_pos)
                    return true;
            }
            return false;
        };

        auto interferes = [&](VReg a, VReg b) -> bool {
            return is_live_at(a, b) || is_live_at(b, a);
        };

        auto dominates = [&](VReg a, VReg b) -> bool {
            auto ba = func->vreg_info[a.id].def.block;
            auto bb = func->vreg_info[b.id].def.block;
            return dom.dominates(ba, bb) &&
                   (ba != bb || vreg_pos[a.id] <= vreg_pos[b.id]);
        };

        auto mgr =
            buildMergeSetManager<VReg>(def_before, interferes, dominates);

        for (auto& block : func->blocks) {
            for (auto p : block.phis) {
                auto& phi = func->insts[p.id];
                auto phi_dest = phi.dest.value();
                for (std::size_t i = 0; i + 1 < phi.operands.size(); i += 2) {
                    auto phi_src = phi.operands[i].as_reg();
                    mgr.merge(phi_dest, phi_src);
                }
            }
        }

        for (auto& copies : block_copies) {
            std::vector<Copy> remaining_start;
            std::vector<Copy> remaining_end;

            for (auto& c : copies.start) {
                if (!mgr.try_coalesce(c.dest, c.src))
                    remaining_start.push_back(c);
            }

            for (auto& c : copies.end) {
                if (!mgr.try_coalesce(c.dest, c.src))
                    remaining_end.push_back(c);
            }

            copies.start = std::move(remaining_start);
            copies.end = std::move(remaining_end);
        }

        for (auto& copies : block_copies) {
            auto rewrite = [&](std::vector<Copy>& list) {
                for (auto& c : list) {
                    c.dest = mgr.representative(c.dest);
                    c.src = mgr.representative(c.src);
                }
                std::erase_if(
                    list, [](const Copy& c) { return c.dest.id == c.src.id; });
            };

            rewrite(copies.start);
            rewrite(copies.end);
        }

        for (auto& inst : func->insts) {
            if (inst.dest)
                inst.dest = mgr.representative(*inst.dest);
            for (auto& op : inst.operands) {
                if (op.is_reg())
                    op = Operand::reg(mgr.representative(op.as_reg()));
            }
        }

        for (auto& p : func->params)
            p = mgr.representative(p);
    }

    std::vector<Copy> sequentialize(const std::vector<Copy>& copies,
                                    BlockID block) {
        if (copies.empty())
            return {};

        std::vector<std::uint32_t> ready;
        std::vector<std::uint32_t> to_do;
        std::vector<Copy> seq;

        std::unordered_map<std::uint32_t, std::uint32_t> loc;
        std::unordered_map<std::uint32_t, std::uint32_t> pred;
        std::unordered_map<std::uint32_t, VReg> vreg_map;

        for (auto [dest, src] : copies) {
            vreg_map[dest.id] = dest;
            vreg_map[src.id] = src;

            loc[src.id] = src.id;
            pred[dest.id] = src.id;
            to_do.push_back(dest.id);
        }

        for (auto [dest, _] : copies) {
            if (!loc.contains(dest.id))
                ready.push_back(dest.id);
        }

        while (!to_do.empty()) {
            while (!ready.empty()) {
                auto b = ready.back();
                ready.pop_back();

                auto a = loc[pred[b]];
                seq.push_back({.dest = vreg_map[b], .src = vreg_map[a]});

                loc[pred[b]] = b;
                if (pred[b] == a && pred.contains(a)) {
                    ready.push_back(a);
                }
            }

            if (!to_do.empty()) {
                auto b = to_do.back();
                to_do.pop_back();

                if (b != loc[pred[b]]) {
                    auto n = new_reg(vreg_map[b], block);
                    vreg_map[n.id] = n;

                    seq.push_back({.dest = n, .src = vreg_map[b]});
                    loc[b] = n.id;
                    ready.push_back(b);
                }
            }
        }

        return seq;
    }

    InstId emit_copy(VReg dest, VReg src) {
        auto inst_id = InstId(func->insts.size());
        func->insts.emplace_back(inst_id, IROp::Copy, dest,
                                 std::initializer_list<Operand>{{src}});
        return inst_id;
    }

    void emit_sequential_copies() {
        for (std::size_t i = 0; i < func->blocks.size(); i++) {
            auto& block = func->blocks[i];
            auto& copies = block_copies[i];

            auto start = sequentialize(copies.start, block.id);
            auto end = sequentialize(copies.end, block.id);

            if (!start.empty() || !end.empty()) {
                std::vector<InstId> new_insts;
                new_insts.reserve(block.insts.size() + start.size() +
                                  end.size());

                for (auto& c : start)
                    new_insts.push_back(emit_copy(c.dest, c.src));

                for (auto id : block.insts)
                    new_insts.push_back(id);

                for (auto& c : end)
                    new_insts.push_back(emit_copy(c.dest, c.src));

                block.insts = std::move(new_insts);
            }

            block.phis.clear();
        }
    }

    void fix_reg_info() {
        func->vreg_info =
            std::vector<IRFunction::VRegInfo>(func->vreg_info.size());

        for (auto& b : func->blocks) {
            for (auto i : b.all_insts()) {
                auto& inst = func->insts[i.id];

                if (inst.op == IROp::Dead)
                    continue;

                if (inst.dest)
                    func->get_reg_info(inst.dest.value()).def =
                        IRFunction::InstRef(i, b.id);

                for (auto& op : inst.operands) {
                    if (op.is_reg()) {
                        func->get_reg_info(op.as_reg())
                            .uses.emplace_back(i, b.id);
                    }
                }
            }
        }
    }

public:
    [[nodiscard]] const char* name() const override { return "OutOfSSAPass"; }

    bool run(IRFunction& func) override {
        this->func = &func;
        block_copies.resize(func.blocks.size());

        insert_phi_copies();

        auto inst_pos = std::vector<std::uint32_t>(func.insts.size());
        auto vreg_pos = std::vector<std::uint32_t>(func.vreg_info.size());
        auto block_end_pos = std::vector<std::uint32_t>(func.blocks.size());
        coalesce(inst_pos, vreg_pos, block_end_pos);

        emit_sequential_copies();
        fix_reg_info();

        return true;
    }
};
} // namespace z::ir
