add_executable(as_compiler
    main.cpp
)

target_compile_features(angelscript_compiler PRIVATE cxx_std_20)
target_link_libraries(angelscript_compiler PRIVATE demo_host_api)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(angelscript_compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()

