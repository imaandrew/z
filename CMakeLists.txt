cmake_minimum_required(VERSION 3.10)
project(z)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(z src/main.cpp src/lexer.cpp src/parser.cpp src/type.cpp src/sym_table.cpp src/type_res.cpp src/scope.cpp)

target_include_directories(z PRIVATE include)