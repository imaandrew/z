#pragma once

#include "core/panic.h"
#include "core/string_pool.h"
#include "core/zctxt.h"
#include "ir.h"
#include "parser/ast.h"
#include "type/type_ref.h"
#include <cstdint>
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

    struct PendingReturn {
        BlockID from_block;
        std::optional<Operand> value;

        PendingReturn(BlockID from_block, std::optional<Operand> value)
            : from_block(from_block), value(value) {}
    };

    ZContext* ctxt;

    std::vector<IRFunction> funcs;
    std::optional<Operand> last_result;

    IRFunction* current_func{};
    std::unordered_map<BlockID, BlockBuildState> block_state;
    std::optional<BlockID> current_block;
    BlockBuildState* current_block_state{};

    std::vector<PendingReturn> pending_returns;

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
    void visit(ast::StructField& field) override;
    void visit(ast::StructDecl& decl) override;
    void visit(ast::EnumField& field) override;
    void visit(ast::EnumDecl& decl) override;
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
        auto reg_id = current_func->vreg_info.size();
        current_func->vreg_info.emplace_back(inst);
        return VReg{.id = static_cast<std::uint32_t>(reg_id), .type = type};
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

    VReg emit_inst(IROp op, std::initializer_list<Operand> ops,
                   type::TypeRef dest_type) {
        const auto inst_id = InstId(get_inst_id());
        const auto dest = emit_reg(dest_type, inst_id);

        for (const auto& op : ops) {
            if (op.is_reg()) {
                current_func->get_reg_info(op.as_reg())
                    .uses.emplace_back(inst_id, *current_block);
            }
        }

        current_func->insts.emplace_back(inst_id, op, dest, ops);
        current_func->get_block(*current_block).insts.push_back(inst_id);
        return dest;
    }

    VReg emit_inst(IROp op, std::vector<Operand> ops, type::TypeRef dest_type) {
        const auto inst_id = InstId(get_inst_id());
        const auto dest = emit_reg(dest_type, inst_id);

        for (const auto& op : ops) {
            if (op.is_reg()) {
                current_func->get_reg_info(op.as_reg())
                    .uses.emplace_back(inst_id, *current_block);
            }
        }

        current_func->insts.emplace_back(inst_id, op, dest, std::move(ops));
        current_func->get_block(*current_block).insts.push_back(inst_id);
        return dest;
    }

    void emit_inst(IROp op, std::initializer_list<Operand> ops) {
        const auto inst_id = InstId(get_inst_id());

        for (const auto& op : ops) {
            if (op.is_reg()) {
                current_func->get_reg_info(op.as_reg())
                    .uses.emplace_back(inst_id, *current_block);
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

    void add_pending_return(std::optional<Operand> val) {
        if (val) {
            val = std::make_optional<Operand>(ensure_reg(*val).as_reg());
        }

        pending_returns.emplace_back(*current_block, val);
    }

    /*
      SSA Construction
      This implementation closely follows Algorithms 1-4 for SSA construction
      Based on:
      Braun et al. (2013). "Simple and Efficient Construction of Static Single
      Assignment Form", In: Compiler Construction. Lecture Notes in Computer
      Science, vol 7791, 102-122. DOI: 10.1007/978-3-642-37051-9_6

    */

    VReg emit_phi(type::TypeRef type) { return emit_inst(IROp::Phi, {}, type); }

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
        return val;
    }

    void add_phi_operand(InstId phi, VReg var, BlockID block_id) {
        auto& inst = get_inst(phi);
        inst.operands.push_back(Operand::reg(var));
        inst.operands.push_back(Operand::label(block_id));
    }

    VReg add_phi_operands(StringID var, BasicBlock& block, InstId phi) {
        for (const auto pred : block.predecessors) {
            auto v = read_var(var, pred);
            add_phi_operand(phi, v, pred);

            auto& users = current_func->get_reg_info(v).uses;
            users.emplace_back(phi, block.id);
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

        for (auto& [user_inst, user_block] :
             current_func->get_reg_info(*phi.dest).uses) {
            for (auto& reg : get_inst(user_inst).operands) {
                if (reg.is_reg() && reg.as_reg().id == phi.dest.value().id) {
                    reg = Operand::reg(same);
                }
            }

            if (get_inst(user_inst).op == IROp::Phi) {
                try_remove_trivial_phi(user_inst,
                                       current_func->get_block(user_block));
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
