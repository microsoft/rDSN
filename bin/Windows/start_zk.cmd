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
        GOTO exit
    )    
    CALL %bin_dir%\7z.exe x %zk%.tar.gz -y -o"%INSTALL_DIR%" > nul
    CALL %bin_dir%\7z.exe x %zk%.tar -y -o"%INSTALL_DIR%" > nul
)

SET ZOOKEEPER_HOME=%INSTALL_DIR%\%zk%
SET ZOOKEEPER_PORT=%PORT%

copy /Y %ZOOKEEPER_HOME%\conf\zoo_sample.cfg %ZOOKEEPER_HOME%\conf\zoo.cfg
REM CALL %bin_dir%\ssed.exe -i "s@dataDir=/tmp/zookeeper@dataDir=%ZOOKEEPER_HOME:\=\\%\\data@" %ZOOKEEPER_HOME%\conf\zoo.cfg
CALL %bin_dir%\ssed.exe %ZOOKEEPER_HOME%\conf\zoo.cfg clientPort=2181 clientPort=%ZOOKEEPER_PORT%

@mkdir %ZOOKEEPER_HOME%\data
CALL start cmd.exe /k "title zk-%PORT%&& %ZOOKEEPER_HOME%\bin\zkServer.cmd"

popd

GOTO exit


:usage
    ECHO run.cmd start_zk [INSTALL_DIR = .\zk [PORT = 12181]]
    GOTO:EOF
    
:exit
