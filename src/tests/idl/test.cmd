@ECHO OFF

SET TOP_DIR=%1
SET build_type=%2
SET build_dir=%3

ECHO DSN_ROOT=%DSN_ROOT%
CALL %build_type%\dsn.idl.tests.exe
