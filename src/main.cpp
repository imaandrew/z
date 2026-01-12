#include "lexer.h"
#include "parser.h"
#include "sem.h"
#include "type_res.h"
#include "zctxt.h"
#include <argparse/argparse.hpp>
#include <exception>
#include <iostream>

using namespace z;

int main(int argc, char** argv) {
    bool dump_constraints = false;

    auto z = argparse::ArgumentParser("z");

    try {
        z.add_argument("INPUT").help("input file");
        z.add_argument("--dump-ast-untyped")
            .help("print parsed AST before type resolution")
            .flag();
        z.add_argument("--dump-ast").help("print typed AST").flag();
        z.add_argument("--dump-type-constraints")
            .help("print type constraints")
            .flag()
            .store_into(dump_constraints);
        z.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << '\n';
        std::cerr << z;
        std::exit(1); // NOLINT
    }

    auto input = z.get<std::string>("INPUT");
    auto c = ZContext::Create(input);
    if (!c)
        return 1;
    auto ctxt = std::move(*c);

    auto lexer = Lexer(ctxt.src.get());
    auto parser = Parser(lexer, ctxt);
    auto file = parser.parse();

    if (z["--dump-ast-untyped"] == true)
        file->dump(ctxt.src.get(), 0, std::cout);

    auto type_res = type::TypeResolver(ctxt);
    auto sem = SemChecker(ctxt);

    type_res.resolve(file.get(), dump_constraints);

    if (z["--dump-ast"] == true)
        file->dump(ctxt.src.get(), 0, std::cout);

    file->accept(sem);
}
