add_executable(as_compiler
    main.cpp
)

target_compile_features(as_compiler PRIVATE cxx_std_20)
target_link_libraries(as_compiler PRIVATE asbc angelscript)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(as_compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()

