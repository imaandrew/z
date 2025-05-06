#include "lexer.h"
#include "parser.h"
#include "sourceman.h"
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
        decl->print(0);
    }
}
