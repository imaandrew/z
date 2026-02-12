sources = files(
  'core/zctxt.cpp',
  'ir/ir_builder.cpp',
  'ir/ir_printer.cpp', 
  'lexer/lexer.cpp',
  'parser/parser.cpp',
  'sema/scope.cpp',
  'sema/sem.cpp',
  'sema/sym_table.cpp',
  'type/constraint_gen.cpp',
  'type/constraint_solve.cpp',
  'type/type_res.cpp',
  'type/type.cpp',
)


zlib = library(
  'z',
  sources,
  cpp_args: args,
  dependencies : deps
)