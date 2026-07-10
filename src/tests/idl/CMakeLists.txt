if (DEFINED DSN_CMAKE_INCLUDED)
else()
    set(DSN_ROOT "$ENV{DSN_ROOT}")
    if(NOT EXISTS "${DSN_ROOT}/")
        message(FATAL_ERROR "Please make sure that ${DSN_ROOT} exists.")
    endif()

    include("${DSN_ROOT}/bin/dsn.cmake")
endif()

file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../.." DSN_ROOT_DIR_FOR_CPP)
file(TO_CMAKE_PATH "${CMAKE_INSTALL_PREFIX}" DSN_INSTALL_ROOT_DIR_FOR_CPP)
add_definitions(-DDSN_ROOT_DIR="${DSN_ROOT_DIR_FOR_CPP}")
add_definitions(-DDSN_INSTALL_ROOT_DIR="${DSN_INSTALL_ROOT_DIR_FOR_CPP}")
if(NOT DEFINED DSN_BUILD_DIR OR "${DSN_BUILD_DIR}" STREQUAL "")
    set(DSN_BUILD_DIR "${CMAKE_BINARY_DIR}")
endif()
file(TO_CMAKE_PATH "${DSN_BUILD_DIR}" DSN_BUILD_DIR_FOR_CPP)
add_definitions(-DDSN_BUILD_DIR="${DSN_BUILD_DIR_FOR_CPP}")
if(DEFINED BOOST_INCLUDEDIR AND NOT "${BOOST_INCLUDEDIR}" STREQUAL "")
    file(TO_CMAKE_PATH "${BOOST_INCLUDEDIR}" DSN_BOOST_INCLUDEDIR_FOR_CPP)
    add_definitions(-DDSN_BOOST_INCLUDEDIR="${DSN_BOOST_INCLUDEDIR_FOR_CPP}")
endif()
if(DEFINED GTEST_LIB_DIR AND NOT "${GTEST_LIB_DIR}" STREQUAL "")
    file(TO_CMAKE_PATH "${GTEST_LIB_DIR}" DSN_GTEST_LIB_DIR_FOR_CPP)
    add_definitions(-DDSN_GTEST_LIB_DIR="${DSN_GTEST_LIB_DIR_FOR_CPP}")
endif()
find_program(DSN_NODE_EXECUTABLE_PATH NAMES node nodejs)
if(DSN_NODE_EXECUTABLE_PATH)
    # The Node-driven test (thrift_helper.js_runtime_compatibility) exercises the
    # generated JavaScript runtime, which uses ES modules and dynamic import() and
    # therefore needs a modern Node.js. The minimum is declared in
    # include/dsn/js/package.json ("engines": { "node": ">=N" }). Only enable the
    # test when the installed runtime satisfies that minimum; otherwise skip it so
    # older systems (e.g. the distro Node.js 10 on Ubuntu 16.04) do not fail on
    # unsupported syntax.
    set(DSN_JS_MIN_NODE_MAJOR 18)
    set(DSN_JS_PACKAGE_JSON "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/dsn/js/package.json")
    if(EXISTS "${DSN_JS_PACKAGE_JSON}")
        file(READ "${DSN_JS_PACKAGE_JSON}" DSN_JS_PACKAGE_JSON_CONTENT)
        string(REGEX MATCH "\"node\"[ \t]*:[ \t]*\"[^\"0-9]*([0-9]+)" DSN_JS_ENGINE_MATCH "${DSN_JS_PACKAGE_JSON_CONTENT}")
        if(DSN_JS_ENGINE_MATCH)
            set(DSN_JS_MIN_NODE_MAJOR ${CMAKE_MATCH_1})
        endif()
    endif()

    execute_process(
        COMMAND "${DSN_NODE_EXECUTABLE_PATH}" --version
        OUTPUT_VARIABLE DSN_NODE_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE DSN_NODE_VERSION_RESULT)
    set(DSN_NODE_MAJOR_VERSION 0)
    if(DSN_NODE_VERSION_RESULT EQUAL 0)
        string(REGEX MATCH "([0-9]+)" DSN_NODE_MAJOR_MATCH "${DSN_NODE_VERSION_OUTPUT}")
        if(DSN_NODE_MAJOR_MATCH)
            set(DSN_NODE_MAJOR_VERSION ${DSN_NODE_MAJOR_MATCH})
        endif()
    endif()

    if(DSN_NODE_MAJOR_VERSION LESS DSN_JS_MIN_NODE_MAJOR)
        message(STATUS "dsn.idl.tests: skipping the JS runtime test; Node.js '${DSN_NODE_VERSION_OUTPUT}' at ${DSN_NODE_EXECUTABLE_PATH} is older than the required v${DSN_JS_MIN_NODE_MAJOR}")
    else()
        file(TO_CMAKE_PATH "${DSN_NODE_EXECUTABLE_PATH}" DSN_NODE_EXECUTABLE_FOR_CPP)
        add_definitions(-DDSN_NODE_EXECUTABLE="${DSN_NODE_EXECUTABLE_FOR_CPP}")
        message(STATUS "dsn.idl.tests: enabling the JS runtime test with Node.js '${DSN_NODE_VERSION_OUTPUT}' at ${DSN_NODE_EXECUTABLE_PATH}")
    endif()
endif()

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
