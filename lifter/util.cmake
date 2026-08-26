function(ConfigureJBCProject ProjectName PROJECT_FOLDER )
    target_link_libraries(${ProjectName} PUBLIC jbc jbc_view)
    add_dependencies(${ProjectName} jbc_resources)

    message(STATUS "Debugger working dir = ${JBC_BIN_DIR}")

    set_target_properties(${ProjectName} 
        PROPERTIES 
            FOLDER ${PROJECT_FOLDER}
            RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${JBC_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${JBC_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${JBC_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${JBC_BIN_DIR}"
            PDB_OUTPUT_DIRECTORY     "${JBC_BIN_DIR}"
           
    )
endfunction()

function(ConfigureASProject ProjectName PROJECT_FOLDER )
    target_link_libraries(${ProjectName} PUBLIC asbc angelscript)
    add_dependencies(${ProjectName} as_resources)

    message(STATUS "Debugger working dir = ${AS_BIN_DIR}")

    set_target_properties(${ProjectName} 
        PROPERTIES 
            FOLDER ${PROJECT_FOLDER}
            RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${AS_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${AS_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${AS_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${AS_BIN_DIR}"
            PDB_OUTPUT_DIRECTORY     "${AS_BIN_DIR}"
           
    )
endfunction()

function(ConfigureASReflectionProject ProjectName PROJECT_FOLDER)
    target_link_libraries(
        ${ProjectName}
        PUBLIC
            asbc
            angelscript
    )

    add_dependencies(
        ${ProjectName}
        as_resources
    )

    # Require standard C++26 rather than GNU extensions.
    set_target_properties(
        ${ProjectName}
        PROPERTIES
            CXX_STANDARD          26
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS        NO
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16.0)
            message(
                FATAL_ERROR
                "${ProjectName} requires GCC 16.0 or newer; "
                "found GCC ${CMAKE_CXX_COMPILER_VERSION}"
            )
        endif()

        # Still required for reflection support in GCC 16.1.
        target_compile_options(
            ${ProjectName}
            PRIVATE
                $<$<COMPILE_LANGUAGE:CXX>:-freflection>
        )
    else()
        message(
            FATAL_ERROR
            "${ProjectName} currently requires GCC 16.1 with -freflection; "
            "found ${CMAKE_CXX_COMPILER_ID} "
            "${CMAKE_CXX_COMPILER_VERSION}"
        )
    endif()

    message(STATUS "Debugger working dir = ${AS_BIN_DIR}")

    set_target_properties(
        ${ProjectName}
        PROPERTIES
            FOLDER "${PROJECT_FOLDER}"

            RUNTIME_OUTPUT_DIRECTORY_DEBUG
                "${AS_BIN_DIR}"

            RUNTIME_OUTPUT_DIRECTORY_RELEASE
                "${AS_BIN_DIR}"

            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO
                "${AS_BIN_DIR}"

            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL
                "${AS_BIN_DIR}"

            PDB_OUTPUT_DIRECTORY
                "${AS_BIN_DIR}"
    )
endfunction()