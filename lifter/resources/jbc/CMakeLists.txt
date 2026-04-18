# resources/jbc/CMakeLists.txt
find_program(JAVAC javac REQUIRED)

file(GLOB JAVA_SOURCES "src/*.java")
set(CLASS_FILES "")

foreach(JAVA_SRC ${JAVA_SOURCES})
    get_filename_component(BASENAME ${JAVA_SRC} NAME_WE)
    set(CLASS_FILE "${CMAKE_CURRENT_BINARY_DIR}/${BASENAME}.class")
    add_custom_command(
        OUTPUT ${CLASS_FILE}
        COMMAND ${JAVAC} -d ${CMAKE_CURRENT_BINARY_DIR} ${JAVA_SRC}
        DEPENDS ${JAVA_SRC}
    )
    list(APPEND CLASS_FILES ${CLASS_FILE})
endforeach()

add_custom_target(jbc_resources ALL DEPENDS ${CLASS_FILES})

# Make the directory containing .class files discoverable by tests
set(JBC_RESOURCES_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "")