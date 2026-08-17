#pragma once

#include "core/panic.h"
#include "core/string_pool.h"
#include "core/types.h"
#include "core/zctxt.h"
#include "ir.h"
#include "parser/ast.h"
#include "type/type_ref.h"
#include <initializer_list>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace z::ir {

struct IRFile {
    std::vector<IRFunction> funcs;

    explicit IRFile(std::vector<IRFunction> funcs) : funcs(std::move(funcs)) {}
};

class IRBuilder final : public ast::ASTVisitor {
    struct BlockBuildState {
        std::unordered_map<StringID, VReg> var_map;
        std::unordered_map<StringID, InstId> incomplete_phis;
        bool sealed = false;
    };

    enum class DeferredJumpType : u8 {
        Break,
        Continue,
    };

    struct DeferredJump {
        DeferredJumpType type;
        BlockID from_block;
        std::optional<Operand> value;

        DeferredJump(DeferredJumpType type, BlockID from_block,
                     std::optional<Operand> value)
            : type(type), from_block(from_block), value(value) {}
    };

    struct DeferredReturn {
        BlockID from_block;
        std::optional<Operand> value;

        DeferredReturn(BlockID from_block, std::optional<Operand> value)
            : from_block(from_block), value(value) {}
    };

    ZContext* ctxt;

    std::vector<IRFunction> funcs;
    std::optional<Operand> last_result;

    IRFunction* current_func{};
    std::unordered_map<BlockID, BlockBuildState> block_state;
    std::optional<BlockID> current_block;
    BlockBuildState* current_block_state{};

    std::vector<DeferredReturn> deferred_returns;
    std::vector<DeferredJump> deferred_loop_jumps;

    std::unordered_map<StringID, Operand> constants;

    void visit(ast::Identifier& ident) override;
    void visit(ast::IntExpr& expr) override;
    void visit(ast::FloatExpr& expr) override;
    void visit(ast::BoolExpr& expr) override;
    void visit(ast::PrefixExpr& expr) override;
    void visit(ast::PostfixExpr& expr) override;
    void visit(ast::BinaryExpr& expr) override;
    void visit(ast::CallExpr& expr) override;
    void visit(ast::ArrayExpr& expr) override;
    void visit(ast::FieldExpr& expr) override;
    void visit(ast::ArrayInitExpr& expr) override;
    void visit(ast::StructExprField& expr) override;
    void visit(ast::StructInitExpr& expr) override;
    void visit(ast::TupleExpr& expr) override;
    void visit(ast::Block& block) override;
    void visit(ast::Param& param) override;
    void visit(ast::SourceFileDecl& file) override;
    void visit(ast::FuncDecl& func) override;
    void visit(ast::BreakStmt& stmt) override;
    void visit(ast::ContinueStmt& stmt) override;
    void visit(ast::ForExpr& expr) override;
    void visit(ast::LetStmt& stmt) override;
    void visit(ast::ReturnStmt& stmt) override;
    void visit(ast::IfExpr& expr) override;
    void visit(ast::ElseExpr& expr) override;
    void visit(ast::LoopExpr& expr) override;
    void visit(ast::WhileExpr& expr) override;
    void visit(ast::StringExpr& expr) override;
    void visit(ast::CharExpr& expr) override;
    void visit(ast::StructField& /*field*/) override {}
    void visit(ast::StructDecl& /*decl*/) override {}
    void visit(ast::EnumField& /*field*/) override {}
    void visit(ast::EnumDecl& /*decl*/) override {}
    void visit(ast::ConstDecl& decl) override;
    void visit(ast::StaticDecl& decl) override;
    void visit(ast::TraitDecl& decl) override;
    void visit(ast::TypeAliasDecl& decl) override;
    void visit(ast::TraitFuncDecl& decl) override;

    IRFunction& get_func(FuncID id) { return funcs[id.id]; }
    IRFunction& get_func(StringID name) {
        for (auto& func : funcs) {
            if (func.name == name)
                return func;
        }
        panic("Could not find function: {}", ctxt->strings->get_string(name));
    }

    Instruction& get_inst(InstId id) { return current_func->insts[id.id]; }

    InstId get_inst_id() const { return InstId(current_func->insts.size()); }
    BlockID get_block_id() const {
        return BlockID(current_func->blocks.size());
    }
    FuncID get_func_id() const { return FuncID(funcs.size()); }

    VReg emit_reg(type::TypeRef type, InstId inst) {
        auto reg_id = current_func->add_reg(inst, current_block.value());
        return VReg{.id = reg_id, .type = type};
    }

