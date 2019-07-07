@echo off

REM Set CMake flags
set CMAKE_FLAGS=%CMAKE_FLAGS% -DCMAKE_PREFIX_PATH="%BUILD_FOLDER%/dependencies/install;%BUILD_FOLDER%/dependencies/swig;${CMAKE_PREFIX_PATH}"
set CMAKE_FLAGS=%CMAKE_FLAGS% -A x64 -DBUILD_SHARED=ON -DBUILD_STATIC=ON -DBUILD_DRAFTS=OFF -DBUILD_TESTING=OFF

echo Configure and make project
cmake -H"%SOURCE_DIR%" -B"%BUILD_FOLDER%" -G "Visual Studio 16 2019" %CMAKE_FLAGS%
cmake --build %BUILD_FOLDER% --config Release --target ALL_BUILD

IF NOT EXIST %OUTPUT_FOLDER%\bin\windows (
    mkdir "%OUTPUT_FOLDER%\bin\windows"
)

IF NOT EXIST %OUTPUT_FOLDER%\lib\windows (
    mkdir "%OUTPUT_FOLDER%\lib\windows"
)

echo Copying binaries to %OUTPUT_FOLDER%\bin\windows
xcopy /S /Y /E %BUILD_FOLDER%\bin %OUTPUT_FOLDER%\bin\windows

echo Copying libraries to %OUTPUT_FOLDER%\lib\windows
xcopy /S /Y /E %BUILD_FOLDER%\lib %OUTPUT_FOLDER%\lib\windows
