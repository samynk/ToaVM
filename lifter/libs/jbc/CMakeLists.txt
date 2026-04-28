add_library(jbc INTERFACE)

message("Creating jbc library")

target_include_directories(jbc
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_sources(jbc INTERFACE 
    FILE_SET HEADERS
    BASE_DIRS "include" FILES
     "include/jbc/Opcodes.hpp")



set_target_properties(jbc PROPERTIES FOLDER "libs")