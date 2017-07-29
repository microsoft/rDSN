SET bin_dir=%~dp0
IF %bin_dir:~-1%==\ SET bin_dir=%bin_dir:~0,-1%
SET TOP_DIR=%bin_dir%\..\..

:: check VS environment
SET DSN_TMP_VS_FOUND=
IF "%VisualStudioVersion%"=="15.0" SET DSN_TMP_VS_FOUND=true
IF "%VisualStudioVersion%"=="14.0" SET DSN_TMP_VS_FOUND=true
IF NOT DEFINED DSN_TMP_VS_FOUND (
    CALL %bin_dir%\echoc.exe 4 "Visusal Studio 2015 or 2017 is not found, please run 'x64 Native Tools Command Prompt' and try later"
    SET DSN_TMP_VS_FOUND=
    exit /B 1
)
SET DSN_TMP_VS_FOUND=

IF NOT EXIST "%bin_dir%\ssed.exe" (
    CALL %bin_dir%\wget.exe --no-check-certificate https://github.com/imzhenyu/packages/raw/master/windows/ssed.exe?raw=true -P %bin_dir%
)

IF NOT EXIST "%bin_dir%\thrift.exe" (
    CALL %bin_dir%\wget.exe --no-check-certificate https://github.com/imzhenyu/thrift/raw/master/pre-built/windows8.1/thrift.exe -P %bin_dir%
)


IF NOT EXIST "%bin_dir%\7z.exe" (
    CALL %bin_dir%\wget.exe --no-check-certificate https://github.com/imzhenyu/packages/raw/master/windows/7z.dll?raw=true -P %bin_dir%
    CALL %bin_dir%\wget.exe --no-check-certificate https://github.com/imzhenyu/packages/raw/master/windows/7z.exe?raw=true -P %bin_dir%
    @copy /y "%bin_dir%\7z.dll" "%bin_dir%\..\"
    @copy /y "%bin_dir%\7z.exe" "%bin_dir%\..\"
)

IF NOT EXIST "%bin_dir%\php.exe" (
    CALL %bin_dir%\wget.exe --no-check-certificate https://github.com/imzhenyu/packages/raw/master/windows/php5.dll?raw=true -P %bin_dir%
    CALL %bin_dir%\wget.exe --no-check-certificate https://github.com/imzhenyu/packages/raw/master/windows/php.exe?raw=true -P %bin_dir%
    CALL %bin_dir%\wget.exe --no-check-certificate https://github.com/imzhenyu/packages/raw/master/windows/php.ini?raw=true -P %bin_dir%
)

SET DSN_TMP_BOOST_VERSION=1_64_0
SET DSN_TMP_BOOST_PACKAGE_NAME=boost_%DSN_TMP_BOOST_VERSION%.7z
IF NOT EXIST "%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%" (
    IF NOT EXIST "%TOP_DIR%\ext\%DSN_TMP_BOOST_PACKAGE_NAME%" CALL %bin_dir%\wget.exe --no-check-certificate http://github.com/imzhenyu/packages/blob/master/windows/%DSN_TMP_BOOST_PACKAGE_NAME%?raw=true -P %TOP_DIR%\ext
    ECHO Decompressing Boost %DSN_TMP_BOOST_VERSION% to "%TOP_DIR%\ext"
    CALL %bin_dir%\7z.exe x "%TOP_DIR%\ext\%DSN_TMP_BOOST_PACKAGE_NAME%" -y -o"%TOP_DIR%\ext" > nul
)
SET DSN_TMP_BOOST_VERSION=
SET DSN_TMP_BOOST_PACKAGE_NAME=

SET DSN_TMP_CMAKE_VERSION=3.9.0
SET DSN_TMP_CMAKE_PACKAGE_NAME=cmake-%DSN_TMP_CMAKE_VERSION%.7z
IF NOT EXIST "%TOP_DIR%\ext\cmake-%DSN_TMP_CMAKE_VERSION%" (
    IF NOT EXIST "%TOP_DIR%\ext\%DSN_TMP_CMAKE_PACKAGE_NAME%" CALL %bin_dir%\wget.exe --no-check-certificate http://github.com/imzhenyu/packages/blob/master/windows/%DSN_TMP_CMAKE_PACKAGE_NAME%?raw=true -P %TOP_DIR%\ext
    ECHO Decompressing cmake %DSN_TMP_CMAKE_VERSION% to "%TOP_DIR%\ext"
    CALL %bin_dir%\7z.exe x "%TOP_DIR%\ext\%DSN_TMP_CMAKE_PACKAGE_NAME%" -y -o"%TOP_DIR%\ext" > nul
)
SET DSN_TMP_CMAKE_VERSION=
SET DSN_TMP_CMAKE_PACKAGE_NAME=
