@ECHO OFF

SET bin_dir=%~dp0
SET TOP_DIR=%bin_dir%\..\..\
PUSHD "%TOP_DIR%"
SET TOP_DIR=%CD%
POPD
SET build_type=Debug
SET build_dir=

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
SET DSN_TMP_BUILD_TYPE_ARG=
SET DSN_TMP_BUILD_DIR_ARG=

IF "%build_dir%" EQU "" (
    SET build_dir=%TOP_DIR%\builder
)

IF NOT EXIST "%build_dir%" (
    CALL "%bin_dir%\echoc.exe" 4 %build_dir% does not exist
    GOTO error
)

SET REPORT_DIR=%build_dir%\test_reports
SET DSN_TEST_TMP_DIR=%build_dir%\test_tmp
IF NOT EXIST "%REPORT_DIR%" MKDIR "%REPORT_DIR%"
IF EXIST "%DSN_TEST_TMP_DIR%" (
    RMDIR /S /Q "%DSN_TEST_TMP_DIR%"
    IF ERRORLEVEL 1 (
        CALL "%bin_dir%\echoc.exe" 4 "Failed to clean %DSN_TEST_TMP_DIR%. Close previous tests or terminals that are using this directory, then retry."
        GOTO error
    )
)
MKDIR "%DSN_TEST_TMP_DIR%"
IF ERRORLEVEL 1 GOTO error

CALL "%bin_dir%\echoc.exe" 2 run the tests here ...

REM set the path of built binaries
SET DSN_TMP_BUILD_DIR_IN_PATH=
@FOR %%P in ("%Path:;=";"%") DO @IF /I %%P=="%build_dir%\bin\%build_type%" SET DSN_TMP_BUILD_DIR_IN_PATH=true
REM SET DSN_TMP_OLD_PATH=%Path%
IF NOT DEFINED DSN_TMP_BUILD_DIR_IN_PATH SET Path=%build_dir%\bin\%build_type%;%build_dir%\lib;%Path%
SET DSN_TMP_BUILD_DIR_IN_PATH=

REM run dll-embedded unit tests
SET DSN_TEST_HOST=%DSN_ROOT:/=\%\bin\dsn.svchost.exe
IF EXIST "%build_dir%\bin\dsn.svchost\%build_type%\dsn.svchost.exe"  SET DSN_TEST_HOST=%build_dir%\bin\dsn.svchost\%build_type%\dsn.svchost.exe

FOR /D %%A IN ("%build_dir%\test\*") DO (
    IF EXIST "%%A\gtests" (
        PUSHD "%%A"
        FOR /F "usebackq eol=# delims=" %%I IN ("%%A\gtests") DO (
            IF EXIST "%%A" (
                ECHO =========== %DSN_TEST_HOST% %%I ======================
                IF EXIST data\ RMDIR /S /Q data
                IF EXIST data DEL /F /Q data
                IF EXIST core\ RMDIR /S /Q core
                IF EXIST core DEL /F /Q core
                CALL "%DSN_TEST_HOST%" "%%I"
                IF ERRORLEVEL 1 POPD && ECHO test "%%I" failed && goto error
            )
        )
        POPD
    )
)

REM run e-e tests
FOR /D %%A IN ("%build_dir%\bin\*") DO (
    ECHO %%A\
    IF EXIST "%%A\test.cmd" (
        PUSHD "%%A"
        ECHO ============ run test.cmd in %%A\ ============
        IF EXIST data\ RMDIR /S /Q data
        IF EXIST data DEL /F /Q data
        IF EXIST core\ RMDIR /S /Q core
        IF EXIST core DEL /F /Q core
        CALL test.cmd "%TOP_DIR%" %build_type% "%build_dir%"
        IF ERRORLEVEL 1 POPD && ECHO test "%%A" failed && goto error
        POPD
    )
)

GOTO exit

:error_usage
    SET DSN_TMP_EXIT_CODE=1
    SET DSN_TMP_USAGE_LEVEL=4
    GOTO usage

:error
    SET DSN_TMP_EXIT_CODE=1
    SET DSN_TMP_USAGE_LEVEL=4
    GOTO usage

:usage_exit
    SET DSN_TMP_EXIT_CODE=0
    SET DSN_TMP_USAGE_LEVEL=2
    GOTO usage

:usage
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL%  "Usage: run.cmd test [-t|--type Debug|Release|RelWithDebInfo|MinSizeRel] [-d|--build_dir builder]"
    exit /B %DSN_TMP_EXIT_CODE%

:exit
    ECHO Test succeed
    exit /B 0
