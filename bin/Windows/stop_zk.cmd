@ECHO OFF

SET bin_dir=%~dp0
SET TOP_DIR=%bin_dir%\..\..
PUSHD "%TOP_DIR%"
SET TOP_DIR=%CD%
POPD
SET INSTALL_DIR=%~f1
SET PORT=%2
SET zk=zookeeper-3.4.6

IF "%INSTALL_DIR%" EQU "" (
    set INSTALL_DIR=%TOP_DIR%\zk
)

IF "%PORT%" EQU "" (
    SET PORT=12181
)

SET ZOOKEEPER_WINDOW_TITLE=zk-%PORT%

TASKKILL /F /T /FI "WINDOWTITLE eq %ZOOKEEPER_WINDOW_TITLE%"

GOTO exit

:usage
    IF "%DSN_TMP_USAGE_LEVEL%" EQU "" SET DSN_TMP_USAGE_LEVEL=4
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% "run.cmd stop_zk [INSTALL_DIR = .\zk [PORT = 12181]]"
    exit /B 0
    
:exit
