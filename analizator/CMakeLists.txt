cmake_minimum_required(VERSION 3.15)
project(analizator C)

set(CMAKE_C_STANDARD 99)

add_executable(analizator gc.c ext_funcs.c vm.c symbols.c sintactic.c analizator.c)