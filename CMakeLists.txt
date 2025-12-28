cmake_minimum_required(VERSION 3.13)
project(ime C ASM)

set(CMAKE_C_STANDARD 11)

add_executable(ime src/core/core.s src/core/core.h src/core/core_util.s src/core/core_crypt.c src/core/core_iram.s src/core/core_load.S src/core/core_mem.s src/core/core_scan.c src/core/core_thrd.s src/core/core_wram.s src/core/aead.c src/core/aead.h src/core/chacha.S src/poly/poly.c src/poly/poly.h src/poly/poly.S src/core/core_stats.c)
target_compile_options(ime PRIVATE -Os)
target_include_directories(ime PRIVATE src/common)
target_compile_definitions(ime PRIVATE -DIME_REPORT_STATS=0)

set_target_properties(ime PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)
target_link_options(ime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)
target_link_options(ime PUBLIC -nostartfiles)
target_link_options(ime PUBLIC -DNR_TASKLETS=16)
# this option is ignored in boot.s
target_link_options(ime PUBLIC -DSTACK_SIZE_DEFAULT=8)
target_link_options(ime PUBLIC -DSTACK_SIZE_TASKLET_0=4096)
target_compile_definitions(ime PUBLIC -DNR_TASKLETS=16)

add_library(ime-rt src/ime-rt/boot.s src/ime-rt/ime.c src/ime-rt/ime.h src/ime-rt/aead.c src/ime-rt/aead.h src/ime-rt/barrier.c src/ime-rt/barrier.h)
set_target_properties(ime-rt PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/user.ld)
target_include_directories(ime-rt PUBLIC src/ime-rt)
target_include_directories(ime-rt PUBLIC src/poly)
target_link_options(ime-rt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ld/user.ld)
target_link_options(ime-rt PUBLIC -nostartfiles)

add_executable(user-crypto src/user-crypto/main.c)
target_link_libraries(user-crypto PUBLIC ime-rt)
target_link_options(user-crypto PUBLIC -DNR_TASKLETS=1)
target_compile_definitions(user-crypto PUBLIC -DNR_TASKLETS=1)

add_executable(add-example src/add/main.c)
target_link_libraries(add-example PUBLIC ime-rt)
target_link_options(add-example PUBLIC -DNR_TASKLETS=1)
target_compile_definitions(add-example PUBLIC -DNR_TASKLETS=1)

add_executable(mbench-example src/mbench/main.c)
target_link_libraries(mbench-example PUBLIC ime-rt)
target_link_options(mbench-example PUBLIC -DNR_TASKLETS=16)

add_executable(gemv-example src/gemv/main.c)
target_link_libraries(gemv-example PUBLIC ime-rt)
target_link_options(gemv-example PUBLIC -DNR_TASKLETS=16)

add_executable(mlp-example src/MLP/main.c)
target_link_libraries(mlp-example PUBLIC ime-rt)
target_link_options(mlp-example PUBLIC -DNR_TASKLETS=16)

add_executable(bfs-example src/BFS/main.c)
target_link_libraries(bfs-example PUBLIC ime-rt)
target_link_options(bfs-example PUBLIC -DNR_TASKLETS=16)

add_executable(va-example src/VA/main.c)
target_link_libraries(va-example PUBLIC ime-rt)
target_link_options(va-example PUBLIC -DNR_TASKLETS=16)

add_executable(hsts-example src/HST-S/main.c)
target_link_libraries(hsts-example PUBLIC ime-rt)
target_link_options(hsts-example PUBLIC -DNR_TASKLETS=16)

add_executable(hstl-example src/HST-L/main.c)
target_link_libraries(hstl-example PUBLIC ime-rt)
target_link_options(hstl-example PUBLIC -DNR_TASKLETS=16)

add_executable(scanssa-example src/SCAN-SSA/main.c)
target_link_libraries(scanssa-example PUBLIC ime-rt)
target_link_options(scanssa-example PUBLIC -DNR_TASKLETS=16)

add_executable(scanrss-example src/SCAN-RSS/main.c)
target_link_libraries(scanrss-example PUBLIC ime-rt)
target_link_options(scanrss-example PUBLIC -DNR_TASKLETS=16)

add_executable(red-example src/RED/main.c)
target_link_libraries(red-example PUBLIC ime-rt)
target_link_options(red-example PUBLIC -DNR_TASKLETS=16)

