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

SET ZOOKEEPER_HOME=%INSTALL_DIR%\%zk%
SET ZOOKEEPER_WINDOW_TITLE=zk-%PORT%
SET ZOOKEEPER_PID_FILE=%INSTALL_DIR%\zk-%PORT%.pid

IF EXIST "%ZOOKEEPER_PID_FILE%" (
    FOR /F "usebackq" %%P IN ("%ZOOKEEPER_PID_FILE%") DO TASKKILL /F /T /PID %%P
    DEL /F /Q "%ZOOKEEPER_PID_FILE%"
)

TASKKILL /F /T /FI "WINDOWTITLE eq %ZOOKEEPER_WINDOW_TITLE%*"
powershell -NoProfile -Command "Get-CimInstance Win32_Process | Where-Object { ($_.Name -eq 'java.exe' -or $_.Name -eq 'javaw.exe') -and $_.CommandLine -like '*org.apache.zookeeper.server.quorum.QuorumPeerMain*' -and $_.CommandLine -like ('*' + $env:ZOOKEEPER_HOME + '*') } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force }"

GOTO exit

:usage
    IF "%DSN_TMP_USAGE_LEVEL%" EQU "" SET DSN_TMP_USAGE_LEVEL=4
    CALL "%bin_dir%\echoc.exe" %DSN_TMP_USAGE_LEVEL% "run.cmd stop_zk [INSTALL_DIR = .\zk [PORT = 12181]]"
    exit /B 0
    
:exit
