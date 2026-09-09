cmake_minimum_required(VERSION 3.20)
project(ToaVMCallsysTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(angelscript
    GIT_REPOSITORY https://github.com/anjo76/angelscript.git
    GIT_TAG v2.38.0
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR sdk/angelscript/projects/cmake
)
FetchContent_MakeAvailable(angelscript)

set(asbc_include "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/angelscript/include")
set(as_include "${angelscript_SOURCE_DIR}/sdk/angelscript/include")

add_executable(generate_callsys_fixture generate_fixture.cpp)
target_link_libraries(generate_callsys_fixture PRIVATE angelscript)
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/callsys_fixture.hpp"
    COMMAND generate_callsys_fixture "${CMAKE_CURRENT_BINARY_DIR}/callsys_fixture.hpp"
    DEPENDS generate_callsys_fixture
    VERBATIM
)
add_custom_target(callsys_fixture DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/callsys_fixture.hpp")

enable_testing()
add_executable(callsys_tests callsys_tests.cpp)
add_dependencies(callsys_tests callsys_fixture)
target_include_directories(callsys_tests PRIVATE
    "${asbc_include}" "${as_include}" "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME callsys COMMAND callsys_tests)

# This additional target exercises the real reflection-generated block.
option(TOAVM_TEST_REFLECTION "Also run CALLSYS through asbc::invoke (GCC 16+)" OFF)
if(TOAVM_TEST_REFLECTION)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
        message(FATAL_ERROR "Reflection tests require GCC 16+ with -freflection")
    endif()
    add_executable(callsys_reflection_tests callsys_tests.cpp)
    add_dependencies(callsys_reflection_tests callsys_fixture)
    set_target_properties(callsys_reflection_tests PROPERTIES CXX_STANDARD 26)
    target_compile_options(callsys_reflection_tests PRIVATE -freflection)
    target_compile_definitions(callsys_reflection_tests PRIVATE TOAVM_TEST_REFLECTION)
    target_include_directories(callsys_reflection_tests PRIVATE
        "${asbc_include}" "${as_include}" "${CMAKE_CURRENT_BINARY_DIR}")
    add_test(NAME callsys_reflection COMMAND callsys_reflection_tests)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(diagnostics
        "Host function is not registered"
        "Host function name is registered more than once"
        "Registered host function does not match the ASBC signature"
        "CALLSYS used-function index is out of range"
        "CALLSYS must reference an application function")
    foreach(case RANGE 1 5)
        math(EXPR diagnostic_index "${case} - 1")
        list(GET diagnostics ${diagnostic_index} diagnostic)
        add_test(NAME callsys_compile_failure_${case}
            COMMAND "${CMAKE_COMMAND}"
                "-DCXX=${CMAKE_CXX_COMPILER}" "-DCASE=${case}"
                "-DASBC_INCLUDE=${asbc_include}" "-DAS_INCLUDE=${as_include}"
                "-DGENERATED_INCLUDE=${CMAKE_CURRENT_BINARY_DIR}"
                "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/compile_fail.cpp"
                "-DDIAGNOSTIC=${diagnostic}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/expect_compile_failure.cmake")
    endforeach()
endif()
