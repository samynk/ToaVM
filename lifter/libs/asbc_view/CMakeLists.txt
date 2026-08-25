add_library(asbc_view STATIC
    "inc/CPULog.hpp"
    "src/CPULog.cpp"
    "inc/ClassViewer.hpp"
    "src/ClassViewer.cpp"
    "inc/AccessFlags.hpp"
)

target_include_directories(asbc_view
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:inc>
)

target_link_libraries(asbc_view PUBLIC
    asbc
    ftxui::screen 
    ftxui::dom 
    ftxui::component
)
set_target_properties(asbc_view PROPERTIES FOLDER "libs/asbc")