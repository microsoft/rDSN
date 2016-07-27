
SET TOP_DIR=%1
SET build_type=%2
SET build_dir=%3

CALL  sbin\mock_install.cmd %TOP_DIR% %build_type% %build_dir%
CALL  sbin\test.cmd %cd% %TOP_DIR%\ext\cmake-3.2.2\bin
IF ERRORLEVEL 1 ECHO IDL test failed && EXIT /B 1
@CALL sbin\clear.cmd
EXIT /B 0

