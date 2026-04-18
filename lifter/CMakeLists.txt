cmake_minimum_required(VERSION 3.28)

project(lifter
    VERSION 0.1.0
    DESCRIPTION "Bytecode lifter via C++26 reflection"
    LANGUAGES CXX
)

add_library(lifter INTERFACE)
add_library(lifter::lifter ALIAS lifter)

target_include_directories(lifter INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

add_subdirectory(src)