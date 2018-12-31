@echo off
setlocal EnableDelayedExpansion


REM Command line arguments
:processargs
SET ARG=%1
IF DEFINED ARG (
    IF "%ARG%" EQU "--build-dir" (
        set BUILD_FOLDER=%2
       
        SHIFT
    )

    IF "%ARG%" EQU "--config" (
        set CONFIGURATION=%2
        
        SHIFT
    )

    IF "%ARG%" EQU "--with-boost" (
        set WITH_BOOST=1
        echo With Boost
    )

    IF "%ARG%" EQU "--with-unity" (
        set WITH_UNITY=1
        echo With Unity
    )

    SHIFT
    GOTO processargs
)

REM Expand variables supplied from :processargs
set BUILD_FOLDER=!BUILD_FOLDER!
set CONFIGURATION=!CONFIGURATION!

REM Default paths and variables
IF NOT DEFINED BUILD_FOLDER (
    set BUILD_FOLDER=%cd%
)
echo Build folder=!BUILD_FOLDER!

IF NOT DEFINED CONFIGURATION (
    set CONFIGURATION=debug
)
echo Configuration=!CONFIGURATION!
for /F %%s IN ('python -c "print(\"%CONFIGURATION%\".lower())"') DO set CONFIG_LOWER=%%s

IF NOT DEFINED WITH_UNITY (
    set WITH_UNITY=0
)

IF NOT DEFINED WITH_BOOST (
    set WITH_BOOST=0
)

set GENERATOR=Visual Studio 15 2017 Win64
set DEPENDENCY_DIR=%BUILD_FOLDER%\dependencies
IF NOT EXIST %DEPENDENCY_DIR% (
    mkdir "%DEPENDENCY_DIR%"
    mkdir "%DEPENDENCY_DIR%\install"
)

REM CMake
set CMAKE_VER=3.13.0
set CMAKE_VER_FULL=cmake-%CMAKE_VER%-win64-x64
set CMAKE_URL=https://cmake.org/files/v3.13/%CMAKE_VER_FULL%.zip
IF EXIST %DEPENDENCY_DIR%\cmake (
    echo Found CMake
) ELSE (
    echo === Downloading %CMAKE_VER_FULL% === 
    powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest %CMAKE_URL% -OutFile %DEPENDENCY_DIR%\cmake.zip"
    
    echo  === Unzipping CMake === 
    7z x -y -bd -bb0 -o%DEPENDENCY_DIR% %DEPENDENCY_DIR%\cmake.zip
    echo Renaming %DEPENDENCY_DIR%\%CMAKE_VER_FULL% to cmake
    rename "%DEPENDENCY_DIR%\%CMAKE_VER_FULL%" cmake
)

set CMAKE_BIN=%DEPENDENCY_DIR%\cmake\bin\cmake
set CTEST_BIN=%DEPENDENCY_DIR%\cmake\bin\ctest

REM Set common build flags and prefixes for czmq and msgpack  
set INSTALL_PREFIX=%DEPENDENCY_DIR%\install
set COMMON_GENERATOR_FLAGS=-G "%GENERATOR%" -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" -DCMAKE_INSTALL_MESSAGE=NEVER -DCMAKE_PREFIX_PATH="%INSTALL_PREFIX%"
set COMMON_BUILD_FLAGS=--config %CONFIGURATION% --target INSTALL -- /nologo /verbosity:minimal


REM libZMQ
IF EXIST %DEPENDENCY_DIR%\libzmq\build (
    echo Found libZMQ
) ELSE (
    echo === Cloning libZMQ === 
    git clone https://github.com/zeromq/libzmq.git %DEPENDENCY_DIR%\libzmq
    REM git -C %DEPENDENCY_DIR%\libzmq fetch --all --tags --prune
    REM git -C %DEPENDENCY_DIR%\libzmq checkout v4.3.0
    REM git clone https://github.com/mystfit/libzmq.git %DEPENDENCY_DIR%\libzmq
    REM git -C %DEPENDENCY_DIR%\libzmq checkout master
    REM git -C %DEPENDENCY_DIR%\libzmq checkout 4.2.5-drafts-fixed
    mkdir "%DEPENDENCY_DIR%\libzmq\build"
)
echo  === Building libzmq === 
REM  -DPOLLER=epoll -DAPI_POLLER=poll
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\libzmq" -B"%DEPENDENCY_DIR%\libzmq\build" %COMMON_GENERATOR_FLAGS% -DENABLE_DRAFTS=TRUE -DZMQ_BUILD_TESTS=OFF
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\libzmq\build" %COMMON_BUILD_FLAGS%


REM CZMQ
IF EXIST %DEPENDENCY_DIR%\czmq\build (
    echo Found CZMQ
) ELSE (
    echo === Cloning CZMQ === 
    REM git clone https://github.com/zeromq/czmq.git %DEPENDENCY_DIR%\czmq
    REM git -C %DEPENDENCY_DIR%\czmq fetch --all --tags --prune
    REM git -C %DEPENDENCY_DIR%\czmq checkout master
    git clone https://github.com/mystfit/czmq.git %DEPENDENCY_DIR%\czmq
    git -C %DEPENDENCY_DIR%\czmq checkout master
    mkdir "%DEPENDENCY_DIR%\czmq\build"
)
echo  === Building czmq === 
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\czmq" -B"%DEPENDENCY_DIR%\czmq\build" %COMMON_GENERATOR_FLAGS% -DENABLE_DRAFTS=TRUE -DBUILD_TESTING=OFF -DLIBZMQ_FIND_USING_CMAKE_PACKAGE=ON
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\czmq\build" %COMMON_BUILD_FLAGS%


