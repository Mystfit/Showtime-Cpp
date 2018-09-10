@echo off
setlocal

REM Environment variables
IF "%1"=="" (
    set BUILD_FOLDER=%cd%
) ELSE (
    set BUILD_FOLDER=%1
)

IF "%2"=="" (
    set CONFIGURATION=debug
) ELSE (
    set CONFIGURATION=%2
)

IF %CONFIGURATION% EQU "debug" (
    set CONFIG_WCAPS="Debug"
) ELSE IF %CONFIGURATION% EQU "release" (
    set CONFIG_WCAPS="Release"
)

IF "%3"=="--with-unity" (
    set WITH_UNITY=1
) ELSE (
    set WITH_UNITY=0
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
set CMAKE_VER=3.12.1
set CMAKE_VER_FULL=cmake-%CMAKE_VER%-win64-x64
set CMAKE_URL=https://cmake.org/files/v3.12/%CMAKE_VER_FULL%.zip

IF EXIST %DEPENDENCY_DIR%\%CMAKE_VER_FULL%.zip (
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
set COMMON_BUILD_FLAGS=--config %CONFIG_WCAPS% --target INSTALL -- /nologo /verbosity:minimal


REM libZMQ
IF EXIST %DEPENDENCY_DIR%\libzmq\build (
    echo Found libZMQ
) ELSE (
    echo === Cloning libZMQ === 
    git clone https://github.com/mystfit/libzmq.git %DEPENDENCY_DIR%\libzmq
    REM git -C %DEPENDENCY_DIR%\libzmq fetch --all --tags --prune
    REM git -C %DEPENDENCY_DIR%\libzmq checkout master
    git -C %DEPENDENCY_DIR%\libzmq checkout 4.2.5-drafts-fixed
    mkdir "%DEPENDENCY_DIR%\libzmq\build"
)
echo  === Building libzmq === 
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\libzmq" -B"%DEPENDENCY_DIR%\libzmq\build" %COMMON_GENERATOR_FLAGS% -DENABLE_DRAFTS=TRUE -DZMQ_BUILD_TESTS=OFF
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\libzmq\build" %COMMON_BUILD_FLAGS%


REM CZMQ
IF EXIST %DEPENDENCY_DIR%\czmq\build (
    echo Found CZMQ
) ELSE (
    echo === Cloning CZMQ === 
    git clone https://github.com/mystfit/czmq.git %DEPENDENCY_DIR%\czmq
    git -C %DEPENDENCY_DIR%\czmq checkout master
    mkdir "%DEPENDENCY_DIR%\czmq\build"
)
echo  === Building czmq === 
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\czmq" -B"%DEPENDENCY_DIR%\czmq\build" %COMMON_GENERATOR_FLAGS% -DENABLE_DRAFTS=TRUE -DBUILD_TESTING=OFF -DLIBZMQ_FIND_USING_CMAKE_PACKAGE=ON
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\czmq\build" %COMMON_BUILD_FLAGS%


REM msgpack-c
IF EXIST %DEPENDENCY_DIR%\msgpack-c\build (
    echo Found MsgPack-C
) ELSE (
    echo === Cloning msgpack-c === 
    git clone https://github.com/msgpack/msgpack-c.git %DEPENDENCY_DIR%\msgpack-c
    git -C %DEPENDENCY_DIR%\msgpack-c fetch --all --tags --prune
    git -C %DEPENDENCY_DIR%\msgpack-c checkout cpp-3.0.1
    mkdir "%DEPENDENCY_DIR%\msgpack-c\build"
)
echo === Building msgpack === 
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\msgpack-c" -B"%DEPENDENCY_DIR%\msgpack-c\build" %COMMON_GENERATOR_FLAGS% -DMSGPACK_BUILD_EXAMPLES=OFF
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\msgpack-c\build" %COMMON_BUILD_FLAGS%


REM fmt
IF EXIST %DEPENDENCY_DIR%\fmt\build (
    echo Found fmt
) ELSE (
    echo === Cloning fmt === 
    git clone https://github.com/fmtlib/fmt.git %DEPENDENCY_DIR%\fmt
    git -C %DEPENDENCY_DIR%\fmt fetch --all --tags --prune
    git -C %DEPENDENCY_DIR%\fmt checkout 5.1.0
    mkdir "%DEPENDENCY_DIR%\fmt\build"
)
echo === Building fmt === 
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\fmt" -B"%DEPENDENCY_DIR%\fmt\build" %COMMON_GENERATOR_FLAGS%
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\fmt\build" %COMMON_BUILD_FLAGS%


REM mpark variant
IF EXIST %DEPENDENCY_DIR%\variant\build (
    echo Found variant
) ELSE (
    echo === Cloning mpark-variant ===
    git clone https://github.com/mpark/variant.git %DEPENDENCY_DIR%\variant
    mkdir "%DEPENDENCY_DIR%\variant\build"
)
echo === Building variant ===
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\variant" -B"%DEPENDENCY_DIR%\variant\build" %COMMON_GENERATOR_FLAGS%
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\variant\build" %COMMON_BUILD_FLAGS%


REM boost
set BOOST_COMMON_FLAGS=--prefix=%DEPENDENCY_DIR%\install address-model=64 variant=%CONFIGURATION% threading=multi runtime-link=shared
set BOOST_SHARED_LIB_FLAGS=--with-system --with-chrono link=shared
set BOOST_STATIC_LIB_FLAGS=--with-log --with-thread --with-filesystem --with-date_time --with-atomic --with-regex link=static
IF EXIST %DEPENDENCY_DIR%\boost_1.68.0.zip (
    echo Found boost
) ELSE (
    echo === Downloading boost ===
    powershell -Command "Invoke-WebRequest http://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.zip -OutFile %DEPENDENCY_DIR%\boost_1.68.0.zip"
    7z x -y -bd -bb0 -o%DEPENDENCY_DIR% %DEPENDENCY_DIR%\boost_1.68.0.zip
    mkdir %DEPENDENCY_DIR%\boost_1_68_0\build
)
echo === Building boost ===
pushd %DEPENDENCY_DIR%\boost_1_68_0
REM call .\bootstrap.bat
echo .\b2.exe install %BOOST_SHARED_LIB_FLAGS% %BOOST_COMMON_FLAGS%
call .\b2.exe install %BOOST_SHARED_LIB_FLAGS% %BOOST_COMMON_FLAGS%
call .\b2.exe install %BOOST_STATIC_LIB_FLAGS% %BOOST_COMMON_FLAGS%
popd


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
SET UNITY_URL=https://netstorage.unity3d.com/unity/1a9968d9f99c/Windows64EditorInstaller/UnitySetup64-2018.2.1f1.exe
IF %WITH_UNITY% EQU 1 (
    IF EXIST %DEPENDENCY_DIR%\UnitySetup64.exe (
        echo Found Unity
    ) ELSE (
        echo === Downloading Unity === 
        powershell -Command "Invoke-WebRequest $env:UNITY_URL -OutFile %DEPENDENCY_DIR%\UnitySetup64.exe"
    )
    echo === Installing Unity === 
    mkdir "%DEPENDENCY_DIR%\unity"
    %DEPENDENCY_DIR%\UnitySetup64.exe /S /D="%DEPENDENCY_DIR%\unity"
    del "%DEPENDENCY_DIR%\UnitySetup64.exe"
)


REM rtmidi
IF EXIST %DEPENDENCY_DIR%\rtmidi\build (
    echo Found RtMidi
) ELSE (
    echo === Cloning RtMidi ===
    git clone https://github.com/thestk/rtmidi.git %DEPENDENCY_DIR%\rtmidi
    mkdir "%DEPENDENCY_DIR%\rtmidi\build"
)
echo === Building RtMidi ===
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\rtmidi" -B"%DEPENDENCY_DIR%\rtmidi\build" %COMMON_GENERATOR_FLAGS% -DBUILD_TESTING=OFF
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\rtmidi\build" %COMMON_BUILD_FLAGS%
