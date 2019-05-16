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

    IF "%ARG%" EQU "--platform" (
        set PLATFORM=%2
        SHIFT
    )

    IF "%ARG%" EQU "--toolchain" (
        set TOOLCHAIN_CMAKE=%2
        SHIFT
    )

    IF "%ARG%" EQU "--generator" (
        set GENERATOR=%2
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

    SHIFT
    GOTO processargs
)

REM Expand variables supplied from :processargs
set BUILD_FOLDER=!BUILD_FOLDER!
set PLATFORM=!PLATFORM!
set CONFIGURATION=!CONFIGURATION!
set GENERATOR=!GENERATOR!
set TOOLCHAIN_CMAKE=!TOOLCHAIN_CMAKE!

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

IF NOT DEFINED PLATFORM (
    set PLATFORM=win64
)
echo Platform=!PLATFORM!

IF NOT DEFINED TOOLCHAIN_CMAKE (
    set TOOLCHAIN_CMAKE=0
)

IF NOT DEFINED GENERATOR (
    IF %PLATFORM% EQU android (
        set GENERATOR="Ninja"
    ) ELSE (
        set GENERATOR="Visual Studio 15 2017 Win64"
    )
)

set DEPENDENCY_DIR=%BUILD_FOLDER%\dependencies
IF NOT EXIST %DEPENDENCY_DIR% (
    mkdir "%DEPENDENCY_DIR%"
    mkdir "%DEPENDENCY_DIR%\install"
)


REM Set common build flags and prefixes for czmq and msgpack  
set INSTALL_PREFIX=%DEPENDENCY_DIR%\install
set COMMON_GENERATOR_FLAGS=-DCMAKE_INSTALL_MESSAGE=NEVER -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" -DCMAKE_PREFIX_PATH="%INSTALL_PREFIX%"
set COMMON_BUILD_FLAGS=--config %CONFIGURATION% --target install

IF DEFINED GENERATOR (
    set COMMON_GENERATOR_FLAGS=!COMMON_GENERATOR_FLAGS! -G !GENERATOR!
)

IF %PLATFORM% EQU android (
    echo %TOOLCHAIN_CMAKE%
    IF !TOOLCHAIN_CMAKE! EQU 0 (
        echo Platform set to android but no toolchain cmake file was provided with the --toolchain flag
        exit /B 1
    )
    REM C:\Users\mystfit\AppData\Local\Android\Sdk\ndk-bundle\build\cmake\android.toolchain.cmake
    set COMMON_GENERATOR_FLAGS=!COMMON_GENERATOR_FLAGS! -DCMAKE_TOOLCHAIN_FILE=%TOOLCHAIN_CMAKE% -DANDROID_PLATFORM=28
)


