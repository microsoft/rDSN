@ECHO OFF

SET bin_dir=%~dp0
SET TOP_DIR=%bin_dir%\..\..\
PUSHD "%TOP_DIR%"
SET TOP_DIR=%CD%
POPD
SET build_type=%1
SET build_dir=%~f2

IF "%build_type%" EQU "" SET build_type=Debug

IF "%build_dir%" EQU "" (
    CALL "%bin_dir%\echoc.exe" 4 please specify build_dir
    GOTO error
)

IF NOT EXIST "%build_dir%" (
    CALL "%bin_dir%\echoc.exe" 4 %build_dir% does not exist
    GOTO error
)

CALL "%bin_dir%\echoc.exe" 2 run the tests here ...

:: set the path of built binaries
SET DSN_TMP_BUILD_DIR_IN_PATH=
@FOR %%P in ("%Path:;=";"%") DO @IF /I %%P=="%build_dir%\bin\%build_type%" SET DSN_TMP_BUILD_DIR_IN_PATH=true
:: SET DSN_TMP_OLD_PATH=%Path%
IF NOT DEFINED DSN_TMP_BUILD_DIR_IN_PATH SET Path=%build_dir%\bin\%build_type%;%build_dir%\lib;%Path%
SET DSN_TMP_BUILD_DIR_IN_PATH=

:: run dll-embedded unit tests
SET DSN_TEST_HOST=%DSN_ROOT:/=\%\bin\dsn.svchost.exe
IF EXIST "%build_dir%\bin\dsn.svchost\%build_type%\dsn.svchost.exe"  SET DSN_TEST_HOST=%build_dir%\bin\dsn.svchost\%build_type%\dsn.svchost.exe

FOR /D %%A IN ("%build_dir%\test\*") DO (
    IF EXIST "%%A\gtests" (
        PUSHD "%%A"
        FOR /F "usebackq" %%I IN ("%%A\gtests") DO (
            IF EXIST "%%A" (
                ECHO =========== %DSN_TEST_HOST% %%I ======================
                CALL "%DSN_TEST_HOST%" "%%I"
                IF ERRORLEVEL 1 POPD && ECHO test "%%I" failed && goto error
            )
        )
        POPD
    )
)

:: run e-e tests
FOR /D %%A IN ("%build_dir%\bin\*") DO (
    IF EXIST "%%A\test.cmd" (
        PUSHD "%%A"
        ECHO ================= %%A\test.cmd ================================
        CALL test.cmd "%TOP_DIR%" %build_type% "%build_dir%"
        IF ERRORLEVEL 1 POPD && ECHO test "%%A" failed && goto error
        POPD
    )
)

goto exit

:error
    CALL "%bin_dir%\echoc.exe" 4  "Usage: run.cmd test build_type(Debug|Release|RelWithDebInfo|MinSizeRel) build_dir"
    exit /B 1

:exit
    exit /B 0
