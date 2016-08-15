SET bin_dir=%~dp0
SET TOP_DIR=%bin_dir%\..\..
SET INSTALL_DIR=%~f1
SET PORT=%2
SET zk=zookeeper-3.4.6

IF "%INSTALL_DIR%" EQU "" (
    set INSTALL_DIR=%TOP_DIR%\zk
)

IF "%PORT%" EQU "" (
    SET PORT=12181
)

CALL %bin_dir%\pre-require.cmd

IF NOT EXIST %INSTALL_DIR% mkdir %INSTALL_DIR%


pushd %INSTALL_DIR%

IF NOT EXIST %INSTALL_DIR%\%zk% (
    CALL %bin_dir%\wget.exe --no-check-certificate https://github.com/shengofsun/packages/raw/master/%zk%.tar.gz?raw=true
    IF NOT EXIST %zk%.tar.gz (
        CALL %bin_dir%\echoc.exe 4 download zookeeper package failed from  https://github.com/shengofsun/packages/raw/master/%zk%.tar.gz?raw=true
        popd
        exit /b 1
    ) 
    
    CALL %bin_dir%\7z.exe x %zk%.tar.gz -y -o"%INSTALL_DIR%"
    CALL %bin_dir%\7z.exe x %zk%.tar -y -o"%INSTALL_DIR%"
)

SET ZOOKEEPER_HOME=%INSTALL_DIR%\%zk%
SET ZOOKEEPER_PORT=%PORT%
SET ZK_SRC_CONFIG=%ZOOKEEPER_HOME%\conf\zoo_sample.cfg
SET ZK_DST_CONFIG=%ZOOKEEPER_HOME%\conf\zoo.cfg

ECHO # GENERATD BY rDSN SCRIPT > %ZK_DST_CONFIG%

for /F  %%A in (%ZK_SRC_CONFIG%) do (
    SET "line=%%A"
    setlocal enabledelayedexpansion    
    set "line=!line:2181=12181!"
    if defined line (
        echo(!line! >> %ZK_DST_CONFIG%
    )
    endlocal
)

@mkdir %ZOOKEEPER_HOME%\data

powershell -command "Start-Process %ZOOKEEPER_HOME%\bin\zkServer.cmd"

REM CALL start cmd.exe /k "title zk-%PORT%&& %ZOOKEEPER_HOME%\bin\zkServer.cmd"

popd

GOTO exit


:usage
    ECHO run.cmd start_zk [INSTALL_DIR = .\zk [PORT = 12181]]
    GOTO:EOF
    
:exit
    exit /b 0