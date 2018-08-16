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

REM CMake
set CMAKE_VER=3.12.0
set CMAKE_VER_FULL=cmake-%CMAKE_VER%-win64-x64
set CMAKE_URL=https://cmake.org/files/v3.12/%CMAKE_VER_FULL%.zip

IF EXIST %DEPENDENCY_DIR%\cmake (
    echo Found CMake
) ELSE (
    echo === Downloading %CMAKE_VER_FULL% === 
    powershell -Command "Invoke-WebRequest %CMAKE_URL% -OutFile %DEPENDENCY_DIR%\%CMAKE_VER_FULL%.zip"
    echo  === Unzipping CMake === 
    7z x -y -bd -bb0 -o%DEPENDENCY_DIR% %DEPENDENCY_DIR%\%CMAKE_VER_FULL%.zip
    echo Renaming %DEPENDENCY_DIR%\%CMAKE_VER_FULL to cmake
    rename "%DEPENDENCY_DIR%\%CMAKE_VER_FULL%" cmake
)
set CMAKE_BIN=%DEPENDENCY_DIR%\cmake\bin\cmake
set CTEST_BIN=%DEPENDENCY_DIR%\cmake\bin\ctest

REM Set common build flags and prefixes for czmq and msgpack  
set INSTALL_PREFIX=%DEPENDENCY_DIR%\install
set COMMON_GENERATOR_FLAGS=-G "%GENERATOR%" -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" -DCMAKE_INSTALL_MESSAGE=NEVER -DCMAKE_PREFIX_PATH="%INSTALL_PREFIX%"
set COMMON_BUILD_FLAGS=--config %CONFIGURATION% --target INSTALL -- /nologo /verbosity:minimal


REM libZMQ
IF EXIST %DEPENDENCY_DIR%\libzmq (
    echo Found libZMQ
) ELSE (
    echo === Cloning libZMQ === 
    git clone https://github.com/mystfit/libzmq.git %DEPENDENCY_DIR%\libzmq
    git -C %DEPENDENCY_DIR%\libzmq checkout drafts
    mkdir "%DEPENDENCY_DIR%\libzmq\build"
    echo  === Building libzmq === 
    %CMAKE_BIN% -H"%DEPENDENCY_DIR%\libzmq" -B"%DEPENDENCY_DIR%\libzmq\build" %COMMON_GENERATOR_FLAGS% -DENABLE_DRAFTS=TRUE -DZMQ_BUILD_TESTS=OFF
    %CMAKE_BIN% --build "%DEPENDENCY_DIR%\libzmq\build" %COMMON_BUILD_FLAGS%
)

REM CZMQ
IF EXIST %DEPENDENCY_DIR%\czmq (
    echo Found CZMQ
) ELSE (
    echo === Cloning CZMQ === 
    git clone https://github.com/mystfit/czmq.git %DEPENDENCY_DIR%\czmq
    git -C %DEPENDENCY_DIR%\czmq checkout master
    mkdir "%DEPENDENCY_DIR%\czmq\build"
    echo  === Building czmq === 
    %CMAKE_BIN% -H"%DEPENDENCY_DIR%\czmq" -B"%DEPENDENCY_DIR%\czmq\build" %COMMON_GENERATOR_FLAGS% -DENABLE_DRAFTS=TRUE -DBUILD_TESTING=OFF -DLIBZMQ_FIND_USING_CMAKE_PACKAGE=ON
    %CMAKE_BIN% --build "%DEPENDENCY_DIR%\czmq\build" %COMMON_BUILD_FLAGS%
)

REM msgpack-c
IF EXIST %DEPENDENCY_DIR%\msgpack-c (
    echo Found MsgPack-C
) ELSE (
    echo === Cloning msgpack-c === 
    git clone https://github.com/msgpack/msgpack-c.git %DEPENDENCY_DIR%\msgpack-c
    git -C %DEPENDENCY_DIR%\msgpack-c fetch --all --tags --prune
    git -C %DEPENDENCY_DIR%\msgpack-c checkout cpp-3.0.1
    mkdir "%DEPENDENCY_DIR%\msgpack-c\build"
    echo === Building msgpack === 
    %CMAKE_BIN% -H"%DEPENDENCY_DIR%\msgpack-c" -B"%DEPENDENCY_DIR%\msgpack-c\build" %COMMON_GENERATOR_FLAGS% -DMSGPACK_BUILD_EXAMPLES=OFF
    %CMAKE_BIN% --build "%DEPENDENCY_DIR%\msgpack-c\build" %COMMON_BUILD_FLAGS%
)


REM fmt
IF EXIST %DEPENDENCY_DIR%\fmt (
    echo Found fmt
) ELSE (
    echo === Cloning fmt === 
    git clone https://github.com/fmtlib/fmt.git %DEPENDENCY_DIR%\fmt
    git -C %DEPENDENCY_DIR%\fmt fetch --all --tags --prune
    git -C %DEPENDENCY_DIR%\fmt checkout 5.1.0
    mkdir "%DEPENDENCY_DIR%\fmt\build"
    echo === Building fmt === 
    %CMAKE_BIN% -H"%DEPENDENCY_DIR%\fmt" -B"%DEPENDENCY_DIR%\fmt\build" %COMMON_GENERATOR_FLAGS%
    %CMAKE_BIN% --build "%DEPENDENCY_DIR%\fmt\build" %COMMON_BUILD_FLAGS%
)


REM swig
set SWIG_VER=swigwin-3.0.12
IF EXIST %DEPENDENCY_DIR%\swig (
    echo Found SWIG
) ELSE (
    echo === Downloading SWIG === 
    powershell -Command "Invoke-WebRequest https://phoenixnap.dl.sourceforge.net/project/swig/swigwin/swigwin-3.0.12/%SWIG_VER%.zip -OutFile %DEPENDENCY_DIR%\%SWIG_VER%.zip"
    echo === Unzipping swig === 
    7z x -y -bd -bb0 -o%DEPENDENCY_DIR% %DEPENDENCY_DIR%\%SWIG_VER%.zip
    echo Renaming %DEPENDENCY_DIR%\%SWIG_VER to swig
    rename "%DEPENDENCY_DIR%\%SWIG_VER%" swig
)


REM Unity
SET UNITY_EXE="%DEPENDENCY_DIR%\unity\Editor\Unity.exe"
if EXIST %DEPENDENCY_DIR%\unity (
    echo Found Unity
) ELSE (
    IF NOT EXIST %DEPENDENCY_DIR%\UnitySetup64.exe (
        echo === Downloading Unity === 
        SET UNITY_URL=https://netstorage.unity3d.com/unity/1a9968d9f99c/Windows64EditorInstaller/UnitySetup64-2018.2.1f1.exe
        powershell -Command "Invoke-WebRequest $env:UNITY_URL -OutFile %DEPENDENCY_DIR%\UnitySetup64.exe"
    )
    echo === Installing Unity === 
    %DEPENDENCY_DIR%\UnitySetup64.exe /S /D=%DEPENDENCY_DIR%\unity
    del %DEPENDENCY_DIR%\UnitySetup64.exe
)
