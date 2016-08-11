SET bin_dir=%~dp0
SET TOP_DIR=%bin_dir%\..\..\
SET build_type=%1
SET build_dir=%~f2
SET install_dir=%~f3

IF NOT EXIST "%build_dir%\output" (
    CALL %bin_dir%\echoc.exe 4 "not build yet"
    GOTO error
)

IF "%install_dir%" EQU "" SET install_dir=%DSN_ROOT:/=\%
IF "%install_dir%" EQU "" (
    CALL %bin_dir%\echoc.exe 4 "install_dir not specified and DSN_ROOT not set"
    GOTO error
)

pushd %build_dir%

XCOPY /Y /S /D output\* %install_dir%\

IF EXIST "dsn.sln" COPY /Y bin\dsn.svchost\%build_type%\dsn.svchost.exe %install_dir%\bin\dsn.svchost.exe

popd
 
goto exit

:error
    CALL %bin_dir%\echoc.exe 4  "Usage: run.cmd install build_type(Debug|Release|RelWithDebInfo|MinSizeRel) build_dir [install_dir (DSN_ROOT by default)]"
    exit 1
    
:exit
    exit 0