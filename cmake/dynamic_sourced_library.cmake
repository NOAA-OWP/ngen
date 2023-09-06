## Name : dynamic_sourced_library
## Params: LIB_NAME, WORK_DIR
## Add/create a new library with the given name, passing as source files all the '*.cpp' files found in the given working directory for the library
function(dynamic_sourced_cxx_library LIB_NAME WORK_DIR)
    file(GLOB_RECURSE lib_CPP_FILES_LIST CONFIGURE_DEPENDS "*.cpp")
    add_library(${LIB_NAME} STATIC ${lib_CPP_FILES_LIST})
endfunction()
