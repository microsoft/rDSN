@ECHO OFF

SET bin_dir=%~dp0
SET TOP_DIR=%bin_dir%\..\..\
PUSHD "%TOP_DIR%"
SET TOP_DIR=%CD%
POPD
SET build_type=%1
SET build_dir=%~f2
SET install_dir=%~f3

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
    CALL %bin_dir%\echoc.exe 4  "Usage: run.cmd install build_type(Debug|Release|RelWithDebInfo|MinSizeRel) build_dir [install_dir (DSN_ROOT by default)]"
    exit /B 1
    
:exit
    exit /B 0
