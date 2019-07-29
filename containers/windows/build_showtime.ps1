Invoke-BatchFile "$Env:PROGRAMFILES (x86)/Microsoft Visual Studio/2019/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" "amd64"

$CMAKE_FLAGS= @(
    "-S", "$env:SOURCE_DIR",
    "-B", "$env:BUILD_FOLDER" 
    "-G", "Visual Studio 16 2019",
    "-A", "x64",
    "-DCMAKE_PREFIX_PATH=$env:BUILD_FOLDER/dependencies/install",
    "-DBUILD_SHARED=ON",
    "-DBUILD_STATIC=ON",
    "-DBUILD_DRAFTS=OFF",
    "-DBUILD_TESTING=OFF",
    "-DBINDINGS_DOTNET=ON",
    "-DBINDINGS_DOTNET_CSPROJ=ON",
    "-DBINDINGS_DOTNET_FRAMEWORK_VERSION=netstandard2.0"
    "-DADD_GENERATED_MSVC_PROJECTS=OFF"
)

cmake @CMAKE_FLAGS
cmake --build "$env:BUILD_FOLDER" --config Release
