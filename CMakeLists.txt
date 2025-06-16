cmake_minimum_required(VERSION 3.10)
project(pnglite C)

enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS "Build shared library" OFF)

add_library(pnglite
    src/pnglite.c
)

target_include_directories(pnglite PUBLIC include)
target_compile_definitions(pnglite PRIVATE
  $<$<BOOL:${BUILD_SHARED_LIBS}>:PNG_BUILD_DLL>
)

add_executable(test_ctx tests/ctx.c)

target_link_libraries(test_ctx PRIVATE pnglite)

target_include_directories(test_ctx PRIVATE include)

add_test(NAME ctx_test COMMAND test_ctx)
