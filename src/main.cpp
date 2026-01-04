#include "lexer.h"
#include "parser.h"
#include "sem.h"
#include "src_mgr.h"
#include "sym_table.h"
#include "type_res.h"
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
    auto sm = SourceManager::CreateFromPath(input);
    if (!sm) {
        return 1;
    }

    auto source_mgr = sm.value();
    auto lexer = Lexer(&source_mgr);
    auto parser = Parser(lexer, &source_mgr);
    auto file = parser.parse();

    if (z["--dump-ast-untyped"] == true)
        file->dump(&source_mgr, 0, std::cout);

    auto syms = SymbolTable(&source_mgr);
    auto type_res = type::TypeResolver(&syms, &source_mgr);
    auto sem = SemChecker(&syms, &source_mgr);

    type_res.resolve(file.get(), dump_constraints);

    if (z["--dump-ast"] == true)
        file->dump(&source_mgr, 0, std::cout);

    file->accept(sem);
}
