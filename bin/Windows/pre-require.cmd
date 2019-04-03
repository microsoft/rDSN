@ECHO OFF

SET bin_dir=%~dp0
IF %bin_dir:~-1%==\ SET bin_dir=%bin_dir:~0,-1%
SET TOP_DIR=%bin_dir%\..\..
PUSHD "%TOP_DIR%"
SET TOP_DIR=%CD%
POPD

:: detect VS
IF "%VisualStudioVersion%"=="15.0" GOTO find_vs
IF "%VisualStudioVersion%"=="14.0" GOTO find_vs
SET DSN_TMP_VS_INSTALL_DIR=
FOR /F "usebackq tokens=1* delims=: " %%i in (`"%bin_dir%\vswhere.exe" -latest`) DO (
  IF /i "%%i"=="installationPath" set DSN_TMP_VS_INSTALL_DIR=%%j
)
IF DEFINED DSN_TMP_VS_INSTALL_DIR (
    SET DSN_TMP_VS_INSTALL_DIR=
    GOTO find_vs
)
IF NOT "%VS140COMNTOOLS%"=="" GOTO find_vs

CALL "%bin_dir%\echoc.exe" 4 "Visusal Studio 2015 or 2017 is not found, please run 'x64 Native Tools Command Prompt' and try later"
SET DSN_TMP_VS_INSTALL_DIR=
exit /B 1

:find_vs

IF NOT EXIST "%bin_dir%\ssed.exe" (
    CALL "%bin_dir%\wget.exe" --no-check-certificate https://raw.githubusercontent.com/imzhenyu/packages/master/windows/ssed.exe -P "%bin_dir%"
)

IF NOT EXIST "%bin_dir%\thrift.exe" (
    CALL "%bin_dir%\wget.exe" --no-check-certificate https://raw.githubusercontent.com/imzhenyu/thrift/master/pre-built/windows8.1/thrift.exe -P "%bin_dir%"
)

IF NOT EXIST "%bin_dir%\7z.exe" (
    CALL "%bin_dir%\wget.exe" --no-check-certificate https://raw.githubusercontent.com/imzhenyu/packages/master/windows/7z.dll -P "%bin_dir%"
    CALL "%bin_dir%\wget.exe" --no-check-certificate https://raw.githubusercontent.com/imzhenyu/packages/master/windows/7z.exe -P "%bin_dir%"
    @copy /y "%bin_dir%\7z.dll" "%bin_dir%\..\"
    @copy /y "%bin_dir%\7z.exe" "%bin_dir%\..\"
)

IF NOT EXIST "%bin_dir%\php.exe" (
    CALL "%bin_dir%\wget.exe" --no-check-certificate https://raw.githubusercontent.com/imzhenyu/packages/master/windows/php5.dll -P "%bin_dir%"
    CALL "%bin_dir%\wget.exe" --no-check-certificate https://raw.githubusercontent.com/imzhenyu/packages/master/windows/php.exe -P "%bin_dir%"
    CALL "%bin_dir%\wget.exe" --no-check-certificate https://raw.githubusercontent.com/imzhenyu/packages/master/windows/php.ini -P "%bin_dir%"
)

SET DSN_TMP_BOOST_VERSION=1_64_0
SET DSN_TMP_BOOST_PACKAGE_NAME=boost_%DSN_TMP_BOOST_VERSION%.7z
IF NOT EXIST "%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%" (
    IF NOT EXIST "%TOP_DIR%\ext\%DSN_TMP_BOOST_PACKAGE_NAME%" CALL "%bin_dir%\wget.exe" --no-check-certificate https://raw.githubusercontent.com/imzhenyu/packages/master/windows/%DSN_TMP_BOOST_PACKAGE_NAME% -P "%TOP_DIR%\ext"
    CALL "%bin_dir%\echoc.exe" 2 "Decompressing Boost %DSN_TMP_BOOST_VERSION% to \"%TOP_DIR%\ext\""
    CALL "%bin_dir%\7z.exe" x "%TOP_DIR%\ext\%DSN_TMP_BOOST_PACKAGE_NAME%" -y -o"%TOP_DIR%\ext" > nul
)
SET DSN_TMP_BOOST_VERSION=
SET DSN_TMP_BOOST_PACKAGE_NAME=

SET DSN_TMP_CMAKE_VERSION=3.9.0
SET DSN_TMP_CMAKE_PACKAGE_NAME=cmake-%DSN_TMP_CMAKE_VERSION%.7z
IF NOT EXIST "%TOP_DIR%\ext\cmake-%DSN_TMP_CMAKE_VERSION%" (
    IF NOT EXIST "%TOP_DIR%\ext\%DSN_TMP_CMAKE_PACKAGE_NAME%" CALL "%bin_dir%\wget.exe" --no-check-certificate https://raw.githubusercontent.com/imzhenyu/packages/master/windows/%DSN_TMP_CMAKE_PACKAGE_NAME% -P "%TOP_DIR%\ext"
    CALL "%bin_dir%\echoc.exe" 2 "Decompressing cmake %DSN_TMP_CMAKE_VERSION% to \"%TOP_DIR%\ext\""
    CALL "%bin_dir%\7z.exe" x "%TOP_DIR%\ext\%DSN_TMP_CMAKE_PACKAGE_NAME%" -y -o"%TOP_DIR%\ext" > nul
)
SET DSN_TMP_CMAKE_VERSION=
SET DSN_TMP_CMAKE_PACKAGE_NAME=
