cmake_minimum_required(VERSION 3.13)
project(ime C ASM)

set(CMAKE_C_STANDARD 11)

add_library(mbedtls-local mbedtls/library/chachapoly.c mbedtls/library/chacha20.c mbedtls/library/poly1305.c)
target_include_directories(mbedtls-local PUBLIC mbedtls/include)
target_compile_definitions(mbedtls-local PUBLIC MBEDTLS_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/mbedtls_config.h")
target_compile_options(mbedtls-local PRIVATE -Os)

add_executable(ime src/main.c src/boot.s src/boot.h)
# target_link_libraries(ime PUBLIC mbedtls-local)
target_compile_options(ime PRIVATE -Os)
target_link_options(ime PUBLIC -DSTACK_SIZE_DEFAULT=512)
target_link_options(ime PUBLIC -DNR_TASKLETS=16)
target_link_options(ime PUBLIC -nostartfiles)