REM libZMQ
IF EXIST %DEPENDENCY_DIR%\install\include\zmq.h (
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
cmake -H"%DEPENDENCY_DIR%\libzmq" -B"%DEPENDENCY_DIR%\libzmq\build" %COMMON_GENERATOR_FLAGS% -DENABLE_DRAFTS=TRUE -DZMQ_BUILD_TESTS=OFF
cmake --build "%DEPENDENCY_DIR%\libzmq\build" %COMMON_BUILD_FLAGS%

REM CZMQ
IF EXIST %DEPENDENCY_DIR%\install\include\czmq.h (
    echo Found CZMQ
) ELSE (
    echo === Cloning CZMQ === 
    git clone https://github.com/mystfit/czmq.git %DEPENDENCY_DIR%\czmq
    git -C %DEPENDENCY_DIR%\czmq checkout 4.2.0
    mkdir "%DEPENDENCY_DIR%\czmq\build"
)
echo  === Building czmq === 
cmake -H"%DEPENDENCY_DIR%\czmq" -B"%DEPENDENCY_DIR%\czmq\build" %COMMON_GENERATOR_FLAGS% -DENABLE_DRAFTS=TRUE -DBUILD_TESTING=OFF -DLIBZMQ_FIND_USING_CMAKE_PACKAGE=ON -DZeroMQ_DIR=%INSTALL_PREFIX%\share\cmake\ZeroMQ
cmake --build "%DEPENDENCY_DIR%\czmq\build" %COMMON_BUILD_FLAGS%


REM msgpack-c
IF EXIST %DEPENDENCY_DIR%\install\include\msgpack.hpp (
    echo Found MsgPack-C
) ELSE (
    echo === Cloning msgpack-c === 
    git clone https://github.com/msgpack/msgpack-c.git %DEPENDENCY_DIR%\msgpack-c
    git -C %DEPENDENCY_DIR%\msgpack-c fetch --all --tags --prune
    git -C %DEPENDENCY_DIR%\msgpack-c checkout cpp-3.0.1
    mkdir "%DEPENDENCY_DIR%\msgpack-c\build"
)
echo === Building msgpack === 
cmake -H"%DEPENDENCY_DIR%\msgpack-c" -B"%DEPENDENCY_DIR%\msgpack-c\build" %COMMON_GENERATOR_FLAGS% -DMSGPACK_BUILD_EXAMPLES=OFF
cmake --build "%DEPENDENCY_DIR%\msgpack-c\build" %COMMON_BUILD_FLAGS%


REM boost
set BOOST_COMMON_FLAGS=--prefix=%DEPENDENCY_DIR%\install address-model=64 variant=%CONFIG_LOWER% threading=multi runtime-link=shared
set BOOST_LIBS=--with-system --with-chrono  --with-log --with-thread --with-filesystem --with-date_time --with-atomic --with-regex --with-context --with-fiber
set BOOST_SHARED_LIB_FLAGS= link=shared
set BOOST_STATIC_LIB_FLAGS= link=static

IF %WITH_BOOST% EQU 1 (
    IF %PLATFORM% EQU android (
        echo === Downloading boost for android ===
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest https://github.com/dec1/Boost-for-Android/releases/download/ndk_19c_boost_1.69.0/boost_1.69.0_prebuilt_ndk_19c.zip -OutFile %DEPENDENCY_DIR%\boost_1.69.0_prebuilt_ndk_19c.zip"
        7z x -y -bd -bb0 -aoa %DEPENDENCY_DIR%\boost_1.69.0_prebuilt_ndk_19c.zip -o%DEPENDENCY_DIR%
        echo Copying boost to %DEPENDENCY_DIR%\install...
        xcopy /E /q %DEPENDENCY_DIR%\boost_1.69.0_prebuilt_ndk_19 %DEPENDENCY_DIR%\install
        echo Cleaning up boost files...
        rmdir /S /q %DEPENDENCY_DIR%\boost_1.69.0_prebuilt_ndk_19
        del %DEPENDENCY_DIR%\boost_1.69.0_prebuilt_ndk_19c.zip
    ) ELSE (
        IF EXIST %DEPENDENCY_DIR%\boost_1_69_0 (
            echo Found boost
        ) ELSE (
            echo === Downloading boost ===
            powershell -Command "Invoke-WebRequest http://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.zip -OutFile %DEPENDENCY_DIR%\boost_1.69.0.zip"
            7z x -y -bd -bb0 -o%DEPENDENCY_DIR% %DEPENDENCY_DIR%\boost_1.69.0.zip
        )
        echo === Building boost ===
        pushd %DEPENDENCY_DIR%\boost_1_69_0
        call %DEPENDENCY_DIR%\boost_1_69_0\bootstrap.bat
        call %DEPENDENCY_DIR%\boost_1_69_0\b2.exe stage %BOOST_LIBS% %BOOST_SHARED_LIB_FLAGS% %BOOST_COMMON_FLAGS%
        call %DEPENDENCY_DIR%\boost_1_69_0\b2.exe install %BOOST_LIBS% %BOOST_SHARED_LIB_FLAGS% %BOOST_COMMON_FLAGS%
        call %DEPENDENCY_DIR%\boost_1_69_0\b2.exe stage %BOOST_LIBS% %BOOST_STATIC_LIB_FLAGS% %BOOST_COMMON_FLAGS%
        call %DEPENDENCY_DIR%\boost_1_69_0\b2.exe install %BOOST_LIBS% %BOOST_STATIC_LIB_FLAGS% %BOOST_COMMON_FLAGS%
        popd
    )
)
