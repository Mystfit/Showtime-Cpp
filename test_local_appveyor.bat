ECHO ON

REM Set up test Appveyor environment
set APPVEYOR_BUILD_FOLDER=C:/projects/showtime-cpp
set GENERATOR=Visual Studio 15 2017 Win64
set CONFIGURATION=Debug
git clone https://github.com/mystfit/showtime-cpp.git %APPVEYOR_BUILD_FOLDER%
cd %APPVEYOR_BUILD_FOLDER%
git checkout develop
cd ..


REM APPVEYOR before_build:
REM ----------------------
REM Environment variables
set DEPENDENCY_DIR=%APPVEYOR_BUILD_FOLDER%/dependencies
set HUNTER_ROOT=%APPVEYOR_BUILD_FOLDER%/hunter_root

REM Set up dependency directories
cd %APPVEYOR_BUILD_FOLDER%
mkdir "%HUNTER_ROOT%"
mkdir "%DEPENDENCY_DIR%"
mkdir "%DEPENDENCY_DIR%/install"

REM Aquire patched hunterized CZMQ
cd "%DEPENDENCY_DIR%"
git clone https://github.com/mystfit/czmq.git
cd czmq
git checkout hunter-v4.1.0
mkdir "%DEPENDENCY_DIR%/czmq/build"
mkdir "%DEPENDENCY_DIR%/czmq/install"
cmake -H. -B"%DEPENDENCY_DIR%/czmq/build" -G "%GENERATOR%" -DCMAKE_INSTALL_PREFIX="%DEPENDENCY_DIR%/install"
cmake --build "%DEPENDENCY_DIR%/czmq/build" --config %CONFIGURATION% --target INSTALL

REM Aquire patched hunterized msgpack
cd "%DEPENDENCY_DIR%"
git clone https://github.com/mystfit/msgpack-c.git
cd msgpack-c
git checkout hunter-2.1.5
mkdir "%DEPENDENCY_DIR%/msgpack-c/build"
mkdir "%DEPENDENCY_DIR%/msgpack-c/install"
cmake -H. -B"%DEPENDENCY_DIR%/msgpack-c/build" -G "%GENERATOR%" -DCMAKE_INSTALL_PREFIX="%DEPENDENCY_DIR%/install"
cmake --build "%DEPENDENCY_DIR%/msgpack-c/build" --config %CONFIGURATION% --target INSTALL

REM APPVEYOR build_script:
REM ----------------------
cd "%APPVEYOR_BUILD_FOLDER%"
mkdir build
cd build
cmake .. -G "%GENERATOR%" -DCMAKE_PREFIX_PATH="%DEPENDENCY_DIR%/install;${CMAKE_PREFIX_PATH}"
cmake --build "%APPVEYOR_BUILD_FOLDER%/build" --config %CONFIGURATION%

REM APPVEYOR before_test:
REM ---------------------
REM Copy CZMQ dlls into build folder - manually until CMake can do this automagically
if %CONFIGURATION% == Debug set LIBCZMQ_DLL=libczmqd.dll
if %CONFIGURATION% == Release set LIBCZMQ_DLL=libczmq.dll
cmake -E copy "%DEPENDENCY_DIR%/install/bin/%LIBCZMQ_DLL%" "%APPVEYOR_BUILD_FOLDER%/build/bin/%CONFIGURATION%/"

REM APPVEYOR test_script:
REM ---------------------
cd "%APPVEYOR_BUILD_FOLDER%/build"
ctest -C Debug -V --output-on-fail