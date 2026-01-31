project(
  'z',
  'cpp',
  default_options : [
    'cpp_std=c++23',
    'warning_level=3',
    'cpp_rtti=false',
  ]
)

cc = meson.get_compiler('cpp')

deps = []
args = []

if get_option('enable_stacktrace')
  args += '-DENABLE_STACKTRACE'
  
  # backtrace_lib = cc.find_library('stdc++_libbacktrace', required : true)
  exp_lib = cc.find_library('stdc++exp', required : true)
  
  deps += [exp_lib]
endif

deps += dependency('magic_enum')

argparse = dependency('argparse')

inc = include_directories('src')

subdir('src')

executable('z', 'src/main.cpp', link_with : zlib, include_directories : inc, dependencies : deps + argparse, cpp_args: args)

if get_option('tests')
    subdir('tests')
endif