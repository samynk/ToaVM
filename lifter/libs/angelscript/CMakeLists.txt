add_library(asbc STATIC
    "include/asbc/frame.hpp"
    "include/asbc/Opcodes.hpp"
    "include/asbc/host_api.hpp"
    "src/host_api.cpp"
)
target_link_libraries(asbc PUBLIC angelscript)
target_compile_features(asbc PRIVATE cxx_std_20)
message("Creating asbc library")

target_include_directories(asbc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
set_target_properties(asbc PROPERTIES FOLDER "libs/angelscript/asbc")