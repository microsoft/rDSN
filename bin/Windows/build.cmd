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

CALL "%bin_dir%\pre-require.cmd"
IF ERRORLEVEL 1 (
    GOTO exit
)

IF "%build_type%" EQU "" SET build_type=Debug

IF "%build_dir%" EQU "" (
    CALL "%bin_dir%\echoc.exe" 4 please specify build_dir
    GOTO error
)

:: detect VS
SET DSN_TMP_CMAKE_VERSION=3.9.0
SET DSN_TMP_BOOST_VERSION=1_64_0
IF "%VisualStudioVersion%"=="15.0" GOTO find_vs2017
IF "%VisualStudioVersion%"=="14.0" GOTO find_vs2015
SET DSN_TMP_VS_INSTALL_DIR=
FOR /f "usebackq tokens=1* delims=: " %%i in (`"%bin_dir%\vswhere.exe" -latest`) DO (
  IF /i "%%i"=="installationPath" set DSN_TMP_VS_INSTALL_DIR=%%j
)
IF DEFINED DSN_TMP_VS_INSTALL_DIR (
    IF EXIST "%DSN_TMP_VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat" (
        CALL "%DSN_TMP_VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat"
        SET DSN_TMP_VS_INSTALL_DIR=
        GOTO find_vs2017
    )
    SET DSN_TMP_VS_INSTALL_DIR=
)
IF NOT "%VS140COMNTOOLS%"=="" (
    IF EXIST "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" (
        CALL "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" amd64
        GOTO find_vs2015
    )
)

CALL "%bin_dir%\echoc.exe" 4 "Visusal Studio 2015 or 2017 is not found, please run 'x64 Native Tools Command Prompt' and try later"
GOTO exit

:find_vs2017
CALL "%bin_dir%\echoc.exe" 2 "Find Visusal Studio 2017."
SET DSN_TMP_BOOST_LIB=lib64-msvc-14.1
SET DSN_TMP_CMAKE_TARGET=Visual Studio 15 2017 Win64
GOTO start_build

:find_vs2015
CALL "%bin_dir%\echoc.exe" 2 "Find Visusal Studio 2015."
SET DSN_TMP_BOOST_LIB=lib64-msvc-14.0
SET DSN_TMP_CMAKE_TARGET=Visual Studio 14 2015 Win64
GOTO start_build

:start_build
IF NOT EXIST "%build_dir%" mkdir "%build_dir%"
pushd "%build_dir%"

:: call cmake
echo CALL "%TOP_DIR%\ext\cmake-%DSN_TMP_CMAKE_VERSION%\bin\cmake.exe" "%cdir%" %buildall% -DCMAKE_INSTALL_PREFIX="%build_dir%\output" -DCMAKE_BUILD_TYPE="%build_type%" -DBOOST_INCLUDEDIR="%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%" -DBOOST_LIBRARYDIR="%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%\%DSN_TMP_BOOST_LIB%" -DDSN_GIT_SOURCE="github" -G "%DSN_TMP_CMAKE_TARGET%"
CALL "%TOP_DIR%\ext\cmake-%DSN_TMP_CMAKE_VERSION%\bin\cmake.exe" "%cdir%" %buildall% -DCMAKE_INSTALL_PREFIX="%build_dir%\output" -DCMAKE_BUILD_TYPE="%build_type%" -DBOOST_INCLUDEDIR="%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%" -DBOOST_LIBRARYDIR="%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%\%DSN_TMP_BOOST_LIB%" -DDSN_GIT_SOURCE="github" -G "%DSN_TMP_CMAKE_TARGET%"

:: clean temp environment variables
SET DSN_TMP_CMAKE_VERSION=
SET DSN_TMP_BOOST_VERSION=
SET DSN_TMP_BOOST_LIB=
SET DSN_TMP_CMAKE_TARGET=

FOR /F "delims=" %%i IN ('dir /b *.sln') DO set solution_name=%%i

msbuild %solution_name% /p:Configuration=%build_type% /m

msbuild INSTALL.vcxproj /p:Configuration=%build_type% /m

popd
goto exit

:error
    CALL "%bin_dir%\echoc.exe" 4 "Usage: run.cmd build build_type(Debug|Release|RelWithDebInfo|MinSizeRel) build_dir [build_plugins]"
    exit /B 1

:exit
    exit /B 0
