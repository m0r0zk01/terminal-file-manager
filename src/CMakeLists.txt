include_directories(${FileManager_SOURCE_DIR}/inc)

add_library(fm_impl fm_impl.c)
add_library(terminal terminal.c)
add_library(columns columns.c)
add_library(files files.c)
add_library(config config.c)
