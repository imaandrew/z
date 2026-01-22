#include "lexer.h"
#include "parser.h"
#include "sem.h"
#include "type_res.h"
#include "zctxt.h"
#include <argparse/argparse.hpp>
#include <exception>
#include <iostream>
#include <string>
#include <utility>

using namespace z;

int main(int argc, char** argv) {
    bool dump_ast_untyped = false;
    bool dump_ast = false;
    bool dump_constraints = false;
    std::string input;

    auto z = argparse::ArgumentParser("z");

    try {
        z.add_argument("INPUT").help("input file").store_into(input);
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

    auto lexer = Lexer(ctxt.src.get());
    auto parser = Parser(lexer, ctxt);
    auto file = parser.parse();

    if (dump_ast_untyped)
        file->dump(&ctxt, 0, std::cout);

    auto type_res = type::TypeResolver(ctxt);
    auto sem = SemChecker(ctxt);

    type_res.resolve(file.get(), dump_constraints);

    if (dump_ast)
        file->dump(&ctxt, 0, std::cout);

    file->accept(sem);

    if (ctxt.diag.has_error())
        return 1;
}
