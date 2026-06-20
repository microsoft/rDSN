SET bin_dir=%~dp0
SET cmd=%1
SHIFT

:parse_args
IF "%~1" EQU "" GOTO args_done
IF /I "%~1" EQU "-h" GOTO usage_exit
IF /I "%~1" EQU "--help" GOTO usage_exit
IF /I "%~1" EQU "-s" (
    IF "%~2" EQU "" GOTO error_usage
    FOR %%I IN ("%~2") DO SET src_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--source-dir" (
    IF "%~2" EQU "" GOTO error_usage
    FOR %%I IN ("%~2") DO SET src_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--source_dir" (
    IF "%~2" EQU "" GOTO error_usage
    FOR %%I IN ("%~2") DO SET src_dir=%%~fI
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "-t" (
    IF "%~2" EQU "" GOTO error_usage
    SET t_dir=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--target-dir" (
    IF "%~2" EQU "" GOTO error_usage
    SET t_dir=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF /I "%~1" EQU "--target_dir" (
    IF "%~2" EQU "" GOTO error_usage
    SET t_dir=%~2
    SHIFT
    SHIFT
    GOTO parse_args
)
IF NOT DEFINED DSN_TMP_SOURCE_DIR_ARG (
    FOR %%I IN ("%~1") DO SET src_dir=%%~fI
    SET DSN_TMP_SOURCE_DIR_ARG=TRUE
    SHIFT
    GOTO parse_args
)
IF NOT DEFINED DSN_TMP_TARGET_DIR_ARG (
    SET t_dir=%~1
    SET DSN_TMP_TARGET_DIR_ARG=TRUE
    SHIFT
    GOTO parse_args
)
GOTO error_usage

:args_done
SET DSN_TMP_SOURCE_DIR_ARG=
SET DSN_TMP_TARGET_DIR_ARG=

IF "%src_dir%" EQU "" (
    CALL "%bin_dir%\echoc.exe" 4 source directory not specified
    SET DSN_TMP_EXIT_CODE=1
    SET DSN_TMP_USAGE_LEVEL=4
    GOTO usage
)

IF NOT EXIST "%src_dir%" (
    CALL "%bin_dir%\echoc.exe" 4 cannot find source directory '%src_dir%'
    SET DSN_TMP_EXIT_CODE=1
    SET DSN_TMP_USAGE_LEVEL=4
    GOTO usage
)

IF "%t_dir%" EQU "" (
    CALL "%bin_dir%\echoc.exe" 4 destination dir not specified
    SET DSN_TMP_EXIT_CODE=1
    SET DSN_TMP_USAGE_LEVEL=4
    GOTO usage
)

IF EXIST "%src_dir%\apps.txt" (
    FOR /F "usebackq" "tokens=*" %%A IN ("%src_dir%\apps.txt") DO CALL "%bin_dir%\deploy.cmd" %cmd% --source-dir "%src_dir%\%%A" --target-dir "%t_dir%"
    GOTO exit
)

FOR %%I IN ("%src_dir%") DO SET deploy_name=%%~nI
SET ldst_dir=%t_dir%\%deploy_name%

SET rdst_dir=%ldst_dir::=$%
SET machine_list=%src_dir%\machines.txt

IF NOT EXIST "%machine_list%" (
    CALL "%bin_dir%\echoc.exe" 4 cannot find machine_list file '%machine_list%'
    SET DSN_TMP_EXIT_CODE=1
    SET DSN_TMP_USAGE_LEVEL=4
    GOTO usage
)

IF NOT EXIST "%src_dir%\start.cmd" (
    CALL "%bin_dir%\echoc.exe" 4 please create start.cmd in source directory '%src_dir%'
    GOTO exit
)

REM SET cmd=%1
REM SET src_dir=%2
REM SET ldst_dir=%3
REM SET deploy_name=%4
REM SET rdst_dir=%ldst_dir::=$%
REM SET machine=%5
FOR /F %%i IN (%machine_list%) DO ECHO %cmd% %%i %src_dir% %ldst_dir% %deploy_name% ... && start "%cmd% %deploy_name% @ %%i" "%bin_dir%\deploy.one.cmd" %cmd% "%src_dir%" "%ldst_dir%" %deploy_name% %%i

IF ERRORLEVEL 1 (
    CALL "%bin_dir%\echoc.exe" 4 unknow command '%cmd%'
    SET DSN_TMP_EXIT_CODE=1
    SET DSN_TMP_USAGE_LEVEL=4
    GOTO usage
)

GOTO exit

:error_usage
    SET DSN_TMP_EXIT_CODE=1
    SET DSN_TMP_USAGE_LEVEL=4
    GOTO usage

:usage_exit
    SET DSN_TMP_EXIT_CODE=0
    SET DSN_TMP_USAGE_LEVEL=2
    GOTO usage

:usage
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% "run.cmd deploy|start|stop|cleanup|quick-cleanup|scds(stop-cleanup-deploy-start) -s|--source-dir source-dir -t|--target-dir target-dir"
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% " source-dir is a directory which contains a start.cmd, machines.txt, and other resource files/dirs"
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% " or, a directory which contains an apps.txt each line specifying a sub-directory above (inside this directory)."
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% " Example: run deploy --source-dir .\skv\meta --target-dir d:\zhenyug deploys meta to d:\zhenyug\meta"
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% " Example: run deploy --source-dir .\skv --target-dir d:\zhenyug deploys skv\meta, skv\replica, ... to d:\zhenyug\meta, replica, ..."
    exit /B %DSN_TMP_EXIT_CODE%

REM  
REM |-source-dir|target-dir
REM   - start.cmd
REM   - machines.txt
REM   - other dependent files or dirs
REM 

    
:exit
    exit /B 0
