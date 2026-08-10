add_library(abc STATIC
    "include/asbc/Opcodes.hpp"
)

message("Creating jbc library")

target_include_directories(jbc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
set_target_properties(jbc PROPERTIES FOLDER "libs/jbc")