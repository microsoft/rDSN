if (DEFINED DSN_CMAKE_INCLUDED)
else()
    
    set(DSN_ROOT "$ENV{DSN_ROOT}")
    if(NOT EXISTS "${DSN_ROOT}/")
        message(FATAL_ERROR "Please make sure that ${DSN_ROOT} exists.")
    endif()

    include("${DSN_ROOT}/bin/dsn.cmake")
endif()

set(MY_PROJ_NAME dsn.dist.service.test.simple_kv)
# add_definitions(-DDSN_IN_CORE)
# set(DSN_IN_CORE "TRUE")

# Source files under CURRENT project directory will be automatically included.
# You can manually set MY_PROJ_SRC to include source files under other directories.
set(MY_PROJ_SRC "")

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_INC_PATH ../../common)

set(MY_BOOST_PACKAGES system)

set(MY_PROJ_LIBS gtest
                 dsn.dist.service.replication_lib
                 dsn.dist.service.meta_server_lib
                 dsn.dist.service.common
                 dsn.dist.service.fd.multimaster
                 dsn.dist.service.fd
                 dsn.dist.service.zookeeper_provider
                 zookeeper_mt
                 dsn.dev.utility
)

set(MY_PROJ_LIB_PATH 
        ${GTEST_LIB_DIR}
        ${ZOOKEEPER_LIB_DIR}
        )

# Extra files that will be installed
FILE(GLOB CASE_FILES "case-*")
set(MY_BINPLACES
    "${CMAKE_CURRENT_SOURCE_DIR}/test.sh"
    "${CMAKE_CURRENT_SOURCE_DIR}/addcase.sh"
    "${CASE_FILES}"
)

if(WIN32)
    SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /ignore:4049 /ignore:4217")
endif()

dsn_add_executable()
add_dependencies(${MY_PROJ_NAME} googletest)
