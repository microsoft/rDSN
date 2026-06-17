set(project_name googletest)

set(target_url ${GTEST_GIT_SOURCE})
set(git_tag ${GTEST_GIT_TAG})
set(target_bin_subdir src)
set(gtest_lib_dir "${PROJECT_BINARY_DIR}/${project_name}/${target_bin_subdir}")

set(my_cmake_args "-Wno-dev;-DCMAKE_SUPPRESS_DEVELOPER_WARNINGS=ON;-Dgtest_force_shared_crt=OFF;-DBUILD_SHARED_LIBS=ON;-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${gtest_lib_dir};-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=${gtest_lib_dir};")

if(WIN32)
    set(target_binaries gtest.lib gtest_main.lib gtest.dll gtest_main.dll)
elseif(APPLE)
    set(target_binaries libgtest.dylib libgtest_main.dylib)
else()
    set(target_binaries libgtest.so libgtest_main.so)
endif()

include(${PROJECT_SOURCE_DIR}/bin/dsn.ext.cmake)

install(DIRECTORY ${my_source_dir}/include/gtest
    DESTINATION include/ext
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.tcc" PATTERN "*.cpp"
)

set(GTEST_INCLUDE_DIR ${my_source_dir}/include PARENT_SCOPE)
set(GTEST_LIB_DIR ${my_binary_dir}/${target_bin_subdir} PARENT_SCOPE)
