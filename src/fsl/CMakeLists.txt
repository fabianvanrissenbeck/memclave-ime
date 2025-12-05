cmake_minimum_required(VERSION 3.20)
project(fsl ASM C)

set(CMAKE_C_STANDARD 11)
set(MBEDTLS_AS_SUBPROJECT)

option(ENABLE_PROGRAMS OFF FORCE)
option(UNSAFE_BUILD ON FORCE)
option(MBEDTLS_FATAL_WARNINGS OFF FORCE)

add_compile_options(-DMBEDTLS_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/mbedtls_config.h")
add_compile_options(-Os)
add_compile_options(-fno-inline)

add_subdirectory(mbedtls EXCLUDE_FROM_ALL)

find_program(PYTHON NAMES python3 python REQUIRED)

function(add_binary_resource TO SYMBOL BINNAME)
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SYMBOL}.c
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ${PYTHON}
            ARGS ${CMAKE_CURRENT_SOURCE_DIR}/arrfy.py ${BINNAME} ${SYMBOL} ${CMAKE_CURRENT_BINARY_DIR}/${SYMBOL}.c
            VERBATIM
            MAIN_DEPENDENCY ${BINNAME}
    )

    add_custom_target(${SYMBOL}-bin DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${SYMBOL}.c)
    set_source_files_properties(${SYMBOL}.c PROPERTIES GENERATED TRUE)
    target_sources(${TO} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${SYMBOL}.c)
    add_dependencies(${TO} ${SYMBOL}-bin)
endfunction()

add_executable(fsl main.c boot.S replace.S)
target_link_libraries(fsl PUBLIC mbedcrypto)
set_target_properties(fsl PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/link.ld)
target_link_options(fsl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/link.ld)
target_link_options(fsl PUBLIC "-nostartfiles")
target_link_options(fsl PUBLIC -DNR_TASKLETS=24)
target_link_options(fsl PUBLIC -DSTACK_SIZE_DEFAULT=8)
target_link_options(fsl PUBLIC -DSTACK_SIZE_TASKLET_0=2048)

add_binary_resource(fsl g_tl sk/ime.iram.raw)
add_binary_resource(fsl g_sk_xchg_1 sk/xchg1.sk)
add_binary_resource(fsl g_sk_xchg_2 sk/xchg2.sk)
add_binary_resource(fsl g_sk_xchg_3 sk/xchg3.sk)
add_binary_resource(fsl g_sk_msg sk/msg.sk)