@ECHO OFF

SET bin_dir=%~dp0
SET TOP_DIR=%bin_dir%\..\..\
PUSHD "%TOP_DIR%"
SET TOP_DIR=%CD%
POPD
SET build_type=%1
SET build_dir=%~f2
SET buildall=-DBUILD_PLUGINS=FALSE
SET cdir=%CD%

IF "%3" EQU "build_plugins" (
    SET buildall=-DBUILD_PLUGINS=TRUE
    pushd "%TOP_DIR%\src\plugins_ext"
    git submodule init
    git submodule update
    popd 
)

IF "%build_type%" EQU "" SET build_type=Debug

IF "%build_dir%" EQU "" (
    SET build_dir=%TOP_DIR%\builder
)

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

CALL "%bin_dir%\pre-require.cmd"
IF ERRORLEVEL 1 (
    GOTO error
)

:: detect VS
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
IF NOT EXIST "%build_dir%" mkdir "%build_dir%"
PUSHD "%build_dir%"

:: call cmake
echo CALL "%DSN_TMP_CMAKE_EXE%" "%cdir%" %buildall% -DCMAKE_INSTALL_PREFIX="%build_dir%\output" -DDSN_BUILD_DIR="%build_dir%" -DCMAKE_BUILD_TYPE="%build_type%" -DBOOST_INCLUDEDIR="%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%" -DBOOST_LIBRARYDIR="%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%\%DSN_TMP_BOOST_LIB%" -DDSN_GIT_SOURCE="github" %DSN_TMP_CMAKE_ARCH% -G "%DSN_TMP_CMAKE_TARGET%"
CALL "%DSN_TMP_CMAKE_EXE%" "%cdir%" %buildall% -DCMAKE_INSTALL_PREFIX="%build_dir%\output" -DDSN_BUILD_DIR="%build_dir%" -DCMAKE_BUILD_TYPE="%build_type%" -DBOOST_INCLUDEDIR="%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%" -DBOOST_LIBRARYDIR="%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%\%DSN_TMP_BOOST_LIB%" -DDSN_GIT_SOURCE="github" %DSN_TMP_CMAKE_ARCH% -G "%DSN_TMP_CMAKE_TARGET%"
IF ERRORLEVEL 1 (
    SET DSN_TMP_CMAKE_TARGET=
    SET DSN_TMP_CMAKE_ARCH=
    SET DSN_TMP_BOOST_LIB=
    SET DSN_TMP_USE_CMAKE_BUILD=
    SET DSN_TMP_BUILD_ARCH=
    SET DSN_TMP_VCVARS_ARCH=
    CALL "%bin_dir%\echoc.exe" 4 "cmake error!"
    POPD
    GOTO error
)

:: clean temp environment variables
SET DSN_TMP_CMAKE_TARGET=
SET DSN_TMP_CMAKE_ARCH=
SET DSN_TMP_BOOST_LIB=
SET DSN_TMP_BUILD_ARCH=
SET DSN_TMP_VCVARS_ARCH=

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
    CALL "%bin_dir%\echoc.exe" 4 "Usage: run.cmd build build_type(Debug|Release|RelWithDebInfo|MinSizeRel) [build_dir=builder] [build_plugins], optionally set DSN_BUILD_ARCH=x64|ARM64"
:error
    EXIT /B 1
