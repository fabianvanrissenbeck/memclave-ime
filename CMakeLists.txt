cmake_minimum_required(VERSION 3.13)
project(ime C ASM)

set(CMAKE_C_STANDARD 11)

add_executable(ime src/core/core.s src/core/core.h src/core/core_util.s src/core/core_crypt.c src/core/core_iram.s src/core/core_load.s src/core/core_mem.s src/core/core_scan.c src/core/core_thrd.s src/core/core_wram.s)
target_compile_options(ime PRIVATE -Os)
target_include_directories(ime PRIVATE src/common)

set_target_properties(ime PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)
target_link_options(ime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)
target_link_options(ime PUBLIC -nostartfiles)
target_link_options(ime PUBLIC -DNR_TASKLETS=16)

add_executable(user src/user/main.c src/user/boot.s)
target_include_directories(user PRIVATE src/common)

set_target_properties(user PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/user.ld)
target_link_options(user PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ld/user.ld)
target_link_options(user PUBLIC -nostartfiles)
