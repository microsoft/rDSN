SET bin_dir=%~dp0
SET data_dir=%1
IF "%data_dir%" EQU "" (
    set data_dir=.\cluster
)

REM copy binaries and common config files 
@mkdir %data_dir%
@mkdir %data_dir%\daemon
COPY /Y %DSN_ROOT:/=\%\bin\dsn.svchost.exe %data_dir%\daemon\
COPY /Y %DSN_ROOT:/=\%\lib\*.dll %data_dir%\daemon\
COPY /Y %DSN_ROOT:/=\%\bin\config.common.ini %data_dir%\daemon\
COPY /Y %DSN_ROOT:/=\%\bin\config.onecluster.ini %data_dir%\daemon\

@mkdir %data_dir%\meta
COPY /Y %data_dir%\daemon\* %data_dir%\meta\

REM start meta server and daemon servers
pushd %data_dir%\meta
start dsn.svchost.exe config.onecluster.ini -app_list meta
REM -overwrite core.pause_on_start=true
popd

pushd %data_dir%\daemon
for /l %%x in (1, 1, 8) do call :start_daemon %%x
popd

REM START web studio
start python %bin_dir%\..\..\webstudio\rDSN.WebStudio.py 

ECHO Now you can visit http://localhost:8088 to start playing with rDSN ...

goto :eof 

:start_daemon
    SET /a port=%1+24801
    @mkdir d%port%
    pushd d%port%
    start dsn.svchost.exe ..\config.onecluster.ini -app_list daemon -cargs daemon_port=%port%
    popd



