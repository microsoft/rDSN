@ECHO OFF

SET TOP_DIR=%~dp0..
PUSHD "%TOP_DIR%"
SET TOP_DIR=%CD%
POPD

SET bin_dir=%TOP_DIR%\bin\Windows
SET old_dsn_root=%DSN_ROOT%
IF "%1" EQU "" GOTO usage
IF "%1" NEQ "setup-env" IF "%DSN_ROOT%" NEQ "" GOTO main
IF "%DSN_AUTO_TEST%" NEQ "" (
    SET DSN_ROOT=%TOP_DIR%
    GOTO install_env
)

SET /p DSN_ROOT=Please enter your DSN_ROOT (e.g., "%TOP_DIR%\install"):
IF "%DSN_ROOT%" EQU "" SET DSN_ROOT=%TOP_DIR%\install
@mkdir "%DSN_ROOT%"
IF "%DSN_ROOT%" NEQ "" IF exist "%DSN_ROOT%" GOTO install_env
CALL "%bin_dir%\echoc.exe" 4 "%DSN_ROOT%" does not exist
exit /B 1

:usage
    IF "%DSN_TMP_USAGE_LEVEL%" EQU "" SET DSN_TMP_USAGE_LEVEL=4
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% "Usage: run.cmd setup-env|pre-require|build|install|test|publish|republish|deploy|start|stop|cleanup|scds(stop-cleanup-deploy-start)|start_zk|stop_zk|onecluster"
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% "       run.cmd build [-t|--type Debug|Release|RelWithDebInfo|MinSizeRel] [-d|--build_dir builder] [-b|--boost_dir boost_dir] [--build_plugins] [--build_csharp] [--build_protobuf_csharp] [--build_thrift_csharp]"
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% "       run.cmd test [-t|--type Debug|Release|RelWithDebInfo|MinSizeRel] [-d|--build_dir builder]"
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% "       run.cmd install [-t|--type Debug|Release|RelWithDebInfo|MinSizeRel] [-b|--build-dir builder] [-d|--install_dir install_dir]"
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% "       run.cmd publish|republish -d|--deploy-name app_name [-b|--build-dir builder] [-t|--type Debug|Release|RelWithDebInfo|MinSizeRel]"
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% "       run.cmd deploy|start|stop|cleanup|quick-cleanup|scds -s|--source-dir source-dir -t|--target-dir target-dir"
    GOTO:EOF

:install_env
setlocal enabledelayedexpansion
SET old_path_appendix=;%old_dsn_root:/=\%\bin;%old_dsn_root:/=\%\lib;
SET new_path_appendix=;%DSN_ROOT:/=\%\bin;%DSN_ROOT:/=\%\lib;
SET lpath=%PATH%
SET lpath=%lpath:!old_path_appendix!=%
endlocal & SET PATH=%lpath%%new_path_appendix%

REM FIXME: It will mix user and system Path
REM SETX PATH "%PATH%"
REM CALL reg add HKCU\Environment /f /v PATH /d "%PATH%" 1>nul
REM CALL "%bin_dir%\flushenv.exe"

REM SET DSN_ROOT=%DSN_ROOT:\=/%
SET DSN_ROOT=%DSN_ROOT:/=\%
CALL reg add HKCU\Environment /f /v DSN_ROOT /d "%DSN_ROOT%" 1>nul
FOR /F "skip=2 tokens=3*" %%a IN ('reg query HKCU\Environment /v PATH') DO IF [%%b]==[] ( SETX Path "%%~a;%%DSN_ROOT%%\bin;%%DSN_ROOT%%\lib;" ) ELSE ( SETX Path "%%~a %%~b;%%DSN_ROOT%%\bin;%%DSN_ROOT%%\lib;" )
CALL "%bin_dir%\flushenv.exe"

CALL "%bin_dir%\echoc.exe" 2 DSN_ROOT ("%DSN_ROOT%") is setup, and rDSN SDK will be installed there.
CALL "%bin_dir%\echoc.exe" 2 DSN_ROOT\lib and DSN_ROOT\bin are added to PATH env.

:main

SET DSN_TMP_CMAKE_VERSION=3.22.6
REM SET DSN_TMP_BOOST_VERSION=1_64_0
SET DSN_TMP_BOOST_VERSION=1_84_0

CALL :%1 %1 %2 %3 %4 %5 %6 %7 %8 %9

IF ERRORLEVEL 1 (
    SET DSN_TMP_CMAKE_VERSION=
    SET DSN_TMP_CMAKE_EXE=
    SET DSN_TMP_BOOST_VERSION=

    CALL "%bin_dir%\echoc.exe" 4 command '%1' fails or is unknown.
    CALL :usage
    exit /B 1
)

SET DSN_TMP_CMAKE_VERSION=
SET DSN_TMP_CMAKE_EXE=
SET DSN_TMP_BOOST_VERSION=

exit /B 0

:pre-require
:build
:install
:test
:start_zk
:stop_zk
:onecluster
    CALL "%bin_dir%\%1.cmd" %2 %3 %4 %5 %6 %7 %8 %9
    GOTO:EOF

:publish
:republish
    CALL "%bin_dir%\publish.cmd" %1 %2 %3 %4 %5 %6 %7 %8 %9
    GOTO:EOF
    
:setup-env
    GOTO:EOF
    
:deploy
:start
:stop
:cleanup
:quick-cleanup
:scds
    CALL "%bin_dir%\deploy.cmd" %1 %2 %3 %4 %5 %6 %7 %8 %9
    GOTO:EOF
    
:exit
