include(ExternalProject)

set(target_url ${THRIFT_GIT_SOURCE})
set(git_tag ${THRIFT_GIT_TAG})

set(THRIFT_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/ext/thrift")

set(THRIFT_CMAKE_ARGS
    "-Wno-dev;"
    "-DCMAKE_SUPPRESS_DEVELOPER_WARNINGS=ON;"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE};"
    "-DWITH_JAVA=OFF;"
    "-DWITH_JAVASCRIPT=OFF;"
    "-DWITH_NODEJS=OFF;"
    "-DWITH_PYTHON=OFF;"
    "-DWITH_C_GLIB=OFF;"
    "-DWITH_CPP=ON;"
    "-DBUILD_CPP=ON;"
    "-DBUILD_TESTING=OFF;"
    "-DBUILD_EXAMPLES=OFF;"
    "-DWITH_QT5=OFF;"
    "-DWITH_QT4=OFF;"
    "-DWITH_OPENSSL=OFF;"
    "-DBUILD_TUTORIALS=OFF;"
    "-DWITH_LIBEVENT=OFF;"
    "-DCMAKE_INSTALL_PREFIX=${THRIFT_INSTALL_PREFIX};"
)

if(DEFINED BOOST_ROOT)
    list(APPEND THRIFT_CMAKE_ARGS "-DBOOST_ROOT=${BOOST_ROOT};")
endif()

set(THRIFT_CXX_FLAGS "")

if (NOT WIN32)
    set(THRIFT_CXX_FLAGS "${THRIFT_CXX_FLAGS} -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-deprecated-declarations")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(THRIFT_CXX_FLAGS "${THRIFT_CXX_FLAGS} -Wno-unused-private-field")
    endif()
endif()

if(DEFINED DSN_BOOST_HEADER_ONLY AND DSN_BOOST_HEADER_ONLY)
    set(THRIFT_CXX_FLAGS "${THRIFT_CXX_FLAGS} -DBOOST_ERROR_CODE_HEADER_ONLY")
endif()

if(WIN32)
    list(APPEND THRIFT_CMAKE_ARGS
        "-DBUILD_COMPILER=OFF;"
        "-DBUILD_CSHARP=ON;"
        "-DWITH_MT=ON;"
        "-DWITH_SHARED_LIB=OFF;"
        "-DBOOST_INCLUDEDIR=${BOOST_INCLUDEDIR};"
        "-DBOOST_LIBRARYDIR=${BOOST_LIBRARYDIR};"
    )
else()
    list(APPEND THRIFT_CMAKE_ARGS
        "-DBUILD_COMPILER=ON;"
        "-DWITH_STATIC_LIB=OFF;"
    )
endif()

if(THRIFT_CXX_FLAGS)
    list(APPEND THRIFT_CMAKE_ARGS "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} ${THRIFT_CXX_FLAGS};")
endif()

ExternalProject_Add(thrift
    GIT_REPOSITORY ${target_url}
    GIT_TAG ${git_tag}
    GIT_SHALLOW TRUE
    GIT_PROGRESS FALSE
    CMAKE_ARGS ${THRIFT_CMAKE_ARGS}
)

install(DIRECTORY ${THRIFT_INSTALL_PREFIX}/include/thrift
    DESTINATION include/ext
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.tcc" PATTERN "*.cpp"
)

install(DIRECTORY ${THRIFT_INSTALL_PREFIX}/lib
    DESTINATION .
    FILES_MATCHING PATTERN "*.lib" PATTERN "*.dll" PATTERN "*.so*" PATTERN "*.dylib*" PATTERN "*.a*"
)

# Copy the thrift compiler into the SDK codegen folder. Unix-like platforms build
# it from source; Windows uses the prebuilt compiler downloaded by pre-require.cmd.
if (WIN32)
    install(PROGRAMS ${PROJECT_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/thrift.exe
        DESTINATION bin/${CMAKE_SYSTEM_NAME}
    )
else()
    install(PROGRAMS ${THRIFT_INSTALL_PREFIX}/bin/thrift
        DESTINATION bin/${CMAKE_SYSTEM_NAME}
    )
endif ()

if (WIN32)
    STRING(REGEX REPLACE "/" "\\\\" WIN_BOOST_INCLUDE_DIR ${BOOST_INCLUDEDIR})
    STRING(REGEX REPLACE "/" "\\\\" WIN_INSTALL_BOOST_DIR ${CMAKE_INSTALL_PREFIX}/include/ext/boost)

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/install_boost.cmd.template" "${CMAKE_CURRENT_SOURCE_DIR}/install_boost.cmd")

    install(CODE "execute_process(COMMAND \"${CMAKE_CURRENT_SOURCE_DIR}/install_boost.cmd\")")
endif ()

# Specify source dir
set(THRIFT_INCLUDE_DIR ${THRIFT_INSTALL_PREFIX}/include PARENT_SCOPE)
set(THRIFT_LIB_DIR ${THRIFT_INSTALL_PREFIX}/lib PARENT_SCOPE)
