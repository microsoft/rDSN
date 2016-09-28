<?php
require_once($argv[1]); // type.php
require_once($argv[2]); // program.php
$file_prefix = $argv[3];
$idl_format = $argv[4];

$my_serialization = "";
if ($idl_format == "thrift")
{
    $my_serialization = "thrift";
} else if ($idl_format == "proto")
{
    $my_serialization = "protobuf";
}
?>
set(MY_PROJ_NAME "<?=$_PROG->name?>")

if (DEFINED DSN_CMAKE_INCLUDED)
else()
    project(${MY_PROJ_NAME} C CXX)
    set(DSN_ROOT "$ENV{DSN_ROOT}")
    if(NOT EXISTS "${DSN_ROOT}/")
        message(FATAL_ERROR "Please make sure that ${DSN_ROOT} exists.")
    endif()

    include("${DSN_ROOT}/bin/dsn.cmake")
endif()

# Source files under CURRENT project directory will be automatically included.
# You can manually set MY_PROJ_SRC to include source files under other directories.
file(GLOB
    MY_PROJ_SRC
<?php if ($idl_format == "thrift") { ?>
   "${CMAKE_CURRENT_SOURCE_DIR}/thrift/*.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/thrift/*.h"
<?php } else { ?>
    ""
<?php } ?>
)

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_INC_PATH "")

set(MY_PROJ_LIBS "")

set(MY_PROJ_LIB_PATH "")

set(INI_FILES "")

set(MY_SERIALIZATION_TYPE "<?=$my_serialization?>")

file(GLOB
    RES_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ini"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.sh"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cmd"
    "${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile"
    )

# Extra files that will be installed
set(MY_BINPLACES ${RES_FILES})

set(MY_BOOST_PACKAGES "")

dsn_add_shared_library()

#add_custom_target( docker 
#   COMMAND docker build -t "${MY_PROJ_NAME}-image" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MY_PROJ_NAME}")
#add_dependencies( docker "${MY_PROJ_NAME}")
