gtest = dependency('gtest', main : true)

test_sources = files(
  'test_lexer.cpp',
)

ztest = executable(
  'ztest',
  test_sources,
  link_with : zlib,
  include_directories : inc,
  dependencies : gtest
)

test('ztest', ztest)
