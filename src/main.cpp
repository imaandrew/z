#include "lexer.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2)
        return 1;

    auto path = std::filesystem::path(argv[1]);

    auto file_size = std::filesystem::file_size(path);

    std::ifstream file(path, std::ios::binary);
    std::vector<char> buf(file_size);

    file.read(buf.data(), file_size);

    auto lexer = Lexer(buf);
    auto t = lexer.lex_token();
    while (t.kind != TokenKind::Eof) {
        std::cout << t.to_string() << '\n';
        t = lexer.lex_token();
    }
}
