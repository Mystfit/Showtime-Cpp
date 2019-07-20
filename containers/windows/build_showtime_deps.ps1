Invoke-BatchFile "$Env:PROGRAMFILES (x86)/Microsoft Visual Studio/2019/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" "amd64"

& "$Env:SOURCE_DIR/install_dependencies.ps1" -build_dir $Env:BUILD_FOLDER
