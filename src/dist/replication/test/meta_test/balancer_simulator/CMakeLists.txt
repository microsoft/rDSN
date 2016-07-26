if (DEFINED DSN_CMAKE_INCLUDED)
else()
    cmake_minimum_required(VERSION 2.8.8)

    set(DSN_ROOT "$ENV{DSN_ROOT}")
    if(NOT EXISTS "${DSN_ROOT}/")
        message(FATAL_ERROR "Please make sure that ${DSN_ROOT} exists.")
    endif()

    include("${DSN_ROOT}/bin/dsn.cmake")
endif()

set(MY_PROJ_NAME sim_lb)

# Source files under CURRENT project directory will be automatically included.
# You can manually set MY_PROJ_SRC to include source files under other directories.
set(MY_PROJ_SRC ../misc/misc.cpp)

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_INC_PATH ../../../meta_server ../../../client_lib ${GTEST_INCLUDE_DIR})

set(MY_PROJ_LIBS dsn.replication.meta_server
                 dsn.replication.clientlib
                 gtest)

set(MY_PROJ_LIB_PATH "")

# Extra files that will be installed
set(MY_BINPLACES "")

dsn_add_executable()
add_dependencies(${MY_PROJ_NAME} googletest)
