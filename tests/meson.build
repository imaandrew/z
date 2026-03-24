gtest_dep = dependency('gtest', main : true, required : true)

test_lexer = executable(
    'test_lexer',
    'test_lexer.cpp',
    include_directories : inc,
    dependencies : deps + [argparse, gtest_dep],
    cpp_args : args,
    link_with : zlib,
)

test('Lexer Tests', test_lexer)
