#pragma once

#include "ir/dom_tree.h"
#include "ir/ir.h"
#include "ir/liveness.h"
#include "ir/pass.h"
#include "ir/union_find.h"
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace z::ir {

class OutOfSSAPass : public IRPass {
    IRFunction* func;
    std::unique_ptr<LiveCheck> live;

    struct Copy {
        VReg dest;
        VReg src;
    };

    std::vector<std::vector<Copy>> block_copies;

    VReg new_reg(VReg like, BlockID block) {
        auto reg_id = func->vreg_info.size();
        func->vreg_info.emplace_back(InstId(UINT32_MAX), block);
        return VReg{.id = static_cast<std::uint32_t>(reg_id),
                    .type = like.type};
    }

    void collect_phi_copies() {
        for (auto& block : func->blocks) {
            for (auto p : block.phis) {
                auto& phi = func->insts[p.id];
                auto phi_dest = phi.dest.value();

                for (std::size_t i = 0; i + 1 < phi.operands.size(); i += 2) {
                    auto src_reg = phi.operands[i].as_reg();
                    auto pred_id = phi.operands[i + 1].as_label().block_id;

                    block_copies[pred_id.id].push_back(
                        {.dest = phi_dest, .src = src_reg});
                }

                phi.op = IROp::Dead;
            }
        }
    }

    void coalesce() {
        UnionFind uf(func->vreg_info.size());

        auto merge_conflicts = [&](std::uint32_t a, std::uint32_t b) -> bool {
            for (auto& copies : block_copies) {
                for (std::size_t i = 0; i < copies.size(); ++i) {
                    auto dest_i = uf.find(copies[i].dest.id);
                    auto src_i = uf.find(copies[i].src.id);
                    if (dest_i == a)
                        dest_i = b;

                    for (std::size_t j = i + 1; j < copies.size(); ++j) {
                        auto dest_j = uf.find(copies[j].dest.id);
                        auto src_j = uf.find(copies[j].src.id);
                        if (dest_j == a)
                            dest_j = b;

                        if (dest_i == dest_j && src_i != src_j)
                            return true;
                    }
                }
            }
            return false;
        };

        for (auto& copies : block_copies) {
            for (auto& c : copies) {
                auto rd = uf.find(c.dest.id);
                auto rs = uf.find(c.src.id);
                if (rd == rs)
                    continue;

                if (live->is_live_at_def(c.src, c.dest) ||
                    live->is_live_at_def(c.dest, c.src))
                    continue;

                if (merge_conflicts(rd, rs))
                    continue;

                uf.merge(c.dest.id, c.src.id);
            }
        }

        for (auto& block : func->blocks) {
            for (auto inst_id : block.all_insts()) {
                auto& inst = func->insts[inst_id.id];
                if (inst.op == IROp::Dead)
                    continue;
                if (inst.dest)
                    inst.dest->id = uf.find(inst.dest->id);
                for (auto& op : inst.operands) {
                    if (op.is_reg())
                        op.as_reg().id = uf.find(op.as_reg().id);
                }
            }
        }

        for (auto& copies : block_copies) {
            std::erase_if(copies, [&](const Copy& c) {
                return uf.find(c.dest.id) == uf.find(c.src.id);
            });

            for (auto& c : copies) {
                c.dest.id = uf.find(c.dest.id);
                c.src.id = uf.find(c.src.id);
            }
        }
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

    InstId emit_copy(VReg dest, VReg src, BlockID block) {
        auto inst_id = InstId(func->insts.size());
        func->insts.emplace_back(inst_id, IROp::Copy, dest,
                                 std::initializer_list<Operand>{{src}});
        func->get_reg_info(dest).def = std::make_pair(inst_id, block);
        func->get_reg_info(src).uses.emplace_back(inst_id, block);
        return inst_id;
    }

    void emit_sequential_copies() {
        for (std::size_t i = 0; i < func->blocks.size(); i++) {
            auto& block = func->blocks[i];
            auto& copies = block_copies[i];

            auto seq = sequentialize(copies, block.id);

            if (!seq.empty()) {
                std::vector<InstId> new_insts;
                new_insts.reserve(block.insts.size() + seq.size());

                for (auto id : block.insts)
                    new_insts.push_back(id);

                for (auto& c : seq)
                    new_insts.push_back(emit_copy(c.dest, c.src, block.id));

                block.insts = std::move(new_insts);
            }

            block.phis.clear();
        }
    }

public:
    [[nodiscard]] const char* name() const override { return "OutOfSSAPass"; }

    bool run(IRFunction& func) override {
        this->func = &func;
        auto dom = DominatorTree::build(func.blocks);
        live = std::make_unique<LiveCheck>(func, dom);
        block_copies.resize(func.blocks.size());

        collect_phi_copies();
        coalesce();
        emit_sequential_copies();

        return true;
    }
};
} // namespace z::ir
