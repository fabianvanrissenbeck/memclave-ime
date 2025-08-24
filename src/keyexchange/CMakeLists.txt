cmake_minimum_required(VERSION 3.20)
project(keyexchange C ASM)

set(MBEDTLS_AS_SUBPROJECT)

option(ENABLE_PROGRAMS OFF FORCE)
option(UNSAFE_BUILD ON FORCE)
option(MBEDTLS_FATAL_WARNINGS OFF FORCE)

add_compile_options(-DMBEDTLS_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/mbedtls_config.h")
add_compile_options(-Os)
add_compile_options(-fno-inline)

add_subdirectory(mbedtls)

add_executable(keyexchange main.c)
target_link_libraries(keyexchange PUBLIC ime-rt)
target_include_directories(keyexchange PUBLIC mbedtls/include/)
target_link_libraries(keyexchange PUBLIC mbedcrypto)
target_link_options(keyexchange PUBLIC -DNR_TASKLETS=1)