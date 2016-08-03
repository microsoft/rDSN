if(UNIX)
    add_compile_options(-fPIC)
endif()

add_subdirectory(core)
add_subdirectory(dev)
add_subdirectory(tools)
add_subdirectory(tests)
add_subdirectory(plugins)

if(DEFINED BUILD_PLUGINS AND BUILD_PLUGINS STREQUAL "TRUE")
    add_subdirectory(plugins_ext)
endif()
