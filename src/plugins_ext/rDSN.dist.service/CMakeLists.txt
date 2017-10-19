if (DEFINED DSN_CMAKE_INCLUDED)
else()
    project(dsn.dist.service C CXX)
    set(DSN_ROOT "$ENV{DSN_ROOT}")
    if(NOT EXISTS "${DSN_ROOT}/")
        message(FATAL_ERROR "Please make sure that ${DSN_ROOT} exists.")
    endif()

    include("${DSN_ROOT}/bin/dsn.cmake")
endif()

set(CMAKE_CXX_FLAGS "-D__STDC_FORMAT_MACROS ${CMAKE_CXX_FLAGS}")

# external projects
if((NOT DEFINED DSN_GIT_SOURCE) OR (DSN_GIT_SOURCE STREQUAL ""))
    set(DSN_GIT_SOURCE "github")
endif()
set(GIT_SOURCE_FILE "ext/gitsource.${DSN_GIT_SOURCE}")
include(${GIT_SOURCE_FILE})
add_subdirectory(ext/zookeeper)
add_subdirectory(ext/rapidjson)

# local source 
include_directories(include)
add_subdirectory(src)

