cmake_minimum_required(VERSION 3.13)
project(ime C ASM)

set(CMAKE_C_STANDARD 11)

add_library(mbedtls-local mbedtls/library/sha256.c)
target_include_directories(mbedtls-local PUBLIC mbedtls/include)
target_compile_definitions(mbedtls-local PUBLIC MBEDTLS_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/mbedtls_config.h")
target_compile_options(mbedtls-local PRIVATE -Os)

add_executable(ime src/main.c)
target_link_libraries(ime PUBLIC mbedtls-local)
target_compile_options(ime PRIVATE -Os)