cmake_minimum_required(VERSION 3.13)
project(ime C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_LINK_EXECUTABLE "ld.lld <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

add_executable(ime src/core/main.c src/core/boot.s src/core/boot.h src/core/core.h src/core/core.c src/core/core.s src/common/global.c src/common/global.h)
target_compile_options(ime PRIVATE -Os)
target_include_directories(ime PRIVATE src/common)

set_target_properties(ime PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)
target_link_options(ime PUBLIC -T ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)

add_executable(user src/user/main.c src/user/boot.s)
set_target_properties(user PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/user.ld)
target_link_options(user PUBLIC -T ${CMAKE_CURRENT_SOURCE_DIR}/ld/user.ld -v)

# target_link_options(ime PUBLIC -DSTACK_SIZE_DEFAULT=512)
# target_link_options(ime PUBLIC -DNR_TASKLETS=16)
# target_link_options(ime PUBLIC -nostartfiles)
# target_link_options(ime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)
# set_target_properties(ime PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ld/core.ld)