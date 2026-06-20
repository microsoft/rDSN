@ECHO OFF

SET bin_dir=%~dp0
SET TOP_DIR=%bin_dir%\..\..\
PUSHD "%TOP_DIR%"
SET TOP_DIR=%CD%
POPD
SET build_type=Debug
SET build_dir=
SET boost_dir=
SET buildall=-DBUILD_PLUGINS=FALSE
SET DSN_TMP_BUILD_CSHARP=-DBUILD_CSHARP=FALSE
SET DSN_TMP_BUILD_PROTOBUF_CSHARP=-DBUILD_PROTOBUF_CSHARP=FALSE
SET DSN_TMP_BUILD_THRIFT_CSHARP=-DBUILD_THRIFT_CSHARP=FALSE
SET cdir=%CD%

:parse_args
IF "%~1" EQU "" GOTO args_done
IF /I "%~1" EQU "-h" GOTO usage_exit
IF /I "%~1" EQU "--help" GOTO usage_exit
IF /I "%~1" EQU "-t" (
    IF "%~2" EQU "" GOTO error_usage
    SET build_type=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--type" (
    IF "%~2" EQU "" GOTO error_usage
    SET build_type=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "-d" (
    IF "%~2" EQU "" GOTO error_usage
    FOR %%I IN ("%~2") DO SET build_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--build_dir" (
    IF "%~2" EQU "" GOTO error_usage
    FOR %%I IN ("%~2") DO SET build_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--build-dir" (
    IF "%~2" EQU "" GOTO error_usage
    FOR %%I IN ("%~2") DO SET build_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "-b" (
    IF "%~2" EQU "" GOTO error_usage
    FOR %%I IN ("%~2") DO SET boost_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--boost_dir" (
    IF "%~2" EQU "" GOTO error_usage
    FOR %%I IN ("%~2") DO SET boost_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--build_plugins" (
    SET buildall=-DBUILD_PLUGINS=TRUE
    SET DSN_TMP_BUILD_PLUGINS=TRUE
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "build_plugins" (
    SET buildall=-DBUILD_PLUGINS=TRUE
    SET DSN_TMP_BUILD_PLUGINS=TRUE
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--build_csharp" (
    SET DSN_TMP_BUILD_CSHARP=-DBUILD_CSHARP=TRUE
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "build_csharp" (
    SET DSN_TMP_BUILD_CSHARP=-DBUILD_CSHARP=TRUE
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--build_protobuf_csharp" (
    SET DSN_TMP_BUILD_PROTOBUF_CSHARP=-DBUILD_PROTOBUF_CSHARP=TRUE
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "build_protobuf_csharp" (
    SET DSN_TMP_BUILD_PROTOBUF_CSHARP=-DBUILD_PROTOBUF_CSHARP=TRUE
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--build_thrift_csharp" (
    SET DSN_TMP_BUILD_THRIFT_CSHARP=-DBUILD_THRIFT_CSHARP=TRUE
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "build_thrift_csharp" (
    SET DSN_TMP_BUILD_THRIFT_CSHARP=-DBUILD_THRIFT_CSHARP=TRUE
    SHIFT
    GOTO parse_args
)
IF NOT DEFINED DSN_TMP_BUILD_TYPE_ARG (
    SET build_type=%~1
    SET DSN_TMP_BUILD_TYPE_ARG=TRUE
    SHIFT
    GOTO parse_args
)
IF NOT DEFINED DSN_TMP_BUILD_DIR_ARG (
    FOR %%I IN ("%~1") DO SET build_dir=%%~fI
    SET DSN_TMP_BUILD_DIR_ARG=TRUE
    SHIFT
    GOTO parse_args
)
GOTO error_usage

:args_done
IF /I "%build_type%" EQU "debug" SET build_type=Debug
IF /I "%build_type%" EQU "release" SET build_type=Release
IF /I "%build_type%" EQU "relwithdebinfo" SET build_type=RelWithDebInfo
IF /I "%build_type%" EQU "minsizerel" SET build_type=MinSizeRel
IF NOT "%build_type%" EQU "Debug" IF NOT "%build_type%" EQU "Release" IF NOT "%build_type%" EQU "RelWithDebInfo" IF NOT "%build_type%" EQU "MinSizeRel" GOTO error_usage

IF "%build_dir%" EQU "" (
    SET build_dir=%TOP_DIR%\builder
)

IF "%DSN_TMP_BUILD_PLUGINS%" EQU "TRUE" (
    pushd "%TOP_DIR%\src\plugins_ext"
    git submodule init
    git submodule update
    popd
)
SET DSN_TMP_BUILD_PLUGINS=
SET DSN_TMP_BUILD_TYPE_ARG=
SET DSN_TMP_BUILD_DIR_ARG=

SET DSN_TMP_BUILD_ARCH=%DSN_BUILD_ARCH%
IF "%DSN_TMP_BUILD_ARCH%"=="" IF NOT "%VSCMD_ARG_TGT_ARCH%"=="" SET DSN_TMP_BUILD_ARCH=%VSCMD_ARG_TGT_ARCH%
IF "%DSN_TMP_BUILD_ARCH%"=="" IF /I "%PROCESSOR_ARCHITECTURE%"=="ARM64" SET DSN_TMP_BUILD_ARCH=ARM64
IF "%DSN_TMP_BUILD_ARCH%"=="" SET DSN_TMP_BUILD_ARCH=x64
IF /I "%DSN_TMP_BUILD_ARCH%"=="amd64" SET DSN_TMP_BUILD_ARCH=x64
IF /I "%DSN_TMP_BUILD_ARCH%"=="x64" SET DSN_TMP_BUILD_ARCH=x64
IF /I "%DSN_TMP_BUILD_ARCH%"=="arm64" SET DSN_TMP_BUILD_ARCH=ARM64
IF NOT "%DSN_TMP_BUILD_ARCH%"=="x64" IF NOT "%DSN_TMP_BUILD_ARCH%"=="ARM64" (
    CALL "%bin_dir%\echoc.exe" 4 "Unsupported build architecture '%DSN_TMP_BUILD_ARCH%'. Use DSN_BUILD_ARCH=x64 or DSN_BUILD_ARCH=ARM64."
    GOTO error
)
IF "%DSN_TMP_BUILD_ARCH%"=="x64" SET DSN_TMP_VCVARS_ARCH=amd64
IF "%DSN_TMP_BUILD_ARCH%"=="ARM64" SET DSN_TMP_VCVARS_ARCH=amd64_arm64
IF "%DSN_TMP_BUILD_ARCH%"=="ARM64" IF /I "%PROCESSOR_ARCHITECTURE%"=="ARM64" SET DSN_TMP_VCVARS_ARCH=arm64

IF NOT "%boost_dir%" EQU "" SET DSN_TMP_CUSTOM_BOOST_DIR=%boost_dir%
CALL "%bin_dir%\pre-require.cmd"
IF ERRORLEVEL 1 (
    GOTO error
)
SET DSN_TMP_CUSTOM_BOOST_DIR=

REM detect VS
IF DEFINED DSN_TRAVIS GOTO find_vs2017
IF NOT "%VisualStudioVersion%"=="" GOTO find_vs_by_version
SET DSN_TMP_VS_INSTALL_DIR=
FOR /f "usebackq tokens=1* delims=: " %%i in (`"%bin_dir%\vswhere.exe" -latest`) DO (
  IF /i "%%i"=="installationPath" set DSN_TMP_VS_INSTALL_DIR=%%j
)
IF DEFINED DSN_TMP_VS_INSTALL_DIR (
    IF EXIST "%DSN_TMP_VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvarsall.bat" (
        CALL "%DSN_TMP_VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvarsall.bat" %DSN_TMP_VCVARS_ARCH%
        SET DSN_TMP_VS_INSTALL_DIR=
        GOTO find_vs_by_version
    ) ELSE IF "%DSN_TMP_BUILD_ARCH%"=="x64" IF EXIST "%DSN_TMP_VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat" (
        CALL "%DSN_TMP_VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat"
        SET DSN_TMP_VS_INSTALL_DIR=
        GOTO find_vs_by_version
    )
    SET DSN_TMP_VS_INSTALL_DIR=
)
IF NOT "%VS140COMNTOOLS%"=="" (
    IF EXIST "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" (
        CALL "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" amd64
        GOTO find_vs2015
    )
)

CALL "%bin_dir%\echoc.exe" 4 "Visual Studio 2015, 2017, 2019, 2022 or 2026 is not found, please run 'x64 Native Tools Command Prompt' and try later"
GOTO error

:find_vs_by_version
IF "%VisualStudioVersion%"=="18.0" GOTO find_vs2026
IF "%VisualStudioVersion%"=="17.0" GOTO find_vs2022
IF "%VisualStudioVersion%"=="16.0" GOTO find_vs2019
IF "%VisualStudioVersion%"=="15.0" GOTO find_vs2017
IF "%VisualStudioVersion%"=="14.0" GOTO find_vs2015
CALL "%bin_dir%\echoc.exe" 4 "Unsupported Visual Studio version %VisualStudioVersion%"
GOTO error

:find_vs2026
CALL "%bin_dir%\echoc.exe" 2 "Find Visual Studio 2026."
SET DSN_TMP_BOOST_LIB=lib64-msvc-14.5
SET DSN_TMP_CMAKE_TARGET=NMake Makefiles
SET DSN_TMP_CMAKE_ARCH=
SET DSN_TMP_USE_CMAKE_BUILD=TRUE
IF NOT "%VSCMD_ARG_TGT_ARCH%"=="" IF /I NOT "%VSCMD_ARG_TGT_ARCH%"=="%DSN_TMP_BUILD_ARCH%" (
    CALL "%bin_dir%\echoc.exe" 4 "VS 2026 NMake builds use the current prompt architecture '%VSCMD_ARG_TGT_ARCH%'. Please use a matching VS prompt or set DSN_BUILD_ARCH=%VSCMD_ARG_TGT_ARCH%."
    GOTO error
)
GOTO start_build

:find_vs2022
CALL "%bin_dir%\echoc.exe" 2 "Find Visual Studio 2022."
SET DSN_TMP_BOOST_LIB=lib64-msvc-14.3
SET DSN_TMP_CMAKE_TARGET=Visual Studio 17 2022
SET DSN_TMP_CMAKE_ARCH=-A %DSN_TMP_BUILD_ARCH%
SET DSN_TMP_USE_CMAKE_BUILD=
GOTO start_build

:find_vs2019
CALL "%bin_dir%\echoc.exe" 2 "Find Visual Studio 2019."
IF "%DSN_TMP_BUILD_ARCH%"=="ARM64" GOTO unsupported_arm64_vs
SET DSN_TMP_BOOST_LIB=lib64-msvc-14.2
SET DSN_TMP_CMAKE_TARGET=Visual Studio 16 2019
SET DSN_TMP_CMAKE_ARCH=-A %DSN_TMP_BUILD_ARCH%
SET DSN_TMP_USE_CMAKE_BUILD=
GOTO start_build

:find_vs2017
CALL "%bin_dir%\echoc.exe" 2 "Find Visual Studio 2017."
IF "%DSN_TMP_BUILD_ARCH%"=="ARM64" GOTO unsupported_arm64_vs
SET DSN_TMP_BOOST_LIB=lib64-msvc-14.1
SET DSN_TMP_CMAKE_TARGET=Visual Studio 15 2017
SET DSN_TMP_CMAKE_ARCH=-A %DSN_TMP_BUILD_ARCH%
SET DSN_TMP_USE_CMAKE_BUILD=
GOTO start_build

:find_vs2015
CALL "%bin_dir%\echoc.exe" 2 "Find Visual Studio 2015."
IF "%DSN_TMP_BUILD_ARCH%"=="ARM64" GOTO unsupported_arm64_vs
SET DSN_TMP_BOOST_LIB=lib64-msvc-14.0
SET DSN_TMP_CMAKE_TARGET=Visual Studio 14 2015
SET DSN_TMP_CMAKE_ARCH=-A %DSN_TMP_BUILD_ARCH%
SET DSN_TMP_USE_CMAKE_BUILD=
GOTO start_build

:unsupported_arm64_vs
CALL "%bin_dir%\echoc.exe" 4 "ARM64 builds require Visual Studio 2022 or 2026."
GOTO error

:start_build
IF "%boost_dir%" EQU "" GOTO use_bundled_boost
IF EXIST "%boost_dir%\boost" (
    SET DSN_TMP_BOOST_INCLUDEDIR=%boost_dir%
    GOTO find_custom_boost_lib
)
IF EXIST "%boost_dir%\include\boost" (
    SET DSN_TMP_BOOST_INCLUDEDIR=%boost_dir%\include
    GOTO find_custom_boost_lib
)
CALL "%bin_dir%\echoc.exe" 4 "Custom Boost directory does not contain boost headers: %boost_dir%"
GOTO error

:find_custom_boost_lib
SET DSN_TMP_BOOST_LIBRARYDIR=
IF EXIST "%boost_dir%\%DSN_TMP_BOOST_LIB%" SET DSN_TMP_BOOST_LIBRARYDIR=%boost_dir%\%DSN_TMP_BOOST_LIB%
IF NOT DEFINED DSN_TMP_BOOST_LIBRARYDIR GOTO use_custom_header_only_boost

:use_custom_binary_boost
CALL "%bin_dir%\echoc.exe" 2 "Use custom Boost: %boost_dir%"
SET DSN_TMP_BOOST_CMAKE_ARGS=-DBoost_NO_BOOST_CMAKE=ON -DBOOST_ROOT="%boost_dir%" -DBOOST_INCLUDEDIR="%DSN_TMP_BOOST_INCLUDEDIR%" -DBoost_INCLUDE_DIR="%DSN_TMP_BOOST_INCLUDEDIR%" -DBOOST_LIBRARYDIR="%DSN_TMP_BOOST_LIBRARYDIR%" -DBoost_LIBRARY_DIR="%DSN_TMP_BOOST_LIBRARYDIR%" -DBoost_LIBRARY_DIRS="%DSN_TMP_BOOST_LIBRARYDIR%" -DBoost_NO_SYSTEM_PATHS=ON
GOTO boost_args_done

:use_custom_header_only_boost
CALL "%bin_dir%\echoc.exe" 2 "Use custom header-only Boost: %boost_dir%"
SET DSN_TMP_BOOST_CMAKE_ARGS=-DDSN_BOOST_HEADER_ONLY=ON -DBoost_NO_BOOST_CMAKE=ON -DBOOST_ROOT="%boost_dir%" -DBOOST_INCLUDEDIR="%DSN_TMP_BOOST_INCLUDEDIR%" -DBoost_INCLUDE_DIR="%DSN_TMP_BOOST_INCLUDEDIR%" -DBoost_NO_SYSTEM_PATHS=ON
GOTO boost_args_done

:use_bundled_boost
SET DSN_TMP_BOOST_CMAKE_ARGS=-DBOOST_INCLUDEDIR="%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%" -DBOOST_LIBRARYDIR="%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%\%DSN_TMP_BOOST_LIB%"

:boost_args_done
IF NOT EXIST "%build_dir%" mkdir "%build_dir%"
PUSHD "%build_dir%"

REM call cmake
echo CALL "%DSN_TMP_CMAKE_EXE%" "%cdir%" %buildall% %DSN_TMP_BUILD_CSHARP% %DSN_TMP_BUILD_PROTOBUF_CSHARP% %DSN_TMP_BUILD_THRIFT_CSHARP% -DCMAKE_INSTALL_PREFIX="%build_dir%\output" -DDSN_BUILD_DIR="%build_dir%" -DCMAKE_BUILD_TYPE="%build_type%" %DSN_TMP_BOOST_CMAKE_ARGS% -DDSN_GIT_SOURCE="github" %DSN_TMP_CMAKE_ARCH% -G "%DSN_TMP_CMAKE_TARGET%"
CALL "%DSN_TMP_CMAKE_EXE%" "%cdir%" %buildall% %DSN_TMP_BUILD_CSHARP% %DSN_TMP_BUILD_PROTOBUF_CSHARP% %DSN_TMP_BUILD_THRIFT_CSHARP% -DCMAKE_INSTALL_PREFIX="%build_dir%\output" -DDSN_BUILD_DIR="%build_dir%" -DCMAKE_BUILD_TYPE="%build_type%" %DSN_TMP_BOOST_CMAKE_ARGS% -DDSN_GIT_SOURCE="github" %DSN_TMP_CMAKE_ARCH% -G "%DSN_TMP_CMAKE_TARGET%"
IF ERRORLEVEL 1 (
    SET DSN_TMP_CMAKE_TARGET=
    SET DSN_TMP_CMAKE_ARCH=
    SET DSN_TMP_BOOST_LIB=
    SET DSN_TMP_BOOST_INCLUDEDIR=
    SET DSN_TMP_BOOST_LIBRARYDIR=
    SET DSN_TMP_BOOST_CMAKE_ARGS=
    SET DSN_TMP_CUSTOM_BOOST_DIR=
    SET DSN_TMP_USE_CMAKE_BUILD=
    SET DSN_TMP_BUILD_CSHARP=
    SET DSN_TMP_BUILD_PROTOBUF_CSHARP=
    SET DSN_TMP_BUILD_THRIFT_CSHARP=
    SET DSN_TMP_BUILD_ARCH=
    SET DSN_TMP_VCVARS_ARCH=
    CALL "%bin_dir%\echoc.exe" 4 "cmake error!"
    POPD
    GOTO error
)

REM clean temp environment variables
SET DSN_TMP_CMAKE_TARGET=
SET DSN_TMP_CMAKE_ARCH=
SET DSN_TMP_BOOST_LIB=
SET DSN_TMP_BOOST_INCLUDEDIR=
SET DSN_TMP_BOOST_LIBRARYDIR=
SET DSN_TMP_BOOST_CMAKE_ARGS=
SET DSN_TMP_CUSTOM_BOOST_DIR=
SET DSN_TMP_BUILD_ARCH=
SET DSN_TMP_VCVARS_ARCH=
SET DSN_TMP_BUILD_CSHARP=
SET DSN_TMP_BUILD_PROTOBUF_CSHARP=
SET DSN_TMP_BUILD_THRIFT_CSHARP=

IF "%DSN_TMP_USE_CMAKE_BUILD%"=="TRUE" (
    SET DSN_TMP_USE_CMAKE_BUILD=
    CALL "%DSN_TMP_CMAKE_EXE%" --build . --config "%build_type%" --target install
    IF ERRORLEVEL 1 (
        CALL "%bin_dir%\echoc.exe" 4 "cmake building error!"
        POPD
        GOTO error
    )
    POPD
    EXIT /B 0
)

FOR /F "delims=" %%i IN ('dir /b *.sln') DO set solution_name=%%i

msbuild %solution_name% /p:Configuration=%build_type% /m
IF ERRORLEVEL 1 (
    CALL "%bin_dir%\echoc.exe" 4 "msbuild building error!"
    POPD
    GOTO error
)

msbuild INSTALL.vcxproj /p:Configuration=%build_type% /m
IF ERRORLEVEL 1 (
    CALL "%bin_dir%\echoc.exe" 4 "msbuild installing error!"
    POPD
    GOTO error
)

POPD
EXIT /B 0

:error_usage
    SET DSN_TMP_EXIT_CODE=1
    SET DSN_TMP_USAGE_LEVEL=4
    GOTO usage
:error
    SET DSN_TMP_CUSTOM_BOOST_DIR=
    SET DSN_TMP_BOOST_INCLUDEDIR=
    SET DSN_TMP_BOOST_LIBRARYDIR=
    SET DSN_TMP_BOOST_CMAKE_ARGS=
    EXIT /B 1

:usage_exit
    SET DSN_TMP_EXIT_CODE=0
    SET DSN_TMP_USAGE_LEVEL=2
    GOTO usage

:usage
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% "Usage: run.cmd build [-t|--type Debug|Release|RelWithDebInfo|MinSizeRel] [-d|--build_dir builder] [-b|--boost_dir boost_dir] [--build_plugins] [--build_csharp] [--build_protobuf_csharp] [--build_thrift_csharp], optionally set DSN_BUILD_ARCH=x64|ARM64"
    EXIT /B %DSN_TMP_EXIT_CODE%
