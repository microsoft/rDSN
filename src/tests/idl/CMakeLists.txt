if (DEFINED DSN_CMAKE_INCLUDED)
else()
    set(DSN_ROOT "$ENV{DSN_ROOT}")
    if(NOT EXISTS "${DSN_ROOT}/")
        message(FATAL_ERROR "Please make sure that ${DSN_ROOT} exists.")
    endif()

    include("${DSN_ROOT}/bin/dsn.cmake")
endif()

add_definitions(-DDSN_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../..")

set(MY_PROJ_NAME dsn.idl.tests)

# Source files under CURRENT project directory will be automatically included.
# You can manually set MY_PROJ_SRC to include source files under other directories.
set(MY_PROJ_SRC "")

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_INC_PATH
    ${GTEST_INCLUDE_DIR} 
)

set(MY_PROJ_LIBS gtest
)

set(MY_PROJ_LIB_PATH
    "${GTEST_LIB_DIR}"
)

# Extra files that will be installed

set(MY_BINPLACES test.sh test.cmd)
set(MY_PROJ_BINDIRS "")

set(MY_SERIALIZATION_TYPE
    "thrift"
    "protobuf"
)

dsn_add_executable()
add_dependencies(${MY_PROJ_NAME} googletest)
add_dependencies(${MY_PROJ_NAME} thrift)
add_dependencies(${MY_PROJ_NAME} protobuf)
