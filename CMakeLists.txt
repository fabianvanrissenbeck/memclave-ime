cmake_minimum_required(VERSION 3.13)
project(ime C ASM)

set(CMAKE_C_STANDARD 11)

add_executable(ime src/core/core.s src/core/core.h src/core/core_util.s src/core/core_crypt.c src/core/core_iram.s src/core/core_load.s src/core/core_mem.s src/core/core_scan.c src/core/core_thrd.s src/core/core_wram.s src/core/aead.c src/core/aead.h src/core/chacha.S src/core/poly.c src/core/poly.h src/core/poly.s)
target_compile_options(ime PRIVATE -Os)
target_include_directories(ime PRIVATE src/common)

set_target_properties(ime PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)
target_link_options(ime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)
target_link_options(ime PUBLIC -nostartfiles)
target_link_options(ime PUBLIC -DNR_TASKLETS=16)

add_library(ime-rt src/ime-rt/boot.s src/ime-rt/ime.c src/ime-rt/ime.h)
set_target_properties(ime-rt PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/user.ld)
target_include_directories(ime-rt PUBLIC src/ime-rt)
target_link_options(ime-rt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ld/user.ld)
target_link_options(ime-rt PUBLIC -nostartfiles)

add_executable(user src/user/main.c src/user/poly.s)
target_link_libraries(user PUBLIC ime-rt)
target_link_options(user PUBLIC -DNR_TASKLETS=1)

add_executable(add-example src/add/main.c)
target_link_libraries(add-example PUBLIC ime-rt)
target_link_options(add-example PUBLIC -DNR_TASKLETS=1)

add_executable(mbench-example src/mbench/main.c)
target_link_libraries(mbench-example PUBLIC ime-rt)
target_link_options(mbench-example PUBLIC -DNR_TASKLETS=1)

add_executable(gemv-example src/gemv/main.c)
target_link_libraries(gemv-example PUBLIC ime-rt)
target_link_options(gemv-example PUBLIC -DNR_TASKLETS=1)

add_executable(mlp-example src/MLP/main.c)
target_link_libraries(mlp-example PUBLIC ime-rt)
target_link_options(mlp-example PUBLIC -DNR_TASKLETS=1)

add_executable(msg src/msg/main.c)
target_link_libraries(msg PUBLIC ime-rt)
target_link_options(msg PUBLIC -DNR_TASKLETS=1)

add_executable(multi-tasklet src/multi-tasklet/main.c)
target_link_libraries(multi-tasklet PUBLIC ime-rt)
target_link_options(multi-tasklet PUBLIC -DNR_TASKLETS=16)
