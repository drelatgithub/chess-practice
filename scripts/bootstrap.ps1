# This script might change the current directory

$ErrorActionPreference = 'Stop'

if(-NOT $chess_root_dir) {
    Write-Error "Variable chess_root_dir is not specified"
    exit 1
}

# Set directories
$chess_build_dir = "$chess_root_dir\build"
$script_dir = "$chess_root_dir\scripts"
$build_dir = "$script_dir\.build"
$vcpkg_dir = "$build_dir\vcpkg"

# Set variables
$chess_vcpkg_cmake_toolchain = "$chess_root_dir\scripts\.build\vcpkg\scripts\buildsystems\vcpkg.cmake"

if($chess_ADDITIONAL_LINK_DIRS) {
    $chess_cmake_additional_link_dirs = "-Dchess_ADDITIONAL_LINK_DIRS=$chess_ADDITIONAL_LINK_DIRS"
}

if("$chess_RPATH") {
    $chess_cmake_rpath = "-Dchess_RPATH=$chess_RPATH"
}


# Download and setup vcpkg
Function Install-Vcpkg([bool]$required, [bool]$rebuild) {

    if($required) {
        if(-NOT $(Test-Path $vcpkg_dir) -OR $rebuild) {
            Write-Host "Configuring vcpkg..."
            Set-Location $build_dir
            git clone https://github.com/Microsoft/vcpkg.git
            Set-Location $vcpkg_dir
            .\bootstrap-vcpkg.bat
        } else {
            Write-Host "vcpkg is already installed."
        }
    } else {
        Write-Host "Skipping vcpkg installation."
    }

}

# Setup dependencies
Function Install-VcpkgPackages() {

    $Env:VCPKG_DEFAULT_TRIPLET="x64-windows"

    Set-Location $vcpkg_dir
    .\vcpkg install catch2
}

# Generate using CMake
Function Use-Cmake() {

    Write-Host "Creating build files..."
    mkdir -Force $chess_build_dir
    Set-Location $chess_build_dir
    cmake `
        $chess_cmake_additional_link_dirs `
        $chess_cmake_rpath `
        .. "-DCMAKE_TOOLCHAIN_FILE=$chess_vcpkg_cmake_toolchain"
}

# Make directories
mkdir -Force $build_dir

# Use vcpkg to resolve dependencies
Install-Vcpkg $true $false
Install-VcpkgPackages

# Use CMake to generate build files
Use-Cmake
