cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED DSN_ROOT OR "${DSN_ROOT}" STREQUAL "")
    get_filename_component(DSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()
if(NOT DEFINED MODE OR "${MODE}" STREQUAL "")
    set(MODE "CHECK")
endif()
if(NOT MODE STREQUAL "CHECK" AND NOT MODE STREQUAL "UPDATE")
    message(FATAL_ERROR "MODE must be CHECK or UPDATE")
endif()

set(JS_BINDING_SOURCE "${DSN_ROOT}/include/dsn/js")
set(JS_BINDING_WEBSTUDIO
    "${DSN_ROOT}/src/tools/webstudio/app_package/static/js/dsn")
set(JS_BINDING_FILES thrift.js dsn_transport.js dsn_types.js)

foreach(JS_BINDING_FILE IN LISTS JS_BINDING_FILES)
    set(SOURCE_FILE "${JS_BINDING_SOURCE}/${JS_BINDING_FILE}")
    set(WEBSTUDIO_FILE "${JS_BINDING_WEBSTUDIO}/${JS_BINDING_FILE}")
    if(NOT EXISTS "${SOURCE_FILE}")
        message(FATAL_ERROR "Canonical JavaScript binding file is missing: ${SOURCE_FILE}")
    endif()

    if(MODE STREQUAL "UPDATE")
        file(COPY_FILE "${SOURCE_FILE}" "${WEBSTUDIO_FILE}" ONLY_IF_DIFFERENT)
    elseif(NOT EXISTS "${WEBSTUDIO_FILE}")
        message(FATAL_ERROR "Webstudio JavaScript binding file is missing: ${WEBSTUDIO_FILE}")
    else()
        file(SHA256 "${SOURCE_FILE}" SOURCE_HASH)
        file(SHA256 "${WEBSTUDIO_FILE}" WEBSTUDIO_HASH)
        if(NOT SOURCE_HASH STREQUAL WEBSTUDIO_HASH)
            message(FATAL_ERROR
                "${WEBSTUDIO_FILE} is stale; run the sync_js_binding build target")
        endif()
    endif()
endforeach()

message(STATUS "JavaScript binding webstudio files are ${MODE}")
