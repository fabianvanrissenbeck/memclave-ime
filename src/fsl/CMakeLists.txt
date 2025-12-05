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

set(MBEDTLS_TARGET_PREFIX "fsl-")
set(ENABLE_TESTING OFF)
add_subdirectory(mbedtls EXCLUDE_FROM_ALL)

find_program(PYTHON NAMES python3 python REQUIRED)
find_program(OBJCOPY NAMES llvm-objcopy REQUIRED)

function(add_subkernel_target OUT SKNAME SUBPROJECT)
    add_custom_command(
            OUTPUT ${OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ${PYTHON}
            ARGS sk.py $<TARGET_FILE:${SKNAME}> ${OUT} auth
            DEPENDS ${SKNAME}
    )

    get_filename_component(TARGET_PREFIX ${OUT} NAME_WLE)
    add_custom_target(${TARGET_PREFIX}-sk DEPENDS ${OUT})
    add_dependencies(${TARGET_PREFIX}-sk ${SKNAME})
    set_source_files_properties(${OUT} PROPERTIES GENERATED TRUE)
    set_target_properties(${TARGET_PREFIX}-sk PROPERTIES LOCATION ${OUT})
endfunction()

function(extract_iram_target TARGET OUT)
    add_custom_command(
            OUTPUT ${OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ${OBJCOPY}
            ARGS --only-section .text -O binary $<TARGET_FILE:ime> ${OUT}
            VERBATIM
            DEPENDS ime
    )

    add_custom_target(${TARGET} DEPENDS ${OUT})
    set_target_properties(${TARGET} PROPERTIES LOCATION ${OUT})
    set_source_files_properties(${OUT} PROPERTIES GENERATED TRUE)
    add_dependencies(${TARGET} ime)
endfunction()

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

function(add_subkernel_resource TO SYMBOL SUBKERNEL)
    get_target_property(SKFILE ${SUBKERNEL} LOCATION)
    add_binary_resource(${TO} ${SYMBOL} ${SKFILE})
    add_dependencies(${TO} ${SUBKERNEL})
endfunction()

add_executable(fsl main.c boot.S replace.S)
target_link_libraries(fsl PUBLIC fsl-mbedcrypto)
set_target_properties(fsl PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/link.ld)
target_link_options(fsl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/link.ld)
target_link_options(fsl PUBLIC "-nostartfiles")
target_link_options(fsl PUBLIC -DNR_TASKLETS=24)
target_link_options(fsl PUBLIC -DSTACK_SIZE_DEFAULT=8)
target_link_options(fsl PUBLIC -DSTACK_SIZE_TASKLET_0=2048)

# add_binary_resource(fsl g_tl sk/ime.iram.raw)

extract_iram_target(ime-iram-raw ${CMAKE_CURRENT_BINARY_DIR}/ime.iram.raw)
add_subkernel_target(${CMAKE_CURRENT_BINARY_DIR}/msg.sk msg "")
add_subkernel_target(${CMAKE_CURRENT_BINARY_DIR}/xchg1.sk keyexchange1 "keyexchange")
add_subkernel_target(${CMAKE_CURRENT_BINARY_DIR}/xchg2.sk keyexchange2 "keyexchange")
add_subkernel_target(${CMAKE_CURRENT_BINARY_DIR}/xchg3.sk keyexchange3 "keyexchange")

add_subkernel_resource(fsl g_sk_msg msg-sk)
add_subkernel_resource(fsl g_sk_xchg_1 xchg1-sk)
add_subkernel_resource(fsl g_sk_xchg_2 xchg2-sk)
add_subkernel_resource(fsl g_sk_xchg_3 xchg3-sk)
add_subkernel_resource(fsl g_tl ime-iram-raw)