project(
  'z',
  'cpp',
  default_options : [
    'cpp_std=c++23',
    'warning_level=3',
    'cpp_rtti=false',
  ]
)

argparse = dependency('argparse')

inc = include_directories('include')

subdir('src')

executable('z', 'src/main.cpp', link_with : zlib, include_directories : inc, dependencies: argparse)

if get_option('tests')
    subdir('tests')
endif