file(GLOB AS_SOURCES "src/*.as")
set(ASBC_FILES "")

foreach(AS_SRC IN LISTS AS_SOURCES)
    get_filename_component(BASENAME "${AS_SRC}" NAME_WE)
    set(ASBC_FILE "${AS_RES_DIR}/${BASENAME}.asbc")

    add_custom_command(
        OUTPUT "${ASBC_FILE}"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory "${AS_RES_DIR}"

        COMMAND
            "$<TARGET_FILE:as_compiler>"
            "${ASBC_FILE}"
            "${AS_SRC}"

        DEPENDS
            as_compiler
            "${AS_SRC}"

        COMMENT
            "Compiling AngelScript ${BASENAME}.as"

        VERBATIM
    )

    list(APPEND ASBC_FILES "${ASBC_FILE}")
endforeach()

add_custom_target(as_resources ALL 
    SOURCES ${AS_SOURCES}
    DEPENDS ${ASBC_FILES}
    COMMENT "Building AngelScript files"
)

set_target_properties(as_resources
    PROPERTIES
        FOLDER "libs/angelscript"
)

# Make the directory containing .asbc files discoverable by tests
set(ASBC_RESOURCES_DIR ${AS_RES_DIR} CACHE INTERNAL "")