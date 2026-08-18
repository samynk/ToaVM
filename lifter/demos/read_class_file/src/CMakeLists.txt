set(LifterMain "lifter_main")
add_executable(${LifterMain}
    main.cpp
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(lifter_main PRIVATE -freflection)
endif()

ConfigureJBCProject(${LifterMain} "demos/jbc")