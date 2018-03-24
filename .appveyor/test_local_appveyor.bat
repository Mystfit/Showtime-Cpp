echo off
setlocal

REM Set up test Appveyor environment
set APPVEYOR_BUILD_FOLDER=C:\projects\showtime-cpp
set HUNTER_ROOT=%APPVEYOR_BUILD_FOLDER%\dependencies\hunter_root
set GENERATOR=Visual Studio 15 2017 Win64
set CONFIGURATION=Debug

REM APPVEYOR install:
REM ----------------------
set BUILD_FOLDER=%APPVEYOR_BUILD_FOLDER%
set DEPENDENCY_DIR=%BUILD_FOLDER%\dependencies
call %BUILD_FOLDER%/.appveyor/install_dependencies.bat %BUILD_FOLDER% %CONFIGURATION%
set CMAKE_FLAGS=-DCMAKE_PREFIX_PATH="%DEPENDENCY_DIR%\install;%DEPENDENCY_DIR%\swig;${CMAKE_PREFIX_PATH}"
set PYTHON=C:\Program Files (x86)\Microsoft Visual Studio\Shared\Python36_64

REM APPVEYOR build_script:
REM ----------------------
mkdir %BUILD_FOLDER%\build
set CMAKE_FLAGS=%CMAKE_FLAGS% -DBINDINGS_DOTNET=ON -DBINDINGS_DOTNET_TESTS=ON
set CMAKE_FLAGS=%CMAKE_FLAGS% -DBINDINGS_PYTHON=ON -DBINDINGS_PYTHON_TESTS=ON -DPYTHON_EXECUTABLE=%PYTHON%/python.exe
set CMAKE_FLAGS=%CMAKE_FLAGS% -DHUNTER_STATUS_PRINT=OFF
set CMAKE_FLAGS=%CMAKE_FLAGS% -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%

REM Build core and dotnet bindings first
cmake -H"%BUILD_FOLDER%" -B"%BUILD_FOLDER%\build" -G "%GENERATOR%" %CMAKE_FLAGS%
cmake --build "%BUILD_FOLDER%\build" --target ShowtimeDotNet --config %CONFIGURATION% -- /nologo /verbosity:minimal

REM Re-run configure to pull in the generated dotnet bindings
cmake -H"%BUILD_FOLDER%" -B"%BUILD_FOLDER%\build" -G "%GENERATOR%" %CMAKE_FLAGS%
cmake --build "%BUILD_FOLDER%\build" --target ALL_BUILD --config %CONFIGURATION% -- /nologo /verbosity:minimal

REM Install showtime
cmake --build "%BUILD_FOLDER%\build" --target INSTALL --config %CONFIGURATION% -- /nologo /verbosity:minimal

REM APPVEYOR before_test:
REM ---------------------
REM Copy CZMQ dlls into build folder - manually until CMake can do this automagically
if %CONFIGURATION% == Debug set LIBCZMQ_RUNTIME=libczmqd.dll
if %CONFIGURATION% == Release set LIBCZMQ_RUNTIME=libczmq.dll
cmake -E copy "%DEPENDENCY_DIR%\install\bin\%LIBCZMQ_RUNTIME%" "%BUILD_FOLDER%\build\bin\%CONFIGURATION%"

REM APPVEYOR test_script:
REM ---------------------
set TESTCLIENT_LOG=%BUILD_FOLDER%\build\Testing\TestClient.log
ctest --build-run-dir "%BUILD_FOLDER%\build" -C %CONFIGURATION% --output-on-fail -V -O "%TESTCLIENT_LOG%" --timeout 120
set TEST_OUTCOME=Passed
if NOT %errorlevel% == 0 set TEST_OUTCOME=Failed
call "%BUILD_FOLDER%\ci\get_test_time.bat" %TESTCLIENT_LOG% TEST_DURATION
echo "Test duration %TEST_DURATION%ms"

SETLOCAL EnableDelayedExpansion
for /f "Tokens=* Delims=" %%x in (%TESTCLIENT_LOG%) do set TESTCLIENT_LOG_CONTENTS=!TESTCLIENT_LOG_CONTENTS!%%x\n

echo "Test log:"
echo "-----------------------------"
echo %TESTCLIENT_LOG_CONTENTS%
ENDLOCAL
