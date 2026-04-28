cmake_minimum_required(VERSION 3.28)

project(lifter
    VERSION 0.1.0
    DESCRIPTION "Bytecode lifter via C++26 reflection"
    LANGUAGES CXX
)
include(util.cmake)


# Set the C++ Standard to 23
set(CMAKE_CXX_STANDARD 23)

# Ensure the standard is required (build fails if compiler is too old)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable compiler-specific extensions (enforces "pure" C++23)
# Set to ON if you need specific GNU or MSVC extras
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(resources/jbc)
add_subdirectory(libs/jbc)



add_subdirectory(src)