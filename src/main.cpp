#include "lexer.h"
#include "parser.h"
#include "sem.h"
#include "src_mgr.h"
#include "sym_table.h"
#include "type_res.h"
#include <span>

int main(int argc, char** argv) {
    if (argc < 2)
        return 1;

    auto args = std::span(argv, argc);
    auto source_man = SourceManager::Create(args[1]);
    auto lexer = Lexer(&source_man);
    auto parser = Parser(lexer, &source_man);
    auto decls = parser.parse();
    for (const auto& decl : decls) {
        // decl->dump(0);
    }

    auto syms = SymbolTable(&source_man);
    auto type_res = TypeResolver(&syms, &source_man);
    auto sem = SemChecker(&syms, &source_man);

    type_res.fill_top_level_syms(decls);

    for (const auto& decl : decls) {
        decl->accept(type_res);
    }

    for (const auto& decl : decls) {
        decl->dump(0);
    }
}