    BlockID new_block() {
        auto id = get_block_id();
        current_func->blocks.emplace_back(id);
        block_state.try_emplace(id);
        return id;
    }

    void switch_block(BlockID block) {
        current_block = block;
        current_block_state = &block_state[block];
    }

    Operand emit_op(ast::Expr* e) {
        e->accept(*this);
        return last_result.value();
    }

    Operand emit_aggregate_insert(ast::Expr* lhs, Operand new_val);

    VReg emit_inst(IROp op, std::initializer_list<Operand> ops,
                   type::TypeRef dest_type) {
        const auto inst_id = get_inst_id();
        const auto dest = emit_reg(dest_type, inst_id);

        for (const auto& op : ops) {
            if (op.is_reg()) {
                current_func->add_use(op.as_reg(), inst_id, *current_block);
            }
        }

        current_func->insts.emplace_back(inst_id, op, dest, ops);
        current_func->get_block(*current_block).insts.push_back(inst_id);
        return dest;
    }

    VReg emit_inst(IROp op, std::vector<Operand> ops, type::TypeRef dest_type) {
        const auto inst_id = get_inst_id();
        const auto dest = emit_reg(dest_type, inst_id);

        for (const auto& op : ops) {
            if (op.is_reg()) {
                current_func->add_use(op.as_reg(), inst_id, *current_block);
            }
        }

        current_func->insts.emplace_back(inst_id, op, dest, std::move(ops));
        current_func->get_block(*current_block).insts.push_back(inst_id);
        return dest;
    }

    void emit_inst(IROp op, std::initializer_list<Operand> ops) {
        const auto inst_id = get_inst_id();

        for (const auto& op : ops) {
            if (op.is_reg()) {
                current_func->add_use(op.as_reg(), inst_id, *current_block);
            }
        }

        current_func->insts.emplace_back(inst_id, op, ops);
        current_func->get_block(*current_block).insts.push_back(inst_id);
    }

    void emit_term(IROp op, std::initializer_list<Operand> ops) {
        const auto inst_id = get_inst_id();

        for (const auto& op : ops) {
            if (op.is_reg()) {
                current_func->add_use(op.as_reg(), inst_id, *current_block);
            }
        }

        current_func->insts.emplace_back(inst_id, op, ops);
        current_func->get_block(*current_block).insts.push_back(inst_id);
    }

    Operand ensure_reg(Operand op) {
        if (op.is_reg())
            return op;
        auto& imm = op.as_imm();
        return Operand::reg(emit_inst(IROp::LoadConst, {op}, imm.type));
    }

    void add_deferred_return(std::optional<Operand> val) {
        if (val) {
            val = std::make_optional<Operand>(ensure_reg(*val).as_reg());
        }

        deferred_returns.emplace_back(*current_block, val);
    }

    void add_deferred_break(std::optional<Operand> val) {
        deferred_loop_jumps.emplace_back(DeferredJumpType::Break,
                                         *current_block, val);
        current_func->get_block(*current_block).term = TerminatorKind::Jump;
    }

    void add_deferred_continue() {
        deferred_loop_jumps.emplace_back(DeferredJumpType::Continue,
                                         *current_block, std::nullopt);
        current_func->get_block(*current_block).term = TerminatorKind::Jump;
    }

    void emit_jump(BlockID to) {
        if (current_func->get_block(*current_block).term)
            return;
        emit_term(IROp::Jump, {Operand::label(to)});
        current_func->get_block(*current_block).term = TerminatorKind::Jump;
        current_func->get_block(to).add_predecessor(*current_block);
        current_func->get_block(*current_block).add_successor(to);
    }

    void emit_branch(Operand cond, BlockID _true, BlockID _false) {
        if (current_func->get_block(*current_block).term)
            return;
        emit_term(IROp::Branch,
                  {cond, Operand::label(_true), Operand::label(_false)});
        current_func->get_block(*current_block).term = TerminatorKind::Branch;
        current_func->get_block(_true).add_predecessor(*current_block);
        current_func->get_block(_false).add_predecessor(*current_block);
        current_func->get_block(*current_block).add_successor(_true);
        current_func->get_block(*current_block).add_successor(_false);
    }

    void link_blocks(BlockID from, BlockID to) {
        current_func->get_block(to).add_predecessor(from);
        current_func->get_block(from).add_successor(to);
    }

    /*
      SSA Construction
      This implementation closely follows Algorithms 1-4 for SSA construction
      Based on:
      Braun et al. (2013). "Simple and Efficient Construction of Static Single
      Assignment Form", In: Compiler Construction. Lecture Notes in Computer
      Science, vol 7791, 102-122. DOI: 10.1007/978-3-642-37051-9_6

    */

