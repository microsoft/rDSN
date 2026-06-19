@ECHO OFF

SET bin_dir=%~dp0
SET TOP_DIR=%bin_dir%\..\..\
PUSHD "%TOP_DIR%"
SET TOP_DIR=%CD%
POPD
SET build_type=Debug
SET build_dir=
SET install_dir=

:parse_args
IF "%~1" EQU "" GOTO args_done
IF /I "%~1" EQU "-h" GOTO usage_exit
IF /I "%~1" EQU "--help" GOTO usage_exit
IF /I "%~1" EQU "-t" (
    IF "%~2" EQU "" GOTO error
    SET build_type=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--type" (
    IF "%~2" EQU "" GOTO error
    SET build_type=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "-b" (
    IF "%~2" EQU "" GOTO error
    FOR %%I IN ("%~2") DO SET build_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--build-dir" (
    IF "%~2" EQU "" GOTO error
    FOR %%I IN ("%~2") DO SET build_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--build_dir" (
    IF "%~2" EQU "" GOTO error
    FOR %%I IN ("%~2") DO SET build_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "-d" (
    IF "%~2" EQU "" GOTO error
    FOR %%I IN ("%~2") DO SET install_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--install-dir" (
    IF "%~2" EQU "" GOTO error
    FOR %%I IN ("%~2") DO SET install_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--install_dir" (
    IF "%~2" EQU "" GOTO error
    FOR %%I IN ("%~2") DO SET install_dir=%%~fI
    SHIFT
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
IF NOT DEFINED DSN_TMP_INSTALL_DIR_ARG (
    FOR %%I IN ("%~1") DO SET install_dir=%%~fI
    SET DSN_TMP_INSTALL_DIR_ARG=TRUE
    SHIFT
    GOTO parse_args
)
GOTO error

:args_done
IF /I "%build_type%" EQU "debug" SET build_type=Debug
IF /I "%build_type%" EQU "release" SET build_type=Release
IF /I "%build_type%" EQU "relwithdebinfo" SET build_type=RelWithDebInfo
IF /I "%build_type%" EQU "minsizerel" SET build_type=MinSizeRel
IF NOT "%build_type%" EQU "Debug" IF NOT "%build_type%" EQU "Release" IF NOT "%build_type%" EQU "RelWithDebInfo" IF NOT "%build_type%" EQU "MinSizeRel" GOTO error
SET DSN_TMP_BUILD_TYPE_ARG=
SET DSN_TMP_BUILD_DIR_ARG=
SET DSN_TMP_INSTALL_DIR_ARG=

IF "%build_dir%" EQU "" SET build_dir=%TOP_DIR%\builder

IF NOT EXIST "%build_dir%\output" (
    CALL "%bin_dir%\echoc.exe" 4 "not build yet"
    GOTO error
)

IF "%install_dir%" EQU "" SET install_dir=%DSN_ROOT:/=\%
IF "%install_dir%" EQU "" (
    CALL "%bin_dir%\echoc.exe" 4 "install_dir not specified and DSN_ROOT not set"
    GOTO error
)

PUSHD "%build_dir%"

XCOPY /Y /S /D /Q output\* "%install_dir%\"

IF EXIST "dsn.sln" COPY /Y "bin\dsn.svchost\%build_type%\dsn.svchost.exe" "%install_dir%\bin\dsn.svchost.exe"

POPD
 
goto exit

:error
    SET DSN_TMP_EXIT_CODE=1
    SET DSN_TMP_USAGE_LEVEL=4
    GOTO usage

:usage_exit
    SET DSN_TMP_EXIT_CODE=0
    SET DSN_TMP_USAGE_LEVEL=2
    GOTO usage

:usage
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL%  "Usage: run.cmd install [-t|--type Debug|Release|RelWithDebInfo|MinSizeRel] [-b|--build-dir builder] [-d|--install_dir install_dir]"
    exit /B %DSN_TMP_EXIT_CODE%
    
:exit
    exit /B 0
