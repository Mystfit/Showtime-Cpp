param(
    [string]$build_dir="$PSScriptRoot/build",
    [string]$dependency_dir="$build_dir/dependencies",
    [string]$install_prefix="$dependency_dir/install",
    [string]$config="release",
    [string]$generator="Visual Studio 16 2019",
    [switch]$without_boost = $false,
    [string]$boost_version = "1.70.0"
)


# Setup
# -----
$env:GIT_REDIRECT_STDERR = "2>&1"

$build_flags = (
  "--config", "$config",
  "--target", "INSTALL"
)

$generator_flags = @(
    "-G", "`"$generator`""
    "-DCMAKE_INSTALL_PREFIX=`"$install_prefix`"",
    "-DCMAKE_INSTALL_INCLUDEDIR=`"$install_prefix/include`"",
    "-DCMAKE_INSTALL_LIBDIR=`"$install_prefix/lib`"",
    "-DCMAKE_INSTALL_BINDIR=`"$install_prefix/bin`"",
    "-DCMAKE_INSTALL_MESSAGE=NEVER",
    "-DCMAKE_PREFIX_PATH=`"$install_prefix`""
)

if($generator.indexOf("Visual Studio") -gt -1){
    $vs_ver = [convert]::ToInt32($generator.trim("Visual Studio").split()[0])
    if($vs_ver -ge 16){
        $generator_flags += @("-A", "x64")
    }
}

md -Force "$install_prefix"


# CMake Libraries
# ---------------
function Build-CmakeFromGit{
    Param($name, $url, $branch, $flags)

    $build_dir = "$dependency_dir/$name/build_dir"

    if (Test-Path $build_dir) {
        Write-Output "Found $name"
    } else {
        Write-Output "Cloning $name"
        git clone $url "$dependency_dir/$name"
        git -C "$dependency_dir/$name" checkout $branch
        md -Force $build_dir
    }
    Write-Output "Building $name"
    $cmake_flags = $generator_flags + $flags + @(
        "-S", "`"$dependency_dir/$name`"",
        "-B", "`"$build_dir`""
    ) 
    & "cmake" @cmake_flags
    & "cmake" $(@("--build", "`"$build_dir`"") + $build_flags)
    Write-Output "Built $name"
}

Build-CmakeFromGit -name "libzmq" -url "https://github.com/mystfit/libzmq.git" -branch "4.2.5-drafts-fixed" -flags @(
    "-DENABLE_DRAFTS=TRUE",
    "-DZMQ_BUILD_TESTS=OFF"
)
Build-CmakeFromGit -name "czmq" -url "https://github.com/mystfit/czmq.git" -branch "4.2.0" -flags @(
    "-DENABLE_DRAFTS=TRUE",
    "-DBUILD_TESTING=OFF",
    "-DLIBZMQ_FIND_USING_CMAKE_PACKAGE=ON"
)
Build-CmakeFromGit -name "flatbuffers" -url "https://github.com/google/flatbuffers.git" -branch "master" -flags @("-DFLATBUFFERS_INSTALL=ON")


# Boost
# -----
if($without_boost -ne $true){
    $boost_flags = @(
        "--prefix=$install_prefix",
        "address-model=64",
        "variant=$($config.ToLower())",
        "threading=multi",
        "runtime-link=shared"
    )
    $boost_libs = @("system","chrono","log","thread","filesystem","date_time","atomic","regex","context","fiber","test")
    $boost_libs = $boost_libs | ForEach-Object {"--with-$_"}
    $boost_shared_lib_flags = @("link=shared")
    $boost_static_lib_flags = @("link=static")
    $boost_ver_scored = $boost_version.Replace(".", "_")
    $boost_file = "$dependency_dir/boost_$boost_ver_scored.7z"
    $b2 = "$dependency_dir/boost_$boost_ver_scored/b2.exe"

    if (Test-Path $dependency_dir/boost_$boost_ver_scored) {
        Write-Output "Found Boost"
    } else {
        Write-Output "Downloading boost"
        Invoke-WebRequest "http://dl.bintray.com/boostorg/release/$boost_version/source/boost_$boost_ver_scored.7z" -OutFile "$boost_file"
        Write-Output "Extracting boost"
        Push-Location $dependency_dir
        7z x -y -bd -bb0 $boost_file
        Pop-Location
    }
    Write-Output "Building boost"
    
    Push-Location "$dependency_dir/boost_$boost_ver_scored"
    cmd.exe /c "call bootstrap.bat"
    ./b2.exe $(@("stage") + $boost_libs + $boost_shared_lib_flags + $boost_flags)
    ./b2.exe $(@("install") + $boost_libs + $boost_shared_lib_flags + $boost_flags)
    ./b2.exe $(@("stage") + $boost_libs + $boost_static_lib_flags + $boost_flags)
    ./b2.exe $(@("install") + $boost_libs + $boost_static_lib_flags + $boost_flags)
    Pop-Location
}

Write-Output "Done"
