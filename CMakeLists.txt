cmake_minimum_required(VERSION 3.13)
project(ime C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_LINK_EXECUTABLE "ld.lld <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

# add_executable(ime src/core/main.c src/core/boot.s src/core/boot.h src/core/core.h src/core/core.c src/core/core.s)
add_executable(ime src/core/core.s src/core/core.h src/core/core_crypt.c src/core/core_iram.s src/core/core_load.s src/core/core_mem.s src/core/core_scan.c src/core/core_thrd.s src/core/core_wram.s)
target_compile_options(ime PRIVATE -Os)
target_include_directories(ime PRIVATE src/common)

set_target_properties(ime PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)
target_link_options(ime PUBLIC -T ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)

add_executable(user src/user/main.c src/user/boot.s)
target_include_directories(user PRIVATE src/common)

set_target_properties(user PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/user.ld)
target_link_options(user PUBLIC -T ${CMAKE_CURRENT_SOURCE_DIR}/ld/user.ld -v)

set_target_properties(ime PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)