add_executable(trns-example src/TRNS/main.c)
target_link_libraries(trns-example PUBLIC ime-rt)
target_link_options(trns-example PUBLIC -DNR_TASKLETS=16)

add_executable(sel-example src/SEL/main.c)
target_link_libraries(sel-example PUBLIC ime-rt)
target_link_options(sel-example PUBLIC -DNR_TASKLETS=16)

add_executable(uni-example src/UNI/main.c)
target_link_libraries(uni-example PUBLIC ime-rt)
target_link_options(uni-example PUBLIC -DNR_TASKLETS=16)

add_executable(bs-example src/BS/main.c)
target_link_libraries(bs-example PUBLIC ime-rt)
target_link_options(bs-example PUBLIC -DNR_TASKLETS=16)

add_executable(ts-example src/TS/main.c)
target_link_libraries(ts-example PUBLIC ime-rt)
target_link_options(ts-example PUBLIC -DNR_TASKLETS=16)

add_executable(spmv-example src/SpMV/main.c)
target_link_libraries(spmv-example PUBLIC ime-rt)
target_link_options(spmv-example PUBLIC -DNR_TASKLETS=16)

add_executable(nw-example src/NW/main.c)
target_link_libraries(nw-example PUBLIC ime-rt)
target_link_options(nw-example PUBLIC -DNR_TASKLETS=16)

add_executable(msg src/msg/main.c)
target_link_libraries(msg PUBLIC ime-rt)
target_link_options(msg PUBLIC -DNR_TASKLETS=1)

add_executable(multi-tasklet src/multi-tasklet/main.c)
target_link_libraries(multi-tasklet PUBLIC ime-rt)
target_link_options(multi-tasklet PUBLIC -DNR_TASKLETS=16)

#add_executable(sk-load-bench src/sk-load-bench/main.c src/poly/poly.c src/poly/poly.S)
#target_include_directories(sk-load-bench PRIVATE src/core)
#target_link_libraries(sk-load-bench PUBLIC ime-rt)
#target_link_options(sk-load-bench PUBLIC -DNR_TASKLETS=1)

#add_executable(dma-bench src/dma/main.c)
#target_link_libraries(dma-bench PUBLIC ime-rt)
#target_link_options(dma-bench PUBLIC -DNR_TASKLETS=1)
#target_compile_options(dma-bench PUBLIC -O3)

#add_executable(wram2-bench src/wram2/main.c)
#target_link_libraries(wram2-bench PUBLIC ime-rt)
#target_link_options(wram2-bench PUBLIC -DNR_TASKLETS=16)
#target_compile_options(wram2-bench PUBLIC -DNR_TASKLETS=16)
#target_compile_options(wram2-bench PUBLIC -O3)

add_executable(stop src/stop/main.c)
target_link_libraries(stop PUBLIC ime-rt)

add_executable(chacha-bench src/chacha-bench/main.c)
target_link_libraries(chacha-bench PUBLIC ime-rt)
target_link_options(chacha-bench PUBLIC -DNR_TASKLETS=16)
target_compile_definitions(chacha-bench PUBLIC -DNR_TASKLETS=16)

add_executable(crypto-bench src/crypto-bench/main.c)
target_link_libraries(crypto-bench PUBLIC ime-rt)
target_link_options(crypto-bench PUBLIC -DNR_TASKLETS=1)
target_compile_definitions(crypto-bench PUBLIC -DNR_TASKLETS=1)

# add_library(mbedtls-local src/keyexchange/mbedtls/library/dhm.c src/keyexchange/mbedtls/include/mbedtls/dhm.h src/keyexchange/mbedtls/library/bignum.c src/keyexchange/mbedtls/library/bignum_core.c src/keyexchange/mbedtls/library/constant_time.c)
# target_include_directories(mbedtls-local PUBLIC src/keyexchange/mbedtls/include)
# target_compile_definitions(mbedtls-local PUBLIC MBEDTLS_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/src/keyexchange/mbedtls_config.h")
# target_compile_options(mbedtls-local PRIVATE -Oz)

add_subdirectory(src/keyexchange ${CMAKE_CURRENT_BINARY_DIR}/keyexchange EXCLUDE_FROM_ALL)
add_subdirectory(src/fsl ${CMAKE_CURRENT_BINARY_DIR}/fsl EXCLUDE_FROM_ALL)
