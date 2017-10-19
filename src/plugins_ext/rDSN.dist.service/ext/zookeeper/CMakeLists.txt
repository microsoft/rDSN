set(project_name zookeeper)

set(target_url ${ZOOKEEPER_GIT_SOURCE})
set(git_tag ${ZOOKEEPER_GIT_TAG})

set(my_cmake_args "")

if(WIN32)
    set(target_binaries zookeeper_mt.dll zookeeper_mt.lib)
else()
    set(target_binaries libzookeeper_mt.a)
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/bin/dsn.ext.cmake")
    include("${PROJECT_SOURCE_DIR}/bin/dsn.ext.cmake")
else()
    include("$ENV{DSN_ROOT}/bin/dsn.ext.cmake")
endif()

set(ZOOKEEPER_INCLUDE_DIR ${my_source_dir}/include PARENT_SCOPE)
set(ZOOKEEPER_LIB_DIR ${my_binary_dir} PARENT_SCOPE)

