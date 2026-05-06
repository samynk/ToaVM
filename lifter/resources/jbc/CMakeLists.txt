# resources/jbc/CMakeLists.txt
find_program(JAVAC javac REQUIRED)

file(GLOB JAVA_SOURCES "src/*.java")
set(CLASS_FILES "")

foreach(JAVA_SRC ${JAVA_SOURCES})
    get_filename_component(BASENAME ${JAVA_SRC} NAME_WE)
    set(CLASS_FILE "${JBC_RES_DIR}/${BASENAME}.class")
    add_custom_command(
        OUTPUT ${CLASS_FILE}
        COMMAND ${JAVAC} -d ${JBC_RES_DIR} ${JAVA_SRC}
        DEPENDS ${JAVA_SRC}
    )
    list(APPEND CLASS_FILES ${CLASS_FILE})
endforeach()

add_custom_target(jbc_resources ALL 
    SOURCES ${JAVA_SOURCES}
    DEPENDS ${CLASS_FILES}
    COMMENT "Building java class files"
)

set_target_properties(jbc_resources
    PROPERTIES
        FOLDER "libs/jbc"
)

# Make the directory containing .class files discoverable by tests
set(JBC_RESOURCES_DIR ${JBC_RES_DIR} CACHE INTERNAL "")