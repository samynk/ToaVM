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