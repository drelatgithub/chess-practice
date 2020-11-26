#!/bin/sh -e

if [ -z $chess_root_dir ]; then
    echo "Error: variable chess_root_dir needs to be specified"
    exit 1
fi

# Set directories
chess_build_dir="$chess_root_dir/build"
script_dir="$chess_root_dir/scripts"
build_dir="$script_dir/.build"
vcpkg_dir="$build_dir/vcpkg"

# Set variables
chess_vcpkg_cmake_toolchain="$chess_root_dir/scripts/.build/vcpkg/scripts/buildsystems/vcpkg.cmake"

if [ -n "$chess_ADDITIONAL_LINK_DIRS" ]; then
    chess_cmake_additional_link_dirs="-Dchess_ADDITIONAL_LINK_DIRS=$chess_ADDITIONAL_LINK_DIRS"
fi

if [ -n "$chess_RPATH" ]; then
    chess_cmake_rpath="-Dchess_RPATH=$chess_RPATH"
fi

if [ "$chess_BUILD_TOOL" = "Xcode" ]; then
    chess_cmake_target="Xcode"
else
    chess_cmake_target="default"
fi


# Download and setup vcpkg
vcpkg_setup() {
    required=$1
    rebuild=$2
    if [ $required = true ]; then
        if [ ! -d "$vcpkg_dir" -o $rebuild = true ]; then
            echo "Downloading vcpkg..."
            (
                cd $build_dir &&
                git clone https://github.com/Microsoft/vcpkg.git
            )
            echo "Configuring vcpkg..."
            (
                cd $vcpkg_dir
                if [ "$chess_SPECIAL_ENVIRONMENT" = "Deepthought2" ]; then
                    # Deepthought2 has old software and is not compatible with the newest version
                    git checkout --force 2019.11
                fi
                ./bootstrap-vcpkg.sh
            )
        else
            echo "vcpkg is already installed."
        fi
    else
        echo "Skipping vcpkg installation."
    fi

    return 0
}

# Setup dependencies
vcpkg_install() {
    (
        cd $vcpkg_dir && {
            ./vcpkg install catch2
        }
    )
}

# Generate using CMake
cmake_generate() {
    target=$1

    if [ "$target" = "Xcode" ]; then
        chess_cmake_generator="-G Xcode"
    fi

    echo "Creating build files..."
    (
        mkdir -p $chess_build_dir &&
        cd $chess_build_dir &&
        cmake \
            $chess_cmake_generator \
            $chess_cmake_additional_link_dirs \
            $chess_cmake_rpath \
            .. "-DCMAKE_TOOLCHAIN_FILE=$chess_vcpkg_cmake_toolchain"
    )
}

# Make directories
mkdir -p $build_dir

# Use vcpkg to resolve dependencies
vcpkg_setup true false
vcpkg_install

# Use CMake to generate build files
cmake_generate $chess_cmake_target
