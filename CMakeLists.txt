cmake_minimum_required(VERSION 3.10)
project(pnglite C)

enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

option(BUILD_SHARED_LIBS "Build shared library" OFF)

add_library(pnglite
    src/pnglite.c
)

target_include_directories(pnglite PUBLIC include)
target_compile_definitions(pnglite PRIVATE
  $<$<BOOL:${BUILD_SHARED_LIBS}>:PNG_BUILD_DLL>
)

link_directories(/usr/lib64)

add_executable(png_static tests/png_static.c)

target_link_libraries(png_static PRIVATE pnglite)

target_include_directories(png_static PRIVATE include)

add_test(NAME png_static COMMAND png_static)
