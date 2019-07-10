@echo off

REM Set CMake flags
set CMAKE_FLAGS=%CMAKE_FLAGS% -DCMAKE_PREFIX_PATH="%BUILD_FOLDER%/dependencies/install;%BUILD_FOLDER%/dependencies/swig;${CMAKE_PREFIX_PATH}"
set CMAKE_FLAGS=%CMAKE_FLAGS% -A x64 -DBUILD_SHARED=ON -DBUILD_STATIC=ON -DBUILD_DRAFTS=OFF -DBUILD_TESTING=OFF

echo Configure and make project
cmake -H"%SOURCE_DIR%" -B"%BUILD_FOLDER%" -G "Visual Studio 16 2019" %CMAKE_FLAGS%
cmake --build %BUILD_FOLDER% --config Release --target ALL_BUILD
