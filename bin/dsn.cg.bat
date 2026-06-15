@ECHO OFF
SET "CODEGEN_ROOT=%~dp0"
CALL "%CODEGEN_ROOT%Windows\php.exe" -f "%CODEGEN_ROOT%dsn.generate_code.php" %*
:EOF
