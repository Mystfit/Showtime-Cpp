@echo off
setlocal

REM Environment variables
IF NOT DEFINED BUILD_FOLDER (
    set BUILD_FOLDER=C:\projects\showtime-cpp
)

REM Set up dependency directories
pushd %BUILD_FOLDER%

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

set COMMON_GENERATOR_FLAGS=-G "%GENERATOR%" -DCMAKE_INSTALL_PREFIX="%DEPENDENCY_DIR%\install" -DHUNTER_STATUS_PRINT=OFF -DCMAKE_INSTALL_MESSAGE=NEVER
set COMMON_BUILD_FLAGS=--config %CONFIGURATION% --target INSTALL -- /nologo /verbosity:minimal

echo === Clone patched hunterized CZMQ === 
pushd "%DEPENDENCY_DIR%"
IF NOT EXIST %DEPENDENCY_DIR%\czmq git clone https://github.com/mystfit/czmq.git

pushd czmq
git checkout hunter-v4.1.0
mkdir "%DEPENDENCY_DIR%\czmq\build"
echo  === Building czmq === 
cmake -H. -B"%DEPENDENCY_DIR%\czmq\build" %COMMON_GENERATOR_FLAGS%
cmake --build "%DEPENDENCY_DIR%\czmq\build" %COMMON_BUILD_FLAGS%
popd

echo  === Cloning patched hunterized msgpack === 
IF NOT EXIST %DEPENDENCY_DIR%\msgpack-c git clone https://github.com/mystfit/msgpack-c.git

pushd msgpack-c
git checkout hunter-2.1.5
mkdir "%DEPENDENCY_DIR%\msgpack-c\build"
echo  === Building msgpack === 
cmake -H. -B"%DEPENDENCY_DIR%\msgpack-c\build" %COMMON_GENERATOR_FLAGS%
cmake --build "%DEPENDENCY_DIR%\msgpack-c\build" %COMMON_BUILD_FLAGS%
popd

IF NOT EXIST %DEPENDENCY_DIR%\swig (
	echo  === Downloading swig === 
    powershell -Command "Invoke-WebRequest https://phoenixnap.dl.sourceforge.net/project/swig/swigwin/swigwin-3.0.12/swigwin-3.0.12.zip -OutFile swigwin.zip"
    echo  === Unzipping swig === 
    7z x -y -bd -bb0 swigwin.zip
    ren .\swigwin-3.0.12 swig
)

REM Pop out of dependencies
popd
popd
