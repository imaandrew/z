#include "core/colour.h"
#include "core/zctxt.h"
#include "ir/ir_builder.h"
#include "ir/ir_printer.h"
#include "ir/pass_mgr.h"
#include "ir/passes/ConstFoldPass.h"
#include "ir/passes/ConstPropPass.h"
#include "ir/passes/DCEPass.h"
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
    bool dump_ir = false;
    bool no_colour = false;
    bool opt = false;
    std::string input;

    auto z = argparse::ArgumentParser("z");

    try {
        z.add_argument("INPUT").help("input file").store_into(input);
        z.add_argument("-O", "--opt")
            .help("enable optimization")
            .flag()
            .store_into(opt);
        z.add_argument("--dump-ast-untyped")
            .help("print parsed AST before type resolution")
            .flag()
            .store_into(dump_ast_untyped);
        z.add_argument("--dump-ast")
            .help("print typed AST")
            .flag()
            .store_into(dump_ast);
        z.add_argument("--dump-type-constraints")
            .help("print type constraints")
            .flag()
            .store_into(dump_constraints);
        z.add_argument("--dump-ir").help("print IR").flag().store_into(dump_ir);
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

    auto c = ZContext::Create(input);
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

    if (opt) {
        auto pass_mgr = ir::PassManager();
        pass_mgr.add_pass(std::make_unique<ir::ConstFoldPass>());
        pass_mgr.add_pass(std::make_unique<ir::ConstPropPass>());
        pass_mgr.add_pass(std::make_unique<ir::DCEPass>());
        pass_mgr.run_passes(ir_code);
    }

    if (dump_ir) {
        ir::IRPrinter().dump(ir_code, ctxt, std::cout);
    }
}
