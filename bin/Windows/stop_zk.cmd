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

REM TASKKILL /F /FI "WINDOWTITLE eq zk-%PORT% - %INSTALL_DIR%\%zk%\bin\zkServer.cmd"
REM TASKKILL /F /FI "WINDOWTITLE eq zk-%PORT% - %INSTALL_DIR%\%zk%\bin\zkServer.cmd"
FOR /L %%i IN (0, 1, 1) DO (
    TASKKILL /F /FI "WINDOWTITLE eq zk-%PORT%-%ZOOKEEPER_HOME%\bin\zkServer.cmd"
)

GOTO exit

:usage
    ECHO run.cmd stop_zk [INSTALL_DIR = .\zk [PORT = 12181]]
    GOTO:EOF
    
:exit
