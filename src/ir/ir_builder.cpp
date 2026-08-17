#include "ir_builder.h"
#include "core/panic.h"
#include "core/types.h"
#include "diag/diagnostics.h"
#include "ir/condition_codes.h"
#include "ir/constants.h"
#include "ir/ir.h"
#include "parser/ast.h"
#include "type/type.h"
#include "type/type_arena.h"
#include "type/type_ref.h"
#include <cassert>
#include <functional>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

namespace z::ir {

using ast::BinOp;
using ast::UnOp;

void IRBuilder::visit(ast::Identifier& ident) {
    if (!ctxt->syms->is_var_local(ident.get_id()) &&
        constants.contains(ident.get_id())) {
        last_result = constants.at(ident.get_id());
        return;
    }

    last_result = Operand::reg(read_var(ident.get_id()));
}

void IRBuilder::visit(ast::IntExpr& expr) {
    const auto* ty = ctxt->ty->get_as<type::IntegerType>(expr.get_type());
    last_result = Operand::imm(
        ConstInt(expr.val, ty->get_width(), ty->is_signed()), expr.get_type());
}

void IRBuilder::visit(ast::FloatExpr& expr) {
    const auto* ty = ctxt->ty->get_as<type::FloatType>(expr.get_type());
    last_result =
        Operand::imm(ConstFloat(expr.val, ty->get_width()), expr.get_type());
}

void IRBuilder::visit(ast::BoolExpr& expr) {
    last_result = Operand::imm(expr.val, type::builtin::BOOL);
}

void IRBuilder::visit(ast::PrefixExpr& expr) {
    const auto var = emit_op(expr.expr.get());
    const auto expr_type = [&] {
        if (var.is_reg())
            return var.as_reg().type;
        if (var.is_imm())
            return var.as_imm().type;
        panic("Invalid unary operand");
    }();

    const auto expr_op = expr.op;

    switch (expr_op) {
    case UnOp::Inc:
    case UnOp::Dec: {
        const auto* int_type = ctxt->ty->get_as<type::IntegerType>(expr_type);

        auto dest =
            emit_inst(get_int_ir_op(expr_op),
                      {var, Operand::imm(ConstInt(1, int_type->get_width(),
                                                  int_type->is_signed()),
                                         expr_type)},
                      expr_type);

        write_var(ast::cast<ast::Identifier>(expr.expr.get())->get_id(), dest);
        last_result = Operand::reg(dest);
        return;
    }
    case UnOp::LogicNot:
    case UnOp::BitNot: {
        auto dest = emit_inst(get_ir_op(expr_op), {var}, expr_type);
        last_result = Operand::reg(dest);
        return;
    }
    case UnOp::Neg: {
        const auto* type = ctxt->ty->get(expr_type);
        if (type->is_integral()) {
            auto dest = emit_inst(IROp::INeg, {var}, expr_type);
            last_result = Operand::reg(dest);
        } else if (type->is_float()) {
            auto dest = emit_inst(IROp::FNeg, {var}, expr_type);
            last_result = Operand::reg(dest);
        } else {
            panic("Invalid type for negation operator");
        }

        return;
    }
    }
    std::unreachable();
}

void IRBuilder::visit(ast::PostfixExpr& expr) {
    const auto var = emit_op(expr.expr.get());
    const auto expr_type = [&] {
        if (var.is_reg())
            return var.as_reg().type;
        if (var.is_imm())
            return var.as_imm().type;
        panic("Invalid unary operand");
    }();

    const auto* int_type = ctxt->ty->get_as<type::IntegerType>(expr_type);

    auto dest = emit_inst(get_int_ir_op(expr.op),
                          {var, Operand::imm(ConstInt(1, int_type->get_width(),
                                                      int_type->is_signed()),
                                             expr_type)},
                          expr_type);
    write_var(ast::cast<ast::Identifier>(expr.expr.get())->get_id(), dest);
    last_result = var;
}

void IRBuilder::visit(ast::BinaryExpr& expr) {
    const auto expr_op = expr.op;
    std::optional<IntCC> int_cc{};
    std::optional<FloatCC> float_cc{};

    if (expr_op == BinOp::LogicAnd || expr_op == BinOp::LogicOr) {
        auto lhs = emit_op(expr.lhs.get());
        auto lhs_reg = ensure_reg(lhs);
        auto lhs_block = *current_block;

        auto rhs_block = new_block();
        auto end_block = new_block();

        if (expr_op == BinOp::LogicAnd)
            emit_branch(lhs_reg, rhs_block, end_block);
        else
            emit_branch(lhs_reg, end_block, rhs_block);

        switch_block(rhs_block);
        seal_block(current_func->get_block(rhs_block));
        auto rhs = emit_op(expr.rhs.get());
        auto rhs_reg = ensure_reg(rhs);
        auto rhs_exit = *current_block;
        emit_jump(end_block);

        switch_block(end_block);
        seal_block(current_func->get_block(end_block));

        auto phi_inst = get_inst_id();
        auto phi = emit_phi(type::builtin::BOOL);
        add_phi_operand(phi_inst, end_block, lhs_reg.as_reg(), lhs_block);
        add_phi_operand(phi_inst, end_block, rhs_reg.as_reg(), rhs_exit);

        last_result = Operand::reg(phi);
        return;
    }

    if (expr_op == BinOp::Eq) {
        auto* lhs = expr.lhs.get();
        auto rhs = emit_op(expr.rhs.get());

        rhs = ensure_reg(rhs);
        if (ast::isa<ast::Identifier>(lhs))
            write_var(ast::cast<ast::Identifier>(lhs)->get_id(), rhs.as_reg());
        else
            emit_aggregate_insert(lhs, rhs);
        return;
    }

    auto op = [&] {
        const auto* type = ctxt->ty->get(expr.lhs->get_type());
        if (type->is_integral()) {
            const auto* int_type = type::cast<const type::IntegerType>(type);
            auto op = get_int_ir_op(expr_op, int_type->is_signed());
            if (op == IROp::ICmp)
                int_cc = get_int_cc(expr_op, int_type->is_signed());

            return op;
        }

        if (type->is_float()) {
            auto op = get_float_ir_op(expr_op);
            if (op == IROp::FCmp)
                float_cc = get_float_cc(expr_op);
            return op;
        }

        return get_ir_op(expr_op);
    }();

    const auto lhs = emit_op(expr.lhs.get());
    const auto rhs = emit_op(expr.rhs.get());

    if (lhs.is_imm() && rhs.is_imm()) {
        auto lhs_imm = lhs.as_imm();
        auto rhs_imm = rhs.as_imm();

        if (lhs_imm.is_int() && rhs_imm.is_int()) {
            if (op == IROp::ICmp) {
                auto res = lhs_imm.as_int().cmp(rhs_imm.as_int(), *int_cc);
                last_result = Operand::imm(res, type::builtin::BOOL);
                return;
            }

            bool overflow = false;
            auto res =
                fold_int_op(op, lhs_imm.as_int(), rhs_imm.as_int(), overflow);

            if (overflow) {
                const auto name =
                    ctxt->ty->get(expr.get_type())->basic_name(ctxt);
                ctxt->diag.error(expr.get_span(),
                                 DiagnosticKind::OperationOverflows, name);
                return;
            }

            if (res) {
                last_result = Operand::imm(*res, expr.get_type());
                return;
            }
        } else if (lhs_imm.is_float() && rhs_imm.is_float()) {
            if (op == IROp::FCmp) {
                auto res =
                    lhs_imm.as_float().cmp(rhs_imm.as_float(), *float_cc);
                last_result = Operand::imm(res, type::builtin::BOOL);
                return;
            }

            auto res =
                fold_float_op(op, lhs_imm.as_float(), rhs_imm.as_float());
            if (res) {
                last_result = Operand::imm(*res, expr.get_type());
                return;
            }
        } else if (lhs_imm.is_bool() && rhs_imm.is_bool()) {
            auto res =
                fold_bool_op(expr_op, lhs_imm.as_bool(), rhs_imm.as_bool());
            if (res) {
                last_result = Operand::imm(*res, type::builtin::BOOL);
                return;
            }
        }
    }

    switch (expr_op) {
    case BinOp::AddEq:
    case BinOp::SubEq:
    case BinOp::MulEq:
    case BinOp::DivEq:
    case BinOp::ModEq:
    case BinOp::BitXorEq:
    case BinOp::BitAndEq:
    case BinOp::BitOrEq:
    case BinOp::ShlEq:
    case BinOp::ShrEq: {
        auto dest = emit_inst(op, {lhs, rhs}, expr.lhs->get_type());
        write_var(ast::cast<ast::Identifier>(expr.lhs.get())->get_id(), dest);
        last_result = std::nullopt;
        break;
    }

    case BinOp::BitOr:
    case BinOp::BitXor:
    case BinOp::BitAnd:
    case BinOp::Shl:
    case BinOp::Shr:
    case BinOp::Mod:
    case BinOp::Add:
    case BinOp::Sub:
    case BinOp::Mul:
    case BinOp::Div: {
        auto dest = emit_inst(op, {lhs, rhs}, expr.get_type());
        last_result = Operand::reg(dest);
        break;
    }
    case BinOp::EqEq:
    case BinOp::Ne:
    case BinOp::Gt:
    case BinOp::Lt:
    case BinOp::Ge:
    case BinOp::Le: {
        if (int_cc) {
            last_result = Operand::reg(emit_inst(
                op, {Operand::intcc(*int_cc), lhs, rhs}, expr.get_type()));
        } else if (float_cc) {
            last_result = Operand::reg(emit_inst(
                op, {Operand::floatcc(*float_cc), lhs, rhs}, expr.get_type()));
        } else {
            last_result =
                Operand::reg(emit_inst(op, {lhs, rhs}, expr.get_type()));
        }
        break;
    }

    case BinOp::Range:
    case BinOp::RangeEq:
    case BinOp::ColonColon:
        panic("BinOp not handled in switch statement");

    case BinOp::LogicAnd:
    case BinOp::LogicOr:
    case BinOp::Eq:
        std::unreachable();
    }
}

void IRBuilder::visit(ast::CallExpr& expr) {
    auto& func =
        get_func(ast::cast<ast::Identifier>(expr.ident.get())->get_id());

    std::vector<Operand> args;
    args.reserve(expr.args.size() + 1);
    args.push_back(Operand::func(func.id));
    for (auto& arg : expr.args) {
        args.push_back(emit_op(arg.get()));
    }

    auto result = emit_inst(IROp::Call, std::move(args), func.return_type);
    last_result = Operand::reg(result);
}

void IRBuilder::visit(ast::ArrayExpr& expr) {
    auto container = emit_op(expr.array.get());
    auto val = emit_op(expr.val.get());

    const auto* arr_type =
        ctxt->ty->get_as<type::ArrayType>(container.as_reg().type);
    expect(arr_type != nullptr, "ArrayExpr should have type ArrayType");

    auto result =
        emit_inst(IROp::ExtractElement, {container, val}, arr_type->get_type());
    last_result = Operand::reg(result);
}

void IRBuilder::visit(ast::FieldExpr& expr) {
    auto container = emit_op(expr.container.get());
    const auto* struct_type =
        ctxt->ty->get_as<type::StructType>(container.as_reg().type);
    expect(struct_type != nullptr,
           "FieldExpr.container should have type StructType");

    auto field_type = struct_type->get_field_type(expr.field->get_id());
    auto field_idx = struct_type->get_field_index(expr.field->get_id());
    expect(field_idx && field_type, "Couldn't find field in StructType");

    auto result =
        emit_inst(IROp::ExtractField, {container, Operand::field(*field_idx)},
                  *field_type);
    last_result = Operand::reg(result);
}

void IRBuilder::visit(ast::ArrayInitExpr& expr) {
    const auto* arr_type = ctxt->ty->get_as<type::ArrayType>(expr.get_type());
    expect(arr_type != nullptr, "ArrayInitExpr should have type ArrayType");

    std::vector<Operand> vals;

    for (auto& val : expr.vals) {
        auto v = emit_op(val.get());
        vals.push_back(v);
    }

    auto result = emit_inst(IROp::ArrayInit, std::move(vals), expr.get_type());
    last_result = Operand::reg(result);
}

void IRBuilder::visit(ast::StructExprField& expr) { expr.val->accept(*this); }

void IRBuilder::visit(ast::StructInitExpr& expr) {
    const auto* struct_type =
        ctxt->ty->get_as<type::StructType>(expr.get_type());
    expect(struct_type != nullptr,
           "StructInitExpr should have type StructType");

    std::vector<Operand> fields;
    fields.reserve(expr.fields.size() + 1);
    const auto* ident = ast::cast<ast::Identifier>(expr.ident.get());
    fields.push_back(Operand::imm(ident->get_id(), ident->get_type()));

    for (auto& field : expr.fields) {
        field->accept(*this);
        auto id = *struct_type->get_field_index(field->ident->get_id());
        fields.insert(fields.begin() + id + 1, *last_result);
    }

    auto result = emit_inst(IROp::StructInit, fields, expr.get_type());
    last_result = Operand::reg(result);
}

void IRBuilder::visit(ast::TupleExpr& expr) {
    const auto first = emit_op(expr.first.get());
    const auto second = emit_op(expr.second.get());
    auto dest = emit_inst(IROp::TupleInit, {first, second}, expr.get_type());
    last_result = Operand::reg(dest);
}

void IRBuilder::visit(ast::Block& block) {
    ctxt->syms->enter_scope(block.get_scope_id());

    for (const auto& stmt : block.stmts) {
        if (current_func->get_block(*current_block).term) {
            ctxt->diag.warn(stmt->get_span(), DiagnosticKind::UnreachableStmt);
            break;
        }
        stmt->accept(*this);
    }
    ctxt->syms->exit_scope();
}

void IRBuilder::visit(ast::Param& param) {
    auto reg = emit_inst(IROp::Arg, {}, param.type);
    current_func->params.push_back(reg);
    write_var(param.name->get_id(), reg);
}

void IRBuilder::visit(ast::SourceFileDecl& file) {
    for (const auto& decl : file.const_decls)
        decl->accept(*this);

    for (const auto& decl : file.decls) {
        decl->accept(*this);
    }
}

void IRBuilder::visit(ast::FuncDecl& func) {
    auto func_id = get_func_id();
    current_func = &funcs.emplace_back(func_id, func.name->get_id(), func.ret,
                                       std::vector<VReg>());
    block_state.clear();

    auto entry = new_block();
    seal_block(current_func->get_block(entry));
    switch_block(entry);

    for (const auto& param : func.params) {
        param->accept(*this);
    }

    last_result = std::nullopt;

    func.body->accept(*this);

    auto& exit_block = current_func->get_block(*current_block);
    if (!exit_block.term) {
        if (func.body->get_type() == type::builtin::VOID)
            add_deferred_return(std::nullopt);
        else
            add_deferred_return(last_result);
    }

    if (deferred_returns.size() == 1) {
        auto& [from, val] = deferred_returns.front();
        assert(from == *current_block);

        if (val)
            emit_term(IROp::Ret, {*val});
        else
            emit_term(IROp::Ret, {});

        deferred_returns.clear();
        current_func->get_block(*current_block).term = TerminatorKind::Jump;
        return;
    }

    auto ret_block = new_block();

    switch_block(ret_block);

    std::optional<VReg> ret_reg;
    std::optional<InstId> ret_phi_inst;

    if (func.ret != type::builtin::VOID) {
        ret_phi_inst = get_inst_id();
        ret_reg = emit_phi(func.ret);
    }

    for (auto& [from, val] : deferred_returns) {
        switch_block(from);
        current_func->get_block(from).term = std::nullopt;
        if (val && ret_phi_inst) {
            add_phi_operand(*ret_phi_inst, ret_block, ensure_reg(*val).as_reg(),
                            from);
        }
        emit_jump(ret_block);
    }

    seal_block(current_func->get_block(ret_block));

    if (ret_phi_inst) {
        auto simplified = try_remove_trivial_phi(
            *ret_phi_inst, current_func->get_block(ret_block));
        ret_reg = simplified;
    }

    deferred_returns.clear();

    switch_block(ret_block);
    if (ret_reg)
        emit_term(IROp::Ret, {Operand::reg(*ret_reg)});
    else
        emit_term(IROp::Ret, {});

    current_func->get_block(ret_block).term = TerminatorKind::Return;
}

void IRBuilder::visit(ast::BreakStmt& stmt) {
    if (stmt.expr)
        add_deferred_break(emit_op(stmt.expr.get()));
    else
        add_deferred_break(std::nullopt);
}

void IRBuilder::visit(ast::ContinueStmt& /*stmt*/) { add_deferred_continue(); }

void IRBuilder::visit(ast::ForExpr& expr) {}

void IRBuilder::visit(ast::LetStmt& stmt) {
    if (stmt.val) {
        auto val = emit_op(stmt.val.get());
        auto reg = ensure_reg(val).as_reg();
        write_var(stmt.ident->get_id(), reg);
    }
}

void IRBuilder::visit(ast::ReturnStmt& stmt) {
    std::optional<Operand> val;

    if (stmt.expr) {
        val = emit_op(stmt.expr.get());
    }

    add_deferred_return(val);
    current_func->get_block(*current_block).term = TerminatorKind::Jump;
}

void IRBuilder::visit(ast::IfExpr& expr) {
    auto cond = emit_op(expr.expr.get());
    auto then_block = new_block();
    auto branch_source = *current_block;

    switch_block(then_block);
    seal_block(current_func->get_block(then_block));
    link_blocks(branch_source, then_block);

    if (expr.else_expr) {
        // Then
        expr.block->accept(*this);
        auto then_result = expr.block->get_type() != type::builtin::VOID
                               ? std::make_optional(ensure_reg(*last_result))
                               : std::nullopt;
        auto then_exit = *current_block;
        bool const then_returned =
            current_func->get_block(then_exit).term.has_value();

        // Else
        auto else_block = new_block();
        switch_block(else_block);
        seal_block(current_func->get_block(else_block));
        link_blocks(branch_source, else_block);
        expr.else_expr->accept(*this);
        auto else_result = expr.else_expr->get_type() != type::builtin::VOID
                               ? std::make_optional(ensure_reg(*last_result))
                               : std::nullopt;
        auto else_exit = *current_block;
        bool const else_returned =
            current_func->get_block(else_exit).term.has_value();

        switch_block(branch_source);
        emit_branch(cond, then_block, else_block);

        if (then_returned && else_returned) {
            last_result = std::nullopt;
            return;
        }

        auto end_block = new_block();

        if (!then_returned) {
            switch_block(then_exit);
            emit_jump(end_block);
        }

        if (!else_returned) {
            switch_block(else_exit);
            emit_jump(end_block);
        }

        switch_block(end_block);
        seal_block(current_func->get_block(end_block));

        if (then_result && else_result) {
            auto phi = emit_phi(expr.get_type());
            auto phi_inst = current_func->get_def(phi).inst;
            add_phi_operand(phi_inst, end_block, then_result->as_reg(),
                            then_exit);
            add_phi_operand(phi_inst, end_block, else_result->as_reg(),
                            else_exit);
            last_result = Operand::reg(phi);
        } else {
            last_result = std::nullopt;
        }

    } else {
        expr.block->accept(*this);

        auto end_block = new_block();

        emit_jump(end_block);

        switch_block(branch_source);
        emit_branch(cond, then_block, end_block);

        switch_block(end_block);
        seal_block(current_func->get_block(end_block));
    }
}

void IRBuilder::visit(ast::ElseExpr& expr) {
    if (expr.if_expr)
        expr.if_expr->accept(*this);
    else
        expr.block->accept(*this);
}

void IRBuilder::visit(ast::LoopExpr& expr) {
    if (expr.expr) {
        auto max_loop_cnt = ensure_reg(emit_op(expr.expr.get()));
        auto loop_cnt = Operand::reg(
            emit_inst(IROp::LoadConst,
                      {Operand::imm(ConstInt(0, sizeof(usize), false),
                                    type::builtin::USIZE)},
                      type::builtin::USIZE));
        auto entry_block = *current_block;

        auto cond_block = new_block();
        seal_block(current_func->get_block(*current_block));
        emit_jump(cond_block);
        switch_block(cond_block);

        auto cnt_reg_id = get_inst_id();
        auto cnt_reg = emit_phi(type::builtin::USIZE);

        add_phi_operand(cnt_reg_id, cond_block, loop_cnt.as_reg(), entry_block);

        auto cond = emit_inst(IROp::ICmp,
                              {Operand::intcc(IntCC::UnsignedLessThan),
                               Operand::reg(cnt_reg), max_loop_cnt},
                              type::builtin::BOOL);

        auto body_block = new_block();
        link_blocks(*current_block, body_block);
        switch_block(body_block);
        expr.block->accept(*this);

        auto inc_block = new_block();
        emit_jump(inc_block);

        seal_block(current_func->get_block(inc_block));
        switch_block(inc_block);

        const auto new_loop_cnt = emit_inst(
            IROp::IAdd,
            std::vector<Operand>{Operand::reg(cnt_reg),
                                 Operand::imm(ConstInt(0, sizeof(usize), false),
                                              type::builtin::USIZE)},
            type::builtin::USIZE);

        add_phi_operand(cnt_reg_id, cond_block, new_loop_cnt, *current_block);

        emit_jump(cond_block);
        switch_block(cond_block);
        auto end_block = new_block();
        emit_branch(Operand::reg(cond), body_block, end_block);
        seal_block(current_func->get_block(cond_block));
        seal_block(current_func->get_block(body_block));

        for (const auto& jump : deferred_loop_jumps) {
            switch_block(jump.from_block);
            if (jump.type == DeferredJumpType::Break) {
                if (jump.value)
                    last_result = ensure_reg(*jump.value);

                emit_jump(end_block);
            } else {
                emit_jump(cond_block);
            }
        }
        deferred_loop_jumps.clear();

        switch_block(end_block);
        seal_block(current_func->get_block(end_block));
    } else {
        auto loop_body = new_block();

        seal_block(current_func->get_block(*current_block));
        emit_jump(loop_body);
        switch_block(loop_body);

        expr.block->accept(*this);
        emit_jump(loop_body);
        seal_block(current_func->get_block(loop_body));

        auto end_block = new_block();

        for (const auto& jump : deferred_loop_jumps) {
            current_func->get_block(jump.from_block).term = std::nullopt;
            switch_block(jump.from_block);
            if (jump.type == DeferredJumpType::Break) {
                if (jump.value)
                    last_result = ensure_reg(*jump.value);

                emit_jump(end_block);
            } else {
                emit_jump(loop_body);
            }
        }
        deferred_loop_jumps.clear();

        switch_block(end_block);
        seal_block(current_func->get_block(end_block));
    }
}

void IRBuilder::visit(ast::WhileExpr& expr) {
    auto cond_block = new_block();

    seal_block(current_func->get_block(*current_block));
    emit_jump(cond_block);
    switch_block(cond_block);

    auto cond = emit_op(expr.expr.get());

    auto body_block = new_block();
    link_blocks(*current_block, body_block);
    switch_block(body_block);

    expr.block->accept(*this);
    emit_jump(cond_block);
    switch_block(cond_block);

    auto end_block = new_block();
    emit_branch(cond, body_block, end_block);
    seal_block(current_func->get_block(cond_block));
    seal_block(current_func->get_block(body_block));

    for (const auto& jump : deferred_loop_jumps) {
        switch_block(jump.from_block);
        current_func->get_block(jump.from_block).term = std::nullopt;
        if (jump.type == DeferredJumpType::Break) {
            if (jump.value)
                last_result = ensure_reg(*jump.value);

            emit_jump(end_block);
        } else {
            emit_jump(cond_block);
        }
    }
    deferred_loop_jumps.clear();

    switch_block(end_block);
    seal_block(current_func->get_block(end_block));
}

void IRBuilder::visit(ast::StringExpr& expr) {
    last_result = Operand::imm(expr.string, expr.get_type());
}

void IRBuilder::visit(ast::CharExpr& expr) {
    last_result = Operand::imm(expr.c, expr.get_type());
}

void IRBuilder::visit(ast::ConstDecl& decl) {
    const auto val = emit_op(decl.val.get());
    constants.emplace(decl.ident->get_id(), val);
}

void IRBuilder::visit(ast::StaticDecl& decl) {}

void IRBuilder::visit(ast::TraitDecl& decl) {}

void IRBuilder::visit(ast::TypeAliasDecl& decl) {}

void IRBuilder::visit(ast::TraitFuncDecl& decl) {}

Operand IRBuilder::emit_aggregate_insert(ast::Expr* lhs, Operand new_val) {
    std::vector<std::pair<Operand, Operand>> aggregate_types;
    ast::Identifier* ident = nullptr;

    std::function<void(ast::Expr*, bool)> extract_recursive;
    extract_recursive = [&](ast::Expr* e, bool first) {
        if (auto* f = ast::dyn_cast<ast::FieldExpr>(e)) {
            extract_recursive(f->container.get(), false);

            auto container = *last_result;

            const auto* struct_type =
                ctxt->ty->get_as<type::StructType>(container.as_reg().type);
            expect(struct_type != nullptr,
                   "FieldExpr.container should have type StructType");

            auto field_type = struct_type->get_field_type(f->field->get_id());
            auto field_idx = struct_type->get_field_index(f->field->get_id());
            expect(field_idx && field_type,
                   "Couldn't find field in StructType");

            aggregate_types.emplace_back(container, Operand::field(*field_idx));

            if (first)
                return;

            auto result =
                emit_inst(IROp::ExtractField,
                          {container, Operand::field(*field_idx)}, *field_type);
            last_result = Operand::reg(result);
        } else if (auto* a = ast::dyn_cast<ast::ArrayExpr>(e)) {
            extract_recursive(a->array.get(), false);

            auto array = *last_result;
            auto val = emit_op(a->val.get());

            aggregate_types.emplace_back(array, val);

            if (first)
                return;

            auto result =
                emit_inst(IROp::ExtractElement, {array, val}, e->get_type());
            last_result = Operand::reg(result);
        } else if (auto* i = ast::dyn_cast<ast::Identifier>(e)) {
            emit_op(i);
            ident = i;
        }
    };

    extract_recursive(lhs, true);

    Operand val = new_val;
    for (auto& [op, idx] : std::ranges::reverse_view(aggregate_types)) {
        expect(op.is_reg(), "Should be reg");
        const auto* t = ctxt->ty->get(op.as_reg().type);
        if (type::isa<type::StructType>(t)) {
            auto result =
                emit_inst(IROp::InsertField, {op, idx, val}, op.as_reg().type);
            val = Operand::reg(result);
        } else if (type::isa<type::ArrayType>(t)) {
            auto result = emit_inst(IROp::InsertElement, {op, idx, val},
                                    op.as_reg().type);
            val = Operand::reg(result);
        }
    }

    *last_result = val;
    write_var(ident->get_id(), val.as_reg());
    return val;
}

} // namespace z::ir
