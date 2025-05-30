cmake_minimum_required(VERSION 3.13)
project(ime C ASM)

set(CMAKE_C_STANDARD 11)

add_executable(ime src/main.c src/boot.s src/boot.h src/core.h src/core.c src/core.s src/global.c src/global.h)
target_compile_options(ime PRIVATE -Os)

set(CMAKE_C_LINK_EXECUTABLE "ld.lld -T '${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld' <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set_target_properties(ime PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)

# target_link_options(ime PUBLIC -DSTACK_SIZE_DEFAULT=512)
# target_link_options(ime PUBLIC -DNR_TASKLETS=16)
# target_link_options(ime PUBLIC -nostartfiles)
# target_link_options(ime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)
# set_target_properties(ime PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)