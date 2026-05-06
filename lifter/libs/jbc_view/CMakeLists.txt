add_library(jbc_view STATIC
    "inc/CPULog.hpp"
    "src/CPULog.cpp"
    "inc/ClassViewer.hpp"
    "src/ClassViewer.cpp"
    "inc/AccessFlags.hpp"
)

target_include_directories(jbc_view
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:inc>
)

target_link_libraries(jbc_view PUBLIC
    jbc
    ftxui::screen 
    ftxui::dom 
    ftxui::component
)
set_target_properties(jbc_view PROPERTIES FOLDER "libs/jbc")