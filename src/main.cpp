#include "core/colour.h"
#include "core/panic.h"
#include "core/zctxt.h"
#include "ir/analysis/analyses.h"
#include "ir/analysis/maxlive.h"
#include "ir/ir.h"
#include "ir/ir_builder.h"
#include "ir/ir_printer.h"
#include "ir/pass_mgr.h"
#include "ir/passes/ConstFoldPass.h"
#include "ir/passes/ConstPropPass.h"
#include "ir/passes/CriticalEdgeSplitPass.h"
#include "ir/passes/DCEPass.h"
#include "ir/passes/backend/CopyCoalescingPass.h"
#include "ir/passes/backend/CopyInsertionPass.h"
#include "ir/passes/backend/CopyLoweringPass.h"
#include "ir/passes/backend/ImmToReg.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "sema/sem.h"
#include "type/type_res.h"
#include <argparse/argparse.hpp>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

using namespace z;

int main(int argc, char** argv) {
    bool dump_ast_untyped = false;
    bool dump_ast = false;
    bool dump_constraints = false;
    bool dump_bytecode = false;
    bool verify_ir = false;
    bool no_colour = false;
    bool opt = false;
    bool show_inst_ids = false;
    std::string print_ir;
    std::string print_ir_before;
    std::string print_ir_after;
    std::string input;

    auto z = argparse::ArgumentParser("z");

    try {
        z.add_argument("INPUT").help("input file").store_into(input);
        z.add_argument("-O", "--opt")
            .help("enable optimization")
            .flag()
            .store_into(opt);
        z.add_argument("--print-ast-untyped")
            .help("print parsed AST before type resolution")
            .flag()
            .store_into(dump_ast_untyped);
        z.add_argument("--print-ast")
            .help("print typed AST")
            .flag()
            .store_into(dump_ast);
        z.add_argument("--print-type-constraints")
            .help("print type constraints")
            .flag()
            .store_into(dump_constraints);
        z.add_argument("--print-ir")
            .help("print IR")
            .default_value(std::string{"none"})
            .implicit_value(std::string{"final"})
            .nargs(argparse::nargs_pattern::optional)
            .choices("none", "final", "initial", "all");
        z.add_argument("--print-ir-before")
            .help("print IR before a pass")
            .store_into(print_ir_before);
        z.add_argument("--print-ir-after")
            .help("print IR after a pass")
            .store_into(print_ir_after);
        z.add_argument("--show-inst-ids")
            .help("display ids of IR instructions")
            .flag()
            .store_into(show_inst_ids);
        z.add_argument("--print-bytecode")
            .help("print bytecode")
            .flag()
            .store_into(dump_bytecode);
        z.add_argument("--verify-ir")
            .help("verify IR after passes")
            .flag()
            .store_into(verify_ir);
        z.add_argument("--no-color")
            .help("disable colored output")
            .flag()
            .store_into(no_colour);
        z.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << '\n';
        std::cerr << z;
        return 1;
    }

    print_ir = z.get("--print-ir");

    auto c = ZContext::CreateFromPath(input);
    if (!c)
        return 1;
    auto ctxt = std::move(*c);

    if (no_colour)
        colour::disable();

    auto lexer = Lexer(ctxt.src.get());
    auto parser = Parser(lexer, ctxt);
    auto file = parser.parse();

    if (ctxt.diag.has_error())
        return 1;

    if (dump_ast_untyped)
        file->dump(&ctxt, 0, std::cout);

    auto type_res = type::TypeResolver(ctxt);
    auto sem = SemChecker(ctxt);

    type_res.resolve(file.get(), dump_constraints);

    if (ctxt.diag.has_error())
        return 1;

    if (dump_ast)
        file->dump(&ctxt, 0, std::cout);

    file->accept(sem);

    if (ctxt.diag.has_error())
        return 1;

    auto ir_builder = ir::IRBuilder(ctxt);
    auto ir_code = ir_builder.lower_ast(file.get());

    if (ctxt.diag.has_error())
        return 1;

    if (print_ir == "initial" || print_ir == "all")
        ir::IRPrinter(ctxt, show_inst_ids).dump_file(ir_code, std::cout);

    auto pass_mgr =
        ir::PassManager(std::move(print_ir_before), std::move(print_ir_after),
                        [&](const ir::IRFunction& func) {
                            ir::IRPrinter(ctxt, show_inst_ids)
                                .dump_func(func, ir_code, std::cout);
                        });

    if (opt) {
        pass_mgr.add_fixpoint_pass(std::make_unique<ir::ConstFoldPass>());
        pass_mgr.add_fixpoint_pass(std::make_unique<ir::ConstPropPass>());
        pass_mgr.add_fixpoint_pass(std::make_unique<ir::DCEPass>());
        pass_mgr.run_fixpoint_passes(ir_code, ctxt, verify_ir);
    }

    pass_mgr.add_oneshot_pass(std::make_unique<ir::CriticalEdgeSplitPass>());
    pass_mgr.add_oneshot_pass(std::make_unique<ir::ImmToReg>());
    pass_mgr.add_oneshot_pass(std::make_unique<ir::CopyInsertionPass>());
    pass_mgr.add_oneshot_pass(std::make_unique<ir::CopyCoalescingPass>());
    pass_mgr.add_oneshot_pass(std::make_unique<ir::CopyLoweringPass>());
    const bool ir_valid = pass_mgr.run_oneshot_passes(ir_code, ctxt, verify_ir);

    if (print_ir == "final" || print_ir == "all")
        ir::IRPrinter(ctxt, show_inst_ids).dump_file(ir_code, std::cout);

    if (!ir_valid)
        return 1;

    for (auto& func : ir_code.funcs) {
        auto fa = ir::FuncAnalyses(func, ctxt);
        const auto maxlive = ir::compute_maxlive(func, fa.live());
        expect(maxlive <= 256, "func {} has >= 256 active regs", func.id.id,
               maxlive);
    }
}
