@ECHO OFF

SET cmd=%1
SET bin_dir=%~dp0
SET TOP_DIR=%bin_dir%\..\..\
PUSHD "%TOP_DIR%"
SET TOP_DIR=%CD%
POPD
SHIFT
SET app_name=
SET build_dir=
SET build_type=Debug
SET webstudio_url=

:parse_args
IF "%~1" EQU "" GOTO args_done
IF /I "%~1" EQU "-h" GOTO usage_exit
IF /I "%~1" EQU "--help" GOTO usage_exit
IF /I "%~1" EQU "-d" (
    IF "%~2" EQU "" GOTO error_usage
    SET app_name=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--deploy-name" (
    IF "%~2" EQU "" GOTO error_usage
    SET app_name=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--deploy_name" (
    IF "%~2" EQU "" GOTO error_usage
    SET app_name=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "-b" (
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
IF /I "%~1" EQU "--build_dir" (
    IF "%~2" EQU "" GOTO error_usage
    FOR %%I IN ("%~2") DO SET build_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
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
IF /I "%~1" EQU "-w" (
    IF "%~2" EQU "" GOTO error_usage
    SET webstudio_url=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--webstudio-package-url" (
    IF "%~2" EQU "" GOTO error_usage
    SET webstudio_url=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--webstudio_package_url" (
    IF "%~2" EQU "" GOTO error_usage
    SET webstudio_url=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF NOT DEFINED DSN_TMP_APP_NAME_ARG (
    SET app_name=%~1
    SET DSN_TMP_APP_NAME_ARG=TRUE
    SHIFT
    GOTO parse_args
)
IF NOT DEFINED DSN_TMP_BUILD_DIR_ARG (
    FOR %%I IN ("%~1") DO SET build_dir=%%~fI
    SET DSN_TMP_BUILD_DIR_ARG=TRUE
    SHIFT
    GOTO parse_args
)
IF NOT DEFINED DSN_TMP_BUILD_TYPE_ARG (
    SET build_type=%~1
    SET DSN_TMP_BUILD_TYPE_ARG=TRUE
    SHIFT
    GOTO parse_args
)
IF NOT DEFINED DSN_TMP_WEBSTUDIO_URL_ARG (
    SET webstudio_url=%~1
    SET DSN_TMP_WEBSTUDIO_URL_ARG=TRUE
    SHIFT
    GOTO parse_args
)
GOTO error_usage

:args_done
IF /I "%build_type%" EQU "debug" SET build_type=Debug
IF /I "%build_type%" EQU "release" SET build_type=Release
IF /I "%build_type%" EQU "relwithdebinfo" SET build_type=RelWithDebInfo
IF /I "%build_type%" EQU "minsizerel" SET build_type=MinSizeRel
SET DSN_TMP_APP_NAME_ARG=
SET DSN_TMP_BUILD_DIR_ARG=
SET DSN_TMP_BUILD_TYPE_ARG=
SET DSN_TMP_WEBSTUDIO_URL_ARG=

IF "%build_dir%" EQU "" SET build_dir=%TOP_DIR%\builder

IF "%app_name%" EQU "" (
    CALL "%bin_dir%\echoc.exe" 4 please specify app_name
    GOTO error
)

IF NOT EXIST "%bin_dir%\publish.%app_name%.cmd" (
    CALL "%bin_dir%\echoc.exe" 4 please create '%bin_dir%\publish.%app_name%.cmd' before publish
    GOTO error
)

IF NOT EXIST "%build_dir%" (
    CALL "%bin_dir%\echoc.exe" 4 build dir '%build_dir%' is not specified or does not exist
    GOTO error
)

SET bt_valid=0
IF "%build_type%" EQU "Debug" SET bt_valid=1
IF "%build_type%" EQU "Release"  SET bt_valid=1
IF "%build_type%" EQU "RelWithDebInfo" SET bt_valid=1
IF "%build_type%" EQU "MinSizeRel" SET bt_valid=1

IF "%bt_valid%" EQU "0" (
    CALL "%bin_dir%\echoc.exe" 4 invalid build_type '%build_type%'
    GOTO error
)
REM COPY /Y "%build_dir%\bin\%build_type%\dsn.core.pdb" .\skv\%app%
IF NOT EXIST ".\skv" mkdir ".\skv"
COPY /Y "%build_dir%\bin\%build_type%\dsn.core.pdb" ".\skv"
CALL "%bin_dir%\publish.%app_name%.cmd" %cmd% "%build_dir%" %build_type% %webstudio_url%
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
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL%  "Usage: run.cmd publish|republish -d|--deploy-name app_name [-b|--build-dir builder] [-t|--type Debug|Release|RelWithDebInfo|MinSizeRel] [-w|--webstudio-package-url url]"
    exit /B %DSN_TMP_EXIT_CODE%

:exit
    exit /B 0
