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
    auto sm = SourceManager::Create(args[1]);
    if (!sm) {
        return 1;
    }

    auto source_mgr = sm.value();
    auto lexer = Lexer(&source_mgr);
    auto parser = Parser(lexer, &source_mgr);
    auto decls = parser.parse();
    for (const auto& decl : decls) {
        // decl->dump(0);
    }

    auto syms = SymbolTable(&source_mgr);
    auto type_res = TypeResolver(&syms, &source_mgr);
    auto sem = SemChecker(&syms, &source_mgr);

    type_res.fill_top_level_syms(decls);

    for (const auto& decl : decls) {
        decl->accept(type_res);
    }

    for (const auto& decl : decls) {
        decl->dump(&source_mgr);
    }
}
