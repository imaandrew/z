cmake_minimum_required(VERSION 3.10)
project(z)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(z src/main.cpp src/lexer.cpp src/parser.cpp)

add_compile_options(-Wall -g)

target_include_directories(z PRIVATE include)