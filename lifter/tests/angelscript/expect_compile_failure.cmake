execute_process(
    COMMAND "${CXX}" -std=c++20 -fsyntax-only "-DFAILURE_CASE=${CASE}"
        "-I${ASBC_INCLUDE}" "-I${AS_INCLUDE}" "-I${GENERATED_INCLUDE}"
        "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(result EQUAL 0)
    message(FATAL_ERROR "Expected compilation to fail for case ${CASE}")
endif()
string(FIND "${error}" "${DIAGNOSTIC}" found)
if(found EQUAL -1)
    message(FATAL_ERROR "Expected diagnostic '${DIAGNOSTIC}', got: ${output}\n${error}")
endif()
