param(
    [string]$build_dir="$PSScriptRoot/build",
    [string]$dependency_dir="$build_dir/dependencies",
    [string]$install_prefix="$dependency_dir/install",
    [string[]]$config=@("release"),
    [string]$generator="Visual Studio 16 2019",
    [string]$toolset="msvc-14.2",
    [switch]$without_boost = $false,
    [string]$boost_version = "1.72.0"
)

$msvc_toolset_versions = @{
    "msvc-14.0" = "v140";
    "msvc-14.1" = "v141";
    "msvc-14.2" = "v142";
}

$toolset_ver = $toolset
$install_prefix += "/$toolset" 

# Setup
# -----
$env:GIT_REDIRECT_STDERR = "2>&1"

$generator_flags = @()
if($generator.indexOf("Visual Studio") -gt -1){
    $vs_ver = [convert]::ToInt32($generator.trim("Visual Studio").split()[0])
    $toolset_ver = $msvc_toolset_versions[$toolset]
    if($vs_ver -ge 16){
        $generator_flags += @("-A", "x64")
    }
}

$generator_flags += @(
    "-G", "`"$generator`""
    "-DCMAKE_INSTALL_PREFIX=`"$install_prefix`"",
    "-DCMAKE_INSTALL_INCLUDEDIR=`"$install_prefix/include`"",
    "-DCMAKE_INSTALL_LIBDIR=`"$install_prefix/lib`"",
    "-DCMAKE_INSTALL_BINDIR=`"$install_prefix/bin`"",
    "-DCMAKE_INSTALL_MESSAGE=NEVER",
    "-DCMAKE_PREFIX_PATH=`"$install_prefix`""
)

md -Force "$install_prefix"


# CMake Libraries
# ---------------
function Build-CmakeFromGit{
    Param($name, $url, $branch, $config, $toolset, $flags)

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
        "-T", $toolset
        "-S", "`"$dependency_dir/$name`"",
        "-B", "`"$build_dir`""
    ) 
    & "cmake" @cmake_flags 2>&1 | %{ "$_" }
    & "cmake" $(@("--build", "`"$build_dir`"", "--config", "$config", "--target", "INSTALL")) 2>&1 | %{ "$_" }
    Write-Output "Built $name"
}

function Build-Boost{
    Param($version, $config, $toolset, $libraries)

    $boost_flags = @(
        "--prefix=$install_prefix",
        "address-model=64",
        "variant=$($config.ToLower())",
        "threading=multi",
        "runtime-link=shared",
        "toolset=$toolset"
    )
    $boost_libs = $libraries
    $boost_libs = $boost_libs | ForEach-Object {"--with-$_"}
    $boost_shared_lib_flags = @("link=shared")
    $boost_static_lib_flags = @("link=static")
    $boost_ver_scored = $version.Replace(".", "_")
    $boost_file = "$dependency_dir/boost_$boost_ver_scored.7z"
    $b2 = "$dependency_dir/boost_$boost_ver_scored/b2.exe"

    if (Test-Path $dependency_dir/boost_$boost_ver_scored) {
        Write-Output "Found Boost"
    } else {
        Write-Output "Downloading boost"
        Invoke-WebRequest "http://dl.bintray.com/boostorg/release/$version/source/boost_$boost_ver_scored.7z" -OutFile "$boost_file"
        Write-Output "Extracting boost"
        Push-Location $dependency_dir
        # TODO: Use platform native zip util!
        7z x -y -bd -bb0 $boost_file
        Remove-Item $boost_file
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


foreach ($c in $config){
    $config_titled = $(Get-Culture).textinfo.totitlecase($c)
    Write-Output "Building config: $config_titled"
    Build-CmakeFromGit -name "libzmq" -url "https://github.com/Mystfit/libzmq.git" -branch "master" -config $config_titled -toolset $toolset_ver -flags @(
        "-DENABLE_DRAFTS=TRUE",
        "-DZMQ_BUILD_TESTS=OFF"
    )
    Build-CmakeFromGit -name "czmq" -url "https://github.com/Mystfit/czmq.git" -branch "android_fixes" -config $config_titled -toolset $toolset_ver -flags @(
        "-DENABLE_DRAFTS=TRUE",
        "-DBUILD_TESTING=OFF",
        "-DCMAKE_DEBUG_POSTFIX=d"
    )
    Build-CmakeFromGit -name "flatbuffers" -url "https://github.com/google/flatbuffers.git" -branch "master" -config $config_titled -toolset $toolset_ver -flags @(
        "-DFLATBUFFERS_INSTALL=ON"
        "-DCMAKE_DEBUG_POSTFIX=d"
    )
    Build-CmakeFromGit -name "fmt" -url "https://github.com/fmtlib/fmt.git" -branch "master" -config $config_titled -toolset $toolset_ver -flags @()

    if($without_boost -ne $true){
        $libraries = @("system","chrono","log","thread","filesystem","date_time","atomic","regex","context","fiber","test")
        Build-Boost -version $boost_version -config $c $toolset $libraries
    }
}


Write-Output "Done"
