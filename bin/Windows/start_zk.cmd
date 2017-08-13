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

CALL "%bin_dir%\pre-require.cmd"

IF NOT EXIST "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"

PUSHD "%INSTALL_DIR%"

IF NOT EXIST "%INSTALL_DIR%\%zk%" (
    CALL "%bin_dir%\wget.exe" --no-check-certificate https://github.com/shengofsun/packages/raw/master/%zk%.tar.gz?raw=true
    IF NOT EXIST %zk%.tar.gz (
        CALL "%bin_dir%\echoc.exe" 4 download zookeeper package failed from  https://github.com/shengofsun/packages/raw/master/%zk%.tar.gz?raw=true
        POPD
        EXIT /B 1
    ) 
    
    CALL "%bin_dir%\echoc.exe" 2 "Decompressing %zk% to \"%INSTALL_DIR%\""
    CALL "%bin_dir%\7z.exe" x %zk%.tar.gz -y -o"%INSTALL_DIR%" > nul
    CALL "%bin_dir%\7z.exe" x %zk%.tar -y -o"%INSTALL_DIR%" > nul
)

SET ZOOKEEPER_HOME=%INSTALL_DIR%\%zk%
SET ZOOKEEPER_PORT=%PORT%
SET ZK_SRC_CONFIG=%ZOOKEEPER_HOME%\conf\zoo_sample.cfg
SET ZK_DST_CONFIG=%ZOOKEEPER_HOME%\conf\zoo.cfg

ECHO # GENERATD BY rDSN SCRIPT > "%ZK_DST_CONFIG%"

FOR /F "usebackq" %%A in ("%ZK_SRC_CONFIG%") do (
    SET "line=%%A"
    setlocal enabledelayedexpansion    
    SET "line=!line:2181=12181!"
    IF defined line (
        ECHO(!line! >> "%ZK_DST_CONFIG%"
    )
    endlocal
)

IF NOT EXIST "%ZOOKEEPER_HOME%\data" @mkdir "%ZOOKEEPER_HOME%\data"

REM powershell -command "Start-Process %ZOOKEEPER_HOME%\bin\zkServer.cmd"
powershell -command "Start-Process -FilePath cmd.exe  -ArgumentList '/C title zk-%PORT%-%ZOOKEEPER_HOME%\bin\zkServer.cmd&&\"%ZOOKEEPER_HOME%\bin\zkServer.cmd\"'"

REM CALL start cmd.exe /k "title zk-%PORT%&& %ZOOKEEPER_HOME%\bin\zkServer.cmd"

POPD

GOTO exit


:usage
    ECHO run.cmd start_zk [INSTALL_DIR = .\zk [PORT = 12181]]
    GOTO:EOF
    
:exit
    EXIT /B 0