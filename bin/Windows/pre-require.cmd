@ECHO OFF

SET bin_dir=%~dp0
IF %bin_dir:~-1%==\ SET bin_dir=%bin_dir:~0,-1%
SET TOP_DIR=%bin_dir%\..\..
PUSHD "%TOP_DIR%"
SET TOP_DIR=%CD%
POPD

:: detect VS
IF NOT "%VisualStudioVersion%"=="" GOTO find_vs
SET DSN_TMP_VS_INSTALL_DIR=
FOR /F "usebackq tokens=1* delims=: " %%i in (`"%bin_dir%\vswhere.exe" -latest`) DO (
  IF /i "%%i"=="installationPath" set DSN_TMP_VS_INSTALL_DIR=%%j
)
IF DEFINED DSN_TMP_VS_INSTALL_DIR (
    SET DSN_TMP_VS_INSTALL_DIR=
    GOTO find_vs
)
IF NOT "%VS140COMNTOOLS%"=="" GOTO find_vs

CALL "%bin_dir%\echoc.exe" 4 "Visual Studio 2015, 2017, 2019, 2022 or 2026 is not found, please run 'x64 Native Tools Command Prompt' and try later"
SET DSN_TMP_VS_INSTALL_DIR=
exit /B 1

:find_vs

IF NOT EXIST "%bin_dir%\ssed.exe" (
    CALL "%bin_dir%\download.cmd" https://raw.githubusercontent.com/linmajia/packages/master/windows/ssed.exe "%bin_dir%" ssed.exe
    IF ERRORLEVEL 1 GOTO error
)

IF NOT EXIST "%bin_dir%\thrift.exe" (
    CALL "%bin_dir%\download.cmd" https://raw.githubusercontent.com/linmajia/thrift/master/pre-built/windows8.1/thrift.exe "%bin_dir%" thrift.exe
    IF ERRORLEVEL 1 GOTO error
)

IF NOT EXIST "%bin_dir%\protoc.exe" (
    CALL "%bin_dir%\download.cmd" https://raw.githubusercontent.com/linmajia/protobuf/master/pre-built/windows8.1/protoc.exe "%bin_dir%" protoc.exe
    IF ERRORLEVEL 1 GOTO error
)

IF NOT EXIST "%bin_dir%\7z.exe" (
    CALL "%bin_dir%\download.cmd" https://raw.githubusercontent.com/linmajia/packages/master/windows/7z.dll "%bin_dir%" 7z.dll
    IF ERRORLEVEL 1 GOTO error
    CALL "%bin_dir%\download.cmd" https://raw.githubusercontent.com/linmajia/packages/master/windows/7z.exe "%bin_dir%" 7z.exe
    IF ERRORLEVEL 1 GOTO error
)

IF NOT EXIST "%bin_dir%\php.exe" (
    CALL "%bin_dir%\download.cmd" https://raw.githubusercontent.com/linmajia/packages/master/windows/php5.dll "%bin_dir%" php5.dll
    IF ERRORLEVEL 1 GOTO error
    CALL "%bin_dir%\download.cmd" https://raw.githubusercontent.com/linmajia/packages/master/windows/php.exe "%bin_dir%" php.exe
    IF ERRORLEVEL 1 GOTO error
    CALL "%bin_dir%\download.cmd" https://raw.githubusercontent.com/linmajia/packages/master/windows/php.ini "%bin_dir%" php.ini
    IF ERRORLEVEL 1 GOTO error
)

SET DSN_TMP_BOOST_PACKAGE_NAME=boost_%DSN_TMP_BOOST_VERSION%.7z
IF NOT EXIST "%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%" (
    IF NOT EXIST "%TOP_DIR%\ext\%DSN_TMP_BOOST_PACKAGE_NAME%" (
        CALL "%bin_dir%\download.cmd" https://raw.githubusercontent.com/linmajia/packages/master/windows/%DSN_TMP_BOOST_PACKAGE_NAME% "%TOP_DIR%\ext" %DSN_TMP_BOOST_PACKAGE_NAME%
        IF ERRORLEVEL 1 GOTO error
    )
    CALL "%bin_dir%\echoc.exe" 2 "Decompressing Boost %DSN_TMP_BOOST_VERSION% to \"%TOP_DIR%\ext\""
    CALL "%bin_dir%\7z.exe" x "%TOP_DIR%\ext\%DSN_TMP_BOOST_PACKAGE_NAME%" -y -o"%TOP_DIR%\ext" > nul
)
SET DSN_TMP_BOOST_PACKAGE_NAME=
IF NOT EXIST "%TOP_DIR%\ext\boost_%DSN_TMP_BOOST_VERSION%" (
    CALL "%bin_dir%\echoc.exe" 4 "Boost does not exist!"
    GOTO error
)

IF 0 EQU 1 (
    REM Keep the old bundled cmake download for easy rollback. It is too old
    REM for the current cmake_minimum_required(VERSION 3.22), so prefer a
    REM user-installed CMake or a manually unpacked ext\cmake-%DSN_TMP_CMAKE_VERSION%.
    SET DSN_TMP_CMAKE_PACKAGE_NAME=cmake-3.14.1.7z
    IF NOT EXIST "%TOP_DIR%\ext\cmake-3.14.1" (
        IF NOT EXIST "%TOP_DIR%\ext\%DSN_TMP_CMAKE_PACKAGE_NAME%" (
            CALL "%bin_dir%\download.cmd" https://raw.githubusercontent.com/linmajia/packages/master/windows/%DSN_TMP_CMAKE_PACKAGE_NAME% "%TOP_DIR%\ext" %DSN_TMP_CMAKE_PACKAGE_NAME%
            IF ERRORLEVEL 1 GOTO error
        )
        CALL "%bin_dir%\echoc.exe" 2 "Decompressing cmake 3.14.1 to \"%TOP_DIR%\ext\""
        CALL "%bin_dir%\7z.exe" x "%TOP_DIR%\ext\%DSN_TMP_CMAKE_PACKAGE_NAME%" -y -o"%TOP_DIR%\ext" > nul
    )
    SET DSN_TMP_CMAKE_PACKAGE_NAME=
)

