set(ReadASByteCode "read_asbc")
add_executable(${ReadASByteCode}
    main.cpp
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${ReadASByteCode} PRIVATE -freflection)
endif()

ConfigureASProject(${ReadASByteCode} "demos/asbc")