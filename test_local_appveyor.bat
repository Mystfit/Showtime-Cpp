@echo off

REM Set up test Appveyor environment
set APPVEYOR_BUILD_FOLDER=C:\projects\showtime-cpp
REM set HUNTER_ROOT=C:\.hunter
set GENERATOR=Visual Studio 15 2017 Win64
set CONFIGURATION=Debug

REM Clone showtime repository
git clone https://github.com/mystfit/showtime-cpp.git %APPVEYOR_BUILD_FOLDER%
pushd %APPVEYOR_BUILD_FOLDER%
git checkout develop
popd

REM APPVEYOR install:
REM ----------------------
set BUILD_FOLDER=%APPVEYOR_BUILD_FOLDER%
set DEPENDENCY_DIR=%BUILD_FOLDER%\dependencies
call "%APPVEYOR_BUILD_FOLDER%\install_dependencies.bat"

REM APPVEYOR build_script:
REM ----------------------
pushd %APPVEYOR_BUILD_FOLDER%
mkdir build
cmake -H. -B"%APPVEYOR_BUILD_FOLDER%\build" -G "%GENERATOR%" -DCMAKE_PREFIX_PATH="%DEPENDENCY_DIR%\install;${CMAKE_PREFIX_PATH}"
cmake --build "%APPVEYOR_BUILD_FOLDER%\build" --config %CONFIGURATION%
popd

REM APPVEYOR before_test:
REM ---------------------
REM Copy CZMQ dlls into build folder - manually until CMake can do this automagically
if %CONFIGURATION% == Debug set LIBCZMQ_RUNTIME=libczmqd.dll
if %CONFIGURATION% == Release set LIBCZMQ_RUNTIME=libczmq.dll
cmake -E copy "%DEPENDENCY_DIR%\install\bin\%LIBCZMQ_RUNTIME%" "%APPVEYOR_BUILD_FOLDER%\build\bin\%CONFIGURATION%"

REM APPVEYOR test_script:
REM ---------------------
pushd "%BUILD_FOLDER%\build"
set TESTCLIENT_LOG=%BUILD_FOLDER%\build\Testing\TestClient.log
ctest -C %CONFIGURATION% --output-on-fail -V -O "%TESTCLIENT_LOG%"
set TEST_OUTCOME=Passed
if NOT %errorlevel% == 0 set TEST_OUTCOME=Failed
call "%BUILD_FOLDER%\ci\get_test_time.bat" %TESTCLIENT_LOG% TEST_DURATION
echo "Test duration %TEST_DURATION%ms"
