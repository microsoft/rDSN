include(ExternalProject)

set(target_url ${RAPIDJSON_GIT_SOURCE})
set(git_tag ${RAPIDJSON_GIT_TAG})

set (RAPIDJSON_CMAKE_ARGS
    "-Wno-dev;"
    "-DCMAKE_SUPPRESS_DEVELOPER_WARNINGS=ON;"
)

ExternalProject_Add(rapidjson_header
    GIT_REPOSITORY ${target_url}
    GIT_TAG ${git_tag}
    GIT_SHALLOW TRUE
    GIT_PROGRESS FALSE
    CMAKE_ARGS ${RAPIDJSON_CMAKE_ARGS}
    INSTALL_COMMAND ""
)

# Specify source dir
ExternalProject_Get_Property(rapidjson_header source_dir)
set(RAPIDJSON_INCLUDE_DIR ${source_dir}/include PARENT_SCOPE)