SET DSN_TMP_CMAKE_EXE=
SET DSN_TMP_CMAKE_DIR=%TOP_DIR%\ext\cmake-%DSN_TMP_CMAKE_VERSION%
SET DSN_TMP_CMAKE_PACKAGE_NAME=cmake-%DSN_TMP_CMAKE_VERSION%-windows-x86_64.zip
SET DSN_TMP_CMAKE_UNPACKED_DIR=%TOP_DIR%\ext\cmake-%DSN_TMP_CMAKE_VERSION%-windows-x86_64
IF NOT EXIST "%DSN_TMP_CMAKE_DIR%\bin\cmake.exe" (
    IF NOT EXIST "%TOP_DIR%\ext\%DSN_TMP_CMAKE_PACKAGE_NAME%" (
        CALL "%bin_dir%\download.cmd" https://raw.githubusercontent.com/linmajia/packages/master/windows/%DSN_TMP_CMAKE_PACKAGE_NAME% "%TOP_DIR%\ext" %DSN_TMP_CMAKE_PACKAGE_NAME%
        IF ERRORLEVEL 1 GOTO error
    )
    IF EXIST "%TOP_DIR%\ext\%DSN_TMP_CMAKE_PACKAGE_NAME%" (
        CALL "%bin_dir%\echoc.exe" 2 "Decompressing cmake %DSN_TMP_CMAKE_VERSION% to \"%TOP_DIR%\ext\""
        CALL "%bin_dir%\7z.exe" x "%TOP_DIR%\ext\%DSN_TMP_CMAKE_PACKAGE_NAME%" -y -o"%TOP_DIR%\ext" > nul
        IF EXIST "%DSN_TMP_CMAKE_UNPACKED_DIR%\bin\cmake.exe" (
            IF EXIST "%DSN_TMP_CMAKE_DIR%" rmdir /S /Q "%DSN_TMP_CMAKE_DIR%"
            REN "%DSN_TMP_CMAKE_UNPACKED_DIR%" "cmake-%DSN_TMP_CMAKE_VERSION%"
        )
    )
)

IF EXIST "%DSN_TMP_CMAKE_DIR%\bin\cmake.exe" (
    SET DSN_TMP_CMAKE_EXE=%DSN_TMP_CMAKE_DIR%\bin\cmake.exe
) ELSE (
    FOR /F "delims=" %%i IN ('dir /b /s "%DSN_TMP_CMAKE_DIR%\cmake.exe" 2^>nul') DO IF NOT DEFINED DSN_TMP_CMAKE_EXE SET DSN_TMP_CMAKE_EXE=%%i
    IF NOT DEFINED DSN_TMP_CMAKE_EXE FOR /F "delims=" %%i IN ('WHERE cmake.exe 2^>nul') DO IF NOT DEFINED DSN_TMP_CMAKE_EXE SET DSN_TMP_CMAKE_EXE=%%i
)

IF NOT DEFINED DSN_TMP_CMAKE_EXE (
    CALL "%bin_dir%\echoc.exe" 4 "CMake %DSN_TMP_CMAKE_VERSION% or newer is required. Please install CMake and add it to PATH."
    GOTO error
)

SET DSN_TMP_CMAKE_FOUND_VERSION=
FOR /F "tokens=3" %%i IN ('"%DSN_TMP_CMAKE_EXE%" --version 2^>nul ^| findstr /B /C:"cmake version"') DO SET DSN_TMP_CMAKE_FOUND_VERSION=%%i
IF NOT DEFINED DSN_TMP_CMAKE_FOUND_VERSION (
    CALL "%bin_dir%\echoc.exe" 4 "failed to detect CMake version from %DSN_TMP_CMAKE_EXE%"
    GOTO error
)

FOR /F "tokens=1,2 delims=." %%i IN ("%DSN_TMP_CMAKE_FOUND_VERSION%") DO (
    SET DSN_TMP_CMAKE_MAJOR=%%i
    SET DSN_TMP_CMAKE_MINOR=%%j
)
IF %DSN_TMP_CMAKE_MAJOR% LSS 3 (
    CALL "%bin_dir%\echoc.exe" 4 "CMake %DSN_TMP_CMAKE_VERSION% or newer is required, found %DSN_TMP_CMAKE_FOUND_VERSION%"
    GOTO error
)
IF %DSN_TMP_CMAKE_MAJOR% EQU 3 IF %DSN_TMP_CMAKE_MINOR% LSS 22 (
    CALL "%bin_dir%\echoc.exe" 4 "CMake %DSN_TMP_CMAKE_VERSION% or newer is required, found %DSN_TMP_CMAKE_FOUND_VERSION%"
    GOTO error
)
SET DSN_TMP_CMAKE_MAJOR=
SET DSN_TMP_CMAKE_MINOR=
SET DSN_TMP_CMAKE_FOUND_VERSION=
SET DSN_TMP_CMAKE_DIR=
SET DSN_TMP_CMAKE_PACKAGE_NAME=
SET DSN_TMP_CMAKE_UNPACKED_DIR=

exit /B 0

:error
    exit /B 1
