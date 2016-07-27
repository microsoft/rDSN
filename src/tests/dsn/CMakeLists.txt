if (DEFINED DSN_CMAKE_INCLUDED)
else()
    cmake_minimum_required(VERSION 2.8.8)

    set(DSN_ROOT "$ENV{DSN_ROOT}")
    if(NOT EXISTS "${DSN_ROOT}/")
        message(FATAL_ERROR "Please make sure that ${DSN_ROOT} exists.")
    endif()

    include("${DSN_ROOT}/bin/dsn.cmake")
endif()

set(MY_PROJ_NAME dsn.tests)

# Source files under CURRENT project directory will be automatically included.
# You can manually set MY_PROJ_SRC to include source files under other directories.
set(MY_PROJ_SRC "")

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

message(STATUS "GTEST_INCLUDE_DIR = ${GTEST_INCLUDE_DIR}")
set(MY_PROJ_INC_PATH
    ${GTEST_INCLUDE_DIR} 
    ../../dist/failure_detector 
    ../../dist/replication/client_lib 
    ../../dist/replication/lib 
    ../../dist/replication/meta_server
    ../../dist/replication/zookeeper
    )

if (UNIX)
    set(MY_PROJ_LIBS gtest pthread)
else()
    set(MY_PROJ_LIBS gtest)
endif()

set(MY_PROJ_LIBS 
    dsn.replication.meta_server
    dsn.replication
    dsn.replication.clientlib
    dsn.failure_detector.multimaster
    dsn.failure_detector
    dsn.replication.zookeeper_provider
    zookeeper_mt
    ${MY_PROJ_LIBS}
    )

set(MY_BOOST_PACKAGES system)

set(MY_PROJ_LIB_PATH 
        ${GTEST_LIB_DIR}
        ${ZOOKEEPER_LIB_DIR}
        )

# Extra files that will be installed
set(MY_BINPLACES
    "${CMAKE_CURRENT_SOURCE_DIR}/test.sh"
    "${CMAKE_CURRENT_SOURCE_DIR}/config-test.ini"
)

dsn_add_executable()
add_dependencies(${MY_PROJ_NAME} googletest)
