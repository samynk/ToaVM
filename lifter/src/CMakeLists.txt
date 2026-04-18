add_executable(lifter_main
    main.cpp
)

target_link_libraries(lifter_main
    PRIVATE
        lifter::lifter
)