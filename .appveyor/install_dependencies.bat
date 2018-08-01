@echo off
setlocal

REM Environment variables

IF "%1"=="" (
    set BUILD_FOLDER=.
) ELSE (
    set BUILD_FOLDER=%1
)

IF "%2"=="" (
    set CONFIGURATION=Debug
) ELSE (
    set CONFIGURATION=%2
)

REM Set up dependency directories

IF NOT DEFINED GENERATOR (
    set GENERATOR=Visual Studio 15 2017 Win64
)
IF NOT DEFINED DEPENDENCY_DIR (
    set DEPENDENCY_DIR=%BUILD_FOLDER%\dependencies
)
IF NOT EXIST %DEPENDENCY_DIR% (
    mkdir "%DEPENDENCY_DIR%"
    mkdir "%DEPENDENCY_DIR%\install"
)
IF NOT DEFINED HUNTER_ROOT (
    set HUNTER_ROOT=%DEPENDENCY_DIR%\hunter_root
)

REM Download CMake 3.12
set CMAKE_VER=3.12.0
set CMAKE_VER_FULL=cmake-%CMAKE_VER%-win64-x64
set CMAKE_URL=https://cmake.org/files/v3.12/%CMAKE_VER_FULL%.zip
IF NOT EXIST %DEPENDENCY_DIR%\cmake (
    echo === Downloading %CMAKE_VER_FULL% === 
    powershell -Command "Invoke-WebRequest %CMAKE_URL% -OutFile %DEPENDENCY_DIR%\%CMAKE_VER_FULL%.zip"
    echo  === Unzipping cmake === 
    7z x -y -bd -bb0 -o%DEPENDENCY_DIR% %DEPENDENCY_DIR%\%CMAKE_VER_FULL%.zip
    echo Renaming %DEPENDENCY_DIR%\%CMAKE_VER_FULL to cmake
    rename "%DEPENDENCY_DIR%\%CMAKE_VER_FULL%" cmake
)
set CMAKE_BIN=%DEPENDENCY_DIR%\cmake\bin\cmake
set CTEST_BIN=%DEPENDENCY_DIR%\cmake\bin\ctest

REM Set common build flags and prefixes for czmq and msgpack
set COMMON_GENERATOR_FLAGS=-G "%GENERATOR%" -DCMAKE_INSTALL_PREFIX="%DEPENDENCY_DIR%\install" -DHUNTER_STATUS_PRINT=OFF -DCMAKE_INSTALL_MESSAGE=NEVER
set COMMON_BUILD_FLAGS=--config %CONFIGURATION% --target INSTALL -- /nologo /verbosity:minimal

echo === Clone patched hunterized CZMQ === 
IF NOT EXIST %DEPENDENCY_DIR%\czmq git clone https://github.com/mystfit/czmq.git %DEPENDENCY_DIR%\czmq

git -C %DEPENDENCY_DIR%\czmq checkout hunter-v4.1.0
mkdir "%DEPENDENCY_DIR%\czmq\build"
echo  === Building czmq === 
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\czmq" -B"%DEPENDENCY_DIR%\czmq\build" %COMMON_GENERATOR_FLAGS%
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\czmq\build" %COMMON_BUILD_FLAGS%

echo  === Cloning patched hunterized msgpack === 
IF NOT EXIST %DEPENDENCY_DIR%\msgpack-c git clone https://github.com/mystfit/msgpack-c.git %DEPENDENCY_DIR%\msgpack-c

git -C %DEPENDENCY_DIR%\msgpack-c checkout hunter-2.1.5
mkdir "%DEPENDENCY_DIR%\msgpack-c\build"
echo  === Building msgpack === 
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\msgpack-c" -B"%DEPENDENCY_DIR%\msgpack-c\build" %COMMON_GENERATOR_FLAGS% -DMSGPACK_BUILD_EXAMPLES=OFF
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\msgpack-c\build" %COMMON_BUILD_FLAGS%

set SWIG_VER=swigwin-3.0.12
IF NOT EXIST %DEPENDENCY_DIR%\swig (
    echo  === Downloading swig === 
    powershell -Command "Invoke-WebRequest https://phoenixnap.dl.sourceforge.net/project/swig/swigwin/swigwin-3.0.12/%SWIG_VER%.zip -OutFile %DEPENDENCY_DIR%\%SWIG_VER%.zip"
    echo  === Unzipping swig === 
    7z x -y -bd -bb0 -o%DEPENDENCY_DIR% %DEPENDENCY_DIR%\%SWIG_VER%.zip
    echo Renaming %DEPENDENCY_DIR%\%SWIG_VER to swig
    rename "%DEPENDENCY_DIR%\%SWIG_VER%" swig
)

