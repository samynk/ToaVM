cmake_minimum_required(VERSION 3.28)

project(lifter
    VERSION 0.1.0
    DESCRIPTION "Bytecode lifter via C++26 reflection"
    LANGUAGES CXX
)

include(FetchContent)

FetchContent_Declare(
    angelscript
    GIT_REPOSITORY https://github.com/anjo76/angelscript.git
    GIT_TAG        v2.38.0
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  sdk/angelscript/projects/cmake
)

FetchContent_MakeAvailable(angelscript)

FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG        v6.1.9
)

FetchContent_MakeAvailable(ftxui)
set_target_properties(component dom screen PROPERTIES FOLDER "libs/ftxui")

# output folders for jbc targets
set(JBC_RUNTIME_DIR "${CMAKE_BINARY_DIR}/jbc_runtime")
set(JBC_BIN_DIR     "${JBC_RUNTIME_DIR}")
set(JBC_RES_DIR     "${JBC_RUNTIME_DIR}/resources")

include(util.cmake)


# Set the C++ Standard to 23
set(CMAKE_CXX_STANDARD 26)

# Ensure the standard is required (build fails if compiler is too old)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable compiler-specific extensions (enforces "pure" C++23)
# Set to ON if you need specific GNU or MSVC extras
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)


add_subdirectory(resources/jbc)
add_subdirectory(libs/jbc)
add_subdirectory(libs/jbc_view)

add_subdirectory(demos/read_class_file/src)