    VReg emit_phi(type::TypeRef type) {
        const auto inst_id = get_inst_id();
        const auto dest = emit_reg(type, inst_id);

        current_func->insts.emplace_back(inst_id, IROp::Phi, dest,
                                         std::initializer_list<Operand>{});
        current_func->get_block(*current_block).phis.push_back(inst_id);
        return dest;
    }

    void write_var(StringID var, VReg val) {
        current_block_state->var_map[var] = val;
    }

    void write_var(StringID var, BlockID block, VReg val) {
        block_state[block].var_map[var] = val;
    }

    VReg read_var(StringID var) {
        if (auto it = current_block_state->var_map.find(var);
            it != current_block_state->var_map.end())
            return it->second;

        return read_var_recursive(var, *current_block);
    }

    VReg read_var(StringID var, BlockID block) {
        if (auto it = block_state[block].var_map.find(var);
            it != block_state[block].var_map.end())
            return it->second;

        return read_var_recursive(var, block);
    }

    VReg read_var_recursive(StringID var, BlockID block_id) {
        VReg val;
        auto& state = block_state[block_id];
        auto& block = current_func->get_block(block_id);

        auto saved_block = current_block;
        auto* saved_state = current_block_state;
        switch_block(block_id);

        if (!state.sealed) {
            auto id = get_inst_id();
            auto phi = emit_phi(*ctxt->syms->get_var(var));
            state.incomplete_phis.insert_or_assign(var, id);
            val = phi;
        } else if (block.predecessors.size() == 1) {
            val = read_var(var, block.predecessors.front());
        } else {
            auto inst_id = get_inst_id();
            auto phi = emit_phi(*ctxt->syms->get_var(var));
            write_var(var, block_id, phi);
            val = add_phi_operands(var, block, inst_id);
        }
        write_var(var, block_id, val);

        current_block = saved_block;
        current_block_state = saved_state;
        return val;
    }

    void add_phi_operand(InstId phi, BlockID phi_inst_block, VReg var,
                         BlockID block_id) {
        auto& inst = get_inst(phi);
        inst.operands.push_back(Operand::reg(var));
        inst.operands.push_back(Operand::label(block_id));
        current_func->add_use(var, phi, block_id);
    }

    VReg add_phi_operands(StringID var, BasicBlock& block, InstId phi) {
        for (const auto pred : block.predecessors) {
            auto v = read_var(var, pred);
            add_phi_operand(phi, block.id, v, pred);
        }

        return try_remove_trivial_phi(phi, block);
    }

    VReg try_remove_trivial_phi(InstId inst_id, BasicBlock& block) {
        VReg same;
        bool has_same = false;
        auto& phi = current_func->insts[inst_id.id];

        for (const auto& op_ : phi.operands) {
            if (!op_.is_reg())
                continue;
            auto op = op_.as_reg();

            if (op.id == phi.dest.value().id)
                continue;

            if (has_same && op.id == same.id)
                continue;

            if (has_same)
                return *phi.dest;

            same = op;
            has_same = true;
        }

        phi.op = IROp::Dead;

        for (auto& [var, reg] : block_state[block.id].var_map) {
            if (reg.id == phi.dest.value().id) {
                block_state[block.id].var_map[var] = same;
            }
        }

        for (const auto& [user_inst, user_block] :
             current_func->get_uses(*phi.dest)) {
            for (auto& reg : get_inst(user_inst).operands) {
                if (reg.is_reg() && reg.as_reg().id == phi.dest.value().id) {
                    reg = Operand::reg(same);
                }
            }

            if (get_inst(user_inst).op == IROp::Phi) {
                auto phi_block =
                    current_func->get_def(*get_inst(user_inst).dest).block;
                try_remove_trivial_phi(user_inst,
                                       current_func->get_block(phi_block));
            }
        }

        return same;
    }

    void seal_block(BasicBlock& block) {
        for (auto& [var, phi] : block_state[block.id].incomplete_phis) {
            add_phi_operands(var, block, phi);
        }
        block_state[block.id].incomplete_phis.clear();
        block_state[block.id].sealed = true;
    }

public:
    explicit IRBuilder(ZContext& ctxt) : ctxt(&ctxt) {}

    IRFile lower_ast(ast::SourceFileDecl* ast) {
        ast->accept(*this);
        return IRFile(std::move(funcs));
    }
};
} // namespace z::ir
