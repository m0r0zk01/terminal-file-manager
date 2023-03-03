cmake_minimum_required(VERSION 3.1)

project(FileManager)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_EXPORT_COMPILE_COMMANDS)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_subdirectory(src)
include_directories(inc)

add_executable(fm main.c)
target_link_libraries(fm fm_impl terminal columns files config)