REM msgpack-c
IF EXIST %DEPENDENCY_DIR%\msgpack\build (
    echo Found MsgPack
) ELSE (
    echo === Cloning msgpack === 
    git clone https://github.com/msgpack/msgpack-c.git %DEPENDENCY_DIR%\msgpack
    git -C %DEPENDENCY_DIR%\msgpack fetch --all --tags --prune
    git -C %DEPENDENCY_DIR%\msgpack checkout cpp-3.0.1
    mkdir "%DEPENDENCY_DIR%\msgpack\build"
)
echo === Building msgpack === 
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\msgpack" -B"%DEPENDENCY_DIR%\msgpack\build" %COMMON_GENERATOR_FLAGS% -DMSGPACK_BUILD_EXAMPLES=OFF
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\msgpack\build" %COMMON_BUILD_FLAGS%


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
echo %CMAKE_BIN% -H"%DEPENDENCY_DIR%\fmt" -B"%DEPENDENCY_DIR%\fmt\build" -DFMT_TEST=OFF %COMMON_GENERATOR_FLAGS% 
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\fmt" -B"%DEPENDENCY_DIR%\fmt\build" %COMMON_GENERATOR_FLAGS%
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\fmt\build" %COMMON_BUILD_FLAGS%


REM json
IF EXIST %DEPENDENCY_DIR%\json\build (
    echo Found nlohmann json
) ELSE (
    echo === Cloning nlohmann json === 
    git clone https://github.com/nlohmann/json.git %DEPENDENCY_DIR%\json
    git -C %DEPENDENCY_DIR%\json fetch --all --tags --prune
    git -C %DEPENDENCY_DIR%\json checkout v3.4.0
    mkdir "%DEPENDENCY_DIR%\json\build"
)
echo === Building nlohmann json === 
%CMAKE_BIN% -H"%DEPENDENCY_DIR%\json" -B"%DEPENDENCY_DIR%\json\build" -DJSON_BuildTests=OFF %COMMON_GENERATOR_FLAGS%
%CMAKE_BIN% --build "%DEPENDENCY_DIR%\json\build" %COMMON_BUILD_FLAGS%


REM boost
set BOOST_COMMON_FLAGS=--prefix=%DEPENDENCY_DIR%\install address-model=64 variant=%CONFIG_LOWER% threading=multi runtime-link=shared
set BOOST_LIBS=--with-system --with-chrono  --with-log --with-thread --with-filesystem --with-date_time --with-atomic --with-regex --with-context --with-fiber
set BOOST_SHARED_LIB_FLAGS= link=shared
set BOOST_STATIC_LIB_FLAGS= link=static

IF %WITH_BOOST% EQU 1 (
    IF EXIST %DEPENDENCY_DIR%\boost_1_68_0 (
        echo Found boost
    ) ELSE (
        echo === Downloading boost ===
        powershell -Command "Invoke-WebRequest http://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.zip -OutFile %DEPENDENCY_DIR%\boost_1.68.0.zip"
        7z x -y -bd -bb0 -o%DEPENDENCY_DIR% %DEPENDENCY_DIR%\boost_1.68.0.zip
        mkdir %DEPENDENCY_DIR%\boost_1_68_0\build
    )
    echo === Building boost ===
    pushd %DEPENDENCY_DIR%\boost_1_68_0
    call %DEPENDENCY_DIR%\boost_1_68_0\bootstrap.bat
    call %DEPENDENCY_DIR%\boost_1_68_0\b2.exe stage %BOOST_LIBS% %BOOST_SHARED_LIB_FLAGS% %BOOST_COMMON_FLAGS%
    call %DEPENDENCY_DIR%\boost_1_68_0\b2.exe install %BOOST_LIBS% %BOOST_SHARED_LIB_FLAGS% %BOOST_COMMON_FLAGS%
    call %DEPENDENCY_DIR%\boost_1_68_0\b2.exe stage %BOOST_LIBS% %BOOST_STATIC_LIB_FLAGS% %BOOST_COMMON_FLAGS%
    call %DEPENDENCY_DIR%\boost_1_68_0\b2.exe install %BOOST_LIBS% %BOOST_STATIC_LIB_FLAGS% %BOOST_COMMON_FLAGS%
    popd
)


REM swig
set SWIG_VER=swigwin-3.0.12
IF EXIST %INSTALL_PREFIX%\swig (
    echo Found SWIG
) ELSE (
    echo === Downloading SWIG === 
    powershell -Command "Invoke-WebRequest https://phoenixnap.dl.sourceforge.net/project/swig/swigwin/swigwin-3.0.12/%SWIG_VER%.zip -OutFile %DEPENDENCY_DIR%\%SWIG_VER%.zip"
    
    echo === Unzipping swig === 
    7z x -y -bd -bb0 -o%DEPENDENCY_DIR% %DEPENDENCY_DIR%\%SWIG_VER%.zip
    echo Moving %DEPENDENCY_DIR%\%SWIG_VER% to %INSTALL_PREFIX%\swig
    move "%DEPENDENCY_DIR%\%SWIG_VER%" "%INSTALL_PREFIX%\swig"
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
