cmake_minimum_required(VERSION 3.20)
project(keyexchange C ASM)

set(MBEDTLS_AS_SUBPROJECT)

option(ENABLE_PROGRAMS OFF FORCE)
option(UNSAFE_BUILD ON FORCE)
option(MBEDTLS_FATAL_WARNINGS OFF FORCE)

add_compile_options(-DMBEDTLS_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/mbedtls_config.h")
add_compile_options(-Os)
add_compile_options(-fno-inline)

set(MBEDTLS_TARGET_PREFIX "keyexchange-")
set(ENABLE_TESTING OFF)
add_subdirectory(mbedtls EXCLUDE_FROM_ALL)

add_executable(keyexchange1 stage1.c)
target_link_libraries(keyexchange1 PUBLIC ime-rt)
target_link_options(keyexchange1 PUBLIC -DNR_TASKLETS=1)

add_executable(keyexchange2 stage2.c)
target_link_libraries(keyexchange2 PUBLIC ime-rt)
target_include_directories(keyexchange2 PUBLIC mbedtls/include/)
target_link_libraries(keyexchange2 PUBLIC keyexchange-mbedcrypto)
target_link_options(keyexchange2 PUBLIC -DNR_TASKLETS=1)

add_executable(keyexchange3 stage3.c)
target_link_libraries(keyexchange3 PUBLIC ime-rt)
target_include_directories(keyexchange3 PUBLIC mbedtls/include/)
target_link_libraries(keyexchange3 PUBLIC keyexchange-mbedcrypto)
target_link_options(keyexchange3 PUBLIC -DNR_TASKLETS=1)