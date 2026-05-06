add_library(jbc STATIC
    "include/jbc/Opcodes.hpp"
    "include/jbc/ClassFile.hpp"
    "include/jbc/ClassReader.hpp"
    "src/jbc/interpreter.cpp"
    "src/jbc/ClassFile.cpp"
)

message("Creating jbc library")

target_include_directories(jbc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
set_target_properties(jbc PROPERTIES FOLDER "libs/jbc")