#include "lexer.h"
#include "parser.h"
#include "sourceman.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2)
        return 1;

/*     auto path = std::filesystem::path(argv[1]);

    auto file_size = std::filesystem::file_size(path);

    std::ifstream file(path, std::ios::binary);
    std::vector<char> buf(file_size);

    file.read(buf.data(), file_size);
 */
    auto source_man = SourceManager::Create(argv[1]);
    auto lexer = Lexer(source_man);
    auto parser = Parser(lexer, source_man);
    auto f = parser.parse();
    for (const auto& d : f) {
        d->print(0);
    }
}
