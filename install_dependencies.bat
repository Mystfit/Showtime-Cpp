ECHO ON
setlocal

REM ----------------------
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
IF NOT EXIST %HUNTER_ROOT% (
	mkdir "%HUNTER_ROOT%"
)

REM Aquire patched hunterized CZMQ
pushd "%DEPENDENCY_DIR%"
IF NOT EXIST %DEPENDENCY_DIR%\czmq git clone https://github.com/mystfit/czmq.git

pushd czmq
git checkout hunter-v4.1.0
mkdir "%DEPENDENCY_DIR%\czmq\build"
cmake -H. -B"%DEPENDENCY_DIR%\czmq\build" -G "%GENERATOR%" -DCMAKE_INSTALL_PREFIX="%DEPENDENCY_DIR%\install"
cmake --build "%DEPENDENCY_DIR%\czmq\build" --config %CONFIGURATION% --target INSTALL
popd

REM Aquire patched hunterized msgpack
IF NOT EXIST %DEPENDENCY_DIR%\msgpack-c (
	git clone https://github.com/mystfit/msgpack-c.git
)
pushd msgpack-c
git checkout hunter-2.1.5
mkdir "%DEPENDENCY_DIR%\msgpack-c\build"
cmake -H. -B"%DEPENDENCY_DIR%\msgpack-c\build" -G "%GENERATOR%" -DCMAKE_INSTALL_PREFIX="%DEPENDENCY_DIR%\install"
cmake --build "%DEPENDENCY_DIR%\msgpack-c\build" --config %CONFIGURATION% --target INSTALL
popd
popd
popd