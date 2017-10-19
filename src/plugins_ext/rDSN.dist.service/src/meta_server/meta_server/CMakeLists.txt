if (DEFINED DSN_CMAKE_INCLUDED)
else()
    
    set(DSN_ROOT "$ENV{DSN_ROOT}")
    if(NOT EXISTS "${DSN_ROOT}/")
        message(FATAL_ERROR "Please make sure that ${DSN_ROOT} exists.")
    endif()

    include("${DSN_ROOT}/bin/dsn.cmake")
endif()

add_definitions(-DDSN_META_SERVER_DYNAMIC_LIB)

set(MY_PROJ_NAME dsn.dist.service.meta_server)

# Source files under CURRENT project directory will be automatically included.
# You can manually set MY_PROJ_SRC to include source files under other directories.
set(MY_PROJ_SRC "")
ms_find_source_files("CXX" "${CMAKE_CURRENT_SOURCE_DIR}/../meta_server_lib" "GLOB" MY_PROJ_SRC)

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_INC_PATH ../../common ../../fd/failure_detector ../meta_server_lib)

set(MY_PROJ_LIBS
    dsn.dist.service.common
    dsn.dist.service.fd
    dsn.dist.service.fd.multimaster
    dsn.dist.service.zookeeper_provider
    zookeeper_mt
    gtest
    )

set(MY_PROJ_LIB_PATH "")

# Extra files that will be installed
set(MY_BINPLACES "")

dsn_add_shared_library()

if(NOT ("$ENV{APPVEYOR}" STREQUAL ""))
    message(STATUS "APPVEYOR = $ENV{APPVEYOR}, skip unit test for ${MY_PROJ_NAME}")
else()
    file(COPY test/ DESTINATION "${CMAKE_BINARY_DIR}/test/${MY_PROJ_NAME}")
endif()
