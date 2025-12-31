#include "lexer.h"
#include "parser.h"
#include "sem.h"
#include "src_mgr.h"
#include "sym_table.h"
#include "type_res.h"
#include <span>

using namespace z;

int main(int argc, char** argv) {
    if (argc < 2)
        return 1;

    auto args = std::span(argv, argc);
    auto sm = SourceManager::Create(args[1]);
    if (!sm) {
        return 1;
    }

    auto source_mgr = sm.value();
    auto lexer = Lexer(&source_mgr);
    auto parser = Parser(lexer, &source_mgr);
    auto file = parser.parse();
    // file->dump(&source_mgr, 0, std::cout);

    auto syms = SymbolTable(&source_mgr);
    auto type_res = type::TypeResolver(&syms, &source_mgr);
    auto sem = SemChecker(&syms, &source_mgr);

    type_res.resolve(file.get());

    file->dump(&source_mgr, 0, std::cout);

    file->accept(sem);
}
