ECHO ON

REM Set up test Appveyor environment
set APPVEYOR_BUILD_FOLDER=C:/projects/showtime-cpp
set BUILD_FOLDER=%APPVEYOR_BUILD_FOLDER%
set DEPENDENCY_DIR=%BUILD_FOLDER%/dependencies
set GENERATOR=Visual Studio 15 2017 Win64
set CONFIGURATION=Debug

REM Clone showtime repository
git clone https://github.com/mystfit/showtime-cpp.git %APPVEYOR_BUILD_FOLDER%
pushd %APPVEYOR_BUILD_FOLDER%
git checkout develop
popd

REM APPVEYOR install:
REM ----------------------
run "%APPVEYOR_BUILD_FOLDER%/install_dependencies.bat"

REM APPVEYOR build_script:
REM ----------------------
pushd %APPVEYOR_BUILD_FOLDER%
mkdir build
cmake -H. -B"%APPVEYOR_BUILD_FOLDER%/build" -G "%GENERATOR%" -DCMAKE_PREFIX_PATH="%DEPENDENCY_DIR%/install;${CMAKE_PREFIX_PATH}"
cmake --build "%APPVEYOR_BUILD_FOLDER%/build" --config %CONFIGURATION%
popd

REM APPVEYOR before_test:
REM ---------------------
REM Copy CZMQ dlls into build folder - manually until CMake can do this automagically
if %CONFIGURATION% == Debug set LIBCZMQ_DLL=libczmqd.dll
if %CONFIGURATION% == Release set LIBCZMQ_DLL=libczmq.dll
cmake -E copy "%DEPENDENCY_DIR%/install/bin/%LIBCZMQ_DLL%" "%APPVEYOR_BUILD_FOLDER%/build/bin/%CONFIGURATION%/"

REM APPVEYOR test_script:
REM ---------------------
pushd "%APPVEYOR_BUILD_FOLDER%/build"
ctest -C Debug -V --output-on-fail
popd