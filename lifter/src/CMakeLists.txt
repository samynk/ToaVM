set(LifterMain "lifter_main")
add_executable(${LifterMain}
    main.cpp
)
ConfigureJBCProject(${LifterMain} "JBC ClassFiles")