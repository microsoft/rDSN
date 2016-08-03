if (DEFINED DSN_CMAKE_INCLUDED)
else()
    
    set(DSN_ROOT "$ENV{DSN_ROOT}")
    if(NOT EXISTS "${DSN_ROOT}/")
        message(FATAL_ERROR "Please make sure that ${DSN_ROOT} exists.")
    endif()

    include("${DSN_ROOT}/bin/dsn.cmake")
endif()

set(MY_PROJ_NAME dsn.dist.service.ddlclient)

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_INC_PATH "")

set(MY_BOOST_PACKAGES system)

set(MY_PROJ_LIBS 
    dsn.dist.service.common
    dsn.dist.service.ddlclient_lib
    )

set(MY_PROJ_LIB_PATH)

set(MY_BINPLACES "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")

if(WIN32)
    SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /ignore:4049 /ignore:4217")
endif()

dsn_add_executable()
