add_subdirectory(tools.common)
add_subdirectory(tools.nfs)
add_subdirectory(dist.uri.resolver)
add_subdirectory(apps.echo)
add_subdirectory(apps.skv)

if (WIN32)
    add_subdirectory(apps.echo.csharp)
endif()
