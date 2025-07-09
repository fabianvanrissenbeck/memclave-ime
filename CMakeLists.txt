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

add_library(ime-rt src/ime-rt/boot.s src/ime-rt/ime.h)
set_target_properties(ime-rt PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/user.ld)
target_include_directories(ime-rt PUBLIC src/ime-rt)
target_link_options(ime-rt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ld/user.ld)
target_link_options(ime-rt PUBLIC -nostartfiles)

add_executable(user src/user/main.c)
target_link_libraries(user PUBLIC ime-rt)
target_link_options(user PUBLIC -DNR_TASKLETS=1)

add_executable(msg src/msg/main.c)
target_link_libraries(msg PUBLIC ime-rt)
target_link_options(msg PUBLIC -DNR_TASKLETS=1)
