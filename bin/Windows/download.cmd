@ECHO OFF
SETLOCAL

SET "url=%~1"
SET "output_dir=%~2"
SET "output_name=%~3"

IF "%url%" EQU "" GOTO usage
IF "%output_dir%" EQU "" SET "output_dir=%CD%"

IF "%output_name%" EQU "" (
    FOR %%F IN ("%url%") DO SET "output_name=%%~nxF"
)

IF "%output_name%" EQU "" GOTO usage
IF NOT EXIST "%output_dir%" MKDIR "%output_dir%"
IF ERRORLEVEL 1 EXIT /B 1

WHERE curl.exe >NUL 2>NUL
IF ERRORLEVEL 1 (
    ECHO curl.exe is required to download "%url%"
    EXIT /B 1
)

curl.exe --fail --location --retry 3 --output "%output_dir%\%output_name%" "%url%"
IF ERRORLEVEL 1 (
    IF EXIST "%output_dir%\%output_name%" DEL /F /Q "%output_dir%\%output_name%" >NUL 2>NUL
    EXIT /B 1
)

EXIT /B 0

:usage
    ECHO Usage: download.cmd URL OUTPUT_DIR [OUTPUT_NAME]
    EXIT /B 1
