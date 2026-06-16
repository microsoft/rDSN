include(ExternalProject)

set(target_url ${PROTOBUF_GIT_SOURCE})
set(git_tag ${PROTOBUF_GIT_TAG})

set(PROTOBUF_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/ext/protobuf")

set (PROTOBUF_CMAKE_ARGS 
    "-Wno-dev;"
    "-DCMAKE_SUPPRESS_DEVELOPER_WARNINGS=ON;"
    "-Dprotobuf_BUILD_TESTS=OFF;"
    "-DCMAKE_INSTALL_PREFIX=${PROTOBUF_INSTALL_PREFIX};"
    "-DCMAKE_INSTALL_BINDIR=${PROTOBUF_INSTALL_PREFIX}/bin;"
    "-DCMAKE_INSTALL_LIBDIR=${PROTOBUF_INSTALL_PREFIX}/lib;"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE};"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON;"
)

if (WIN32)
    list (APPEND PROTOBUF_CMAKE_ARGS "-DWITH_CSHARP=ON;")
endif ()

ExternalProject_Add(protobuf
    GIT_REPOSITORY ${target_url}
    GIT_TAG ${git_tag}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    CMAKE_ARGS ${PROTOBUF_CMAKE_ARGS}
)

install(DIRECTORY ${PROTOBUF_INSTALL_PREFIX}/include/google
    DESTINATION include/ext
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(DIRECTORY ${PROTOBUF_INSTALL_PREFIX}/lib
        DESTINATION .
    FILES_MATCHING PATTERN "*.lib" PATTERN "*.dll" PATTERN "*.so" PATTERN "*.a" PATTERN "cmake" EXCLUDE
)

if (WIN32)
    # Prefer protoc built by protobuf. Fall back to the prebuilt compiler
    # downloaded by pre-require.cmd when this protobuf build does not produce it.
    install(CODE "
        set(PROTOC_EXE \"${PROTOBUF_INSTALL_PREFIX}/bin/protoc.exe\")
        if(NOT EXISTS \"\${PROTOC_EXE}\")
            set(PROTOC_EXE \"${PROJECT_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/protoc.exe\")
        endif()
        if(NOT EXISTS \"\${PROTOC_EXE}\")
            message(FATAL_ERROR \"Cannot find protoc.exe to install\")
        endif()
        file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_SYSTEM_NAME}\" TYPE PROGRAM FILES \"\${PROTOC_EXE}\")
    ")
else()
    install(PROGRAMS ${PROTOBUF_INSTALL_PREFIX}/bin/protoc
        DESTINATION bin/${CMAKE_SYSTEM_NAME}
    )
endif ()

# Specify source dir
set(PROTOBUF_INCLUDE_DIR ${PROTOBUF_INSTALL_PREFIX}/include PARENT_SCOPE)
set(PROTOBUF_LIB_DIR ${PROTOBUF_INSTALL_PREFIX}/lib PARENT_SCOPE)
set(PROTOBUF_BIN_DIR ${PROTOBUF_INSTALL_PREFIX}/bin PARENT_SCOPE)
