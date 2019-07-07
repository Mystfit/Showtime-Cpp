# Setup VS environment
Invoke-BatchFile "$Env:PROGRAMFILES (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\VC\\Auxiliary\\Build\\vcvarsall.bat" "amd64"

if (-not (Test-Path "$Env:BUILD_FOLDER\\dependencies")){
	mkdir $Env:BUILD_FOLDER
}

# Compile downloaded dependencies
if (-not (Test-Path "$Env:BUILD_FOLDER\\dependencies")){
  cmd.exe /c "call $Env:SOURCE_DIR\\install_dependencies.bat --build-dir $Env:BUILD_FOLDER --config Release --with-boost"
}
