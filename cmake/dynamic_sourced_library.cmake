## Name : dynamic_sourced_library
## Params: LIB_NAME, WORK_DIR
## Add/create a new library with the given name, passing as source files all the '*.cpp' files found in the given working directory for the library
function(dynamic_sourced_cxx_library LIB_NAME WORK_DIR)
    execute_process(COMMAND ls
            COMMAND grep .cpp
            WORKING_DIRECTORY "${WORK_DIR}"
            OUTPUT_VARIABLE lib_CPP_FILES
            )
    string(REGEX REPLACE "\n$" "" lib_CPP_FILES "${lib_CPP_FILES}")
    string(REPLACE "\n" ";" lib_CPP_FILES ${lib_CPP_FILES})
    set(lib_CPP_FILES_LIST ${lib_CPP_FILES})

    add_library(${LIB_NAME} STATIC ${lib_CPP_FILES_LIST})
endfunction()
