@ECHO OFF

SET TOP_DIR=%1
SET build_type=%2
SET "build_dir=%~3"

IF NOT "%build_dir%" EQU "" IF NOT DEFINED DSN_TEST_TMP_DIR SET DSN_TEST_TMP_DIR=%build_dir%\test_tmp
IF DEFINED DSN_TEST_TMP_DIR IF NOT EXIST "%DSN_TEST_TMP_DIR%" (
    MKDIR "%DSN_TEST_TMP_DIR%"
    IF ERRORLEVEL 1 (
        ECHO Failed to create %DSN_TEST_TMP_DIR%. Close previous tests or terminals that are using this directory, then retry.
        EXIT /B 1
    )
)

ECHO DSN_ROOT=%DSN_ROOT%
CALL "%build_type%\dsn.idl.tests.exe"
EXIT /B %ERRORLEVEL%
