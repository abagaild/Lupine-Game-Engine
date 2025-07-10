#!/bin/bash

# Lupine Engine - Linux Static Library Downloader
# This script downloads and sets up all required Linux static libraries
# for cross-compilation and native Linux builds

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
THIRDPARTY_DIR="$PROJECT_ROOT/thirdparty"
LINUX_LIB_DIR="$THIRDPARTY_DIR/linux_x64-static"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Lupine Engine Linux Library Downloader ===${NC}"
echo -e "${BLUE}Setting up static libraries for Linux export system${NC}"
echo ""

# Create directories
mkdir -p "$LINUX_LIB_DIR"
cd "$LINUX_LIB_DIR"

# Function to download and extract library
download_library() {
    local name=$1
    local url=$2
    local extract_dir=$3
    
    echo -e "${YELLOW}Downloading $name...${NC}"
    
    if [ ! -d "$extract_dir" ]; then
        mkdir -p "$extract_dir"
        
        # Download
        if command -v wget >/dev/null 2>&1; then
            wget -q "$url" -O "${name}.tar.gz"
        elif command -v curl >/dev/null 2>&1; then
            curl -sL "$url" -o "${name}.tar.gz"
        else
            echo -e "${RED}Error: Neither wget nor curl found${NC}"
            exit 1
        fi
        
        # Extract
        tar -xzf "${name}.tar.gz" -C "$extract_dir" --strip-components=1
        rm "${name}.tar.gz"
        
        echo -e "${GREEN}✓ $name downloaded and extracted${NC}"
    else
        echo -e "${GREEN}✓ $name already exists${NC}"
    fi
}

# Function to build static library
build_static_library() {
    local name=$1
    local source_dir=$2
    local build_options=$3
    
    echo -e "${YELLOW}Building static library: $name...${NC}"
    
    cd "$source_dir"
    
    if [ -f "CMakeLists.txt" ]; then
        # CMake build
        mkdir -p build
        cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release \
                 -DBUILD_SHARED_LIBS=OFF \
                 -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
                 $build_options
        make -j$(nproc)
        make install DESTDIR="$LINUX_LIB_DIR/$name"
    elif [ -f "configure" ]; then
        # Autotools build
        ./configure --prefix="$LINUX_LIB_DIR/$name" \
                   --enable-static \
                   --disable-shared \
                   $build_options
        make -j$(nproc)
        make install
    elif [ -f "Makefile" ]; then
        # Direct make
        make -j$(nproc) $build_options
        make install PREFIX="$LINUX_LIB_DIR/$name"
    fi
    
    cd "$LINUX_LIB_DIR"
    echo -e "${GREEN}✓ $name built successfully${NC}"
}

echo -e "${BLUE}=== Core Graphics Libraries ===${NC}"

# SDL2
download_library "sdl2" \
    "https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-2.28.5.tar.gz" \
    "sdl2-src"
build_static_library "sdl2" "sdl2-src" "-DSDL_SHARED=OFF -DSDL_STATIC=ON"

# SDL2_image
download_library "sdl2-image" \
    "https://github.com/libsdl-org/SDL_image/releases/download/release-2.6.3/SDL2_image-2.6.3.tar.gz" \
    "sdl2-image-src"
build_static_library "sdl2-image" "sdl2-image-src" "-DSDL2IMAGE_SHARED=OFF"

# SDL2_ttf
download_library "sdl2-ttf" \
    "https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.20.2/SDL2_ttf-2.20.2.tar.gz" \
    "sdl2-ttf-src"
build_static_library "sdl2-ttf" "sdl2-ttf-src" "-DSDL2TTF_SHARED=OFF"

# SDL2_mixer
download_library "sdl2-mixer" \
    "https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.6.3/SDL2_mixer-2.6.3.tar.gz" \
    "sdl2-mixer-src"
build_static_library "sdl2-mixer" "sdl2-mixer-src" "-DSDL2MIXER_SHARED=OFF"

echo -e "${BLUE}=== 3D Graphics & Model Loading ===${NC}"

# Assimp
download_library "assimp" \
    "https://github.com/assimp/assimp/archive/v5.3.1.tar.gz" \
    "assimp-src"
build_static_library "assimp" "assimp-src" "-DASSIMP_BUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_TESTS=OFF"

# GLAD (OpenGL loader)
download_library "glad" \
    "https://github.com/Dav1dde/glad/archive/v0.1.36.tar.gz" \
    "glad-src"
build_static_library "glad" "glad-src" ""

echo -e "${BLUE}=== Physics Libraries ===${NC}"

# Box2D
download_library "box2d" \
    "https://github.com/erincatto/box2d/archive/v2.4.1.tar.gz" \
    "box2d-src"
build_static_library "box2d" "box2d-src" "-DBOX2D_BUILD_SHARED=OFF -DBOX2D_BUILD_TESTBED=OFF"

# Bullet Physics
download_library "bullet3" \
    "https://github.com/bulletphysics/bullet3/archive/3.25.tar.gz" \
    "bullet3-src"
build_static_library "bullet3" "bullet3-src" "-DBUILD_SHARED_LIBS=OFF -DBUILD_BULLET2_DEMOS=OFF -DBUILD_EXTRAS=OFF"

echo -e "${BLUE}=== Audio Libraries ===${NC}"

# libogg
download_library "libogg" \
    "https://github.com/xiph/ogg/releases/download/v1.3.5/libogg-1.3.5.tar.gz" \
    "libogg-src"
build_static_library "libogg" "libogg-src" ""

# libvorbis
download_library "libvorbis" \
    "https://github.com/xiph/vorbis/releases/download/v1.3.7/libvorbis-1.3.7.tar.gz" \
    "libvorbis-src"
build_static_library "libvorbis" "libvorbis-src" ""

# FLAC
download_library "flac" \
    "https://github.com/xiph/flac/releases/download/1.4.3/flac-1.4.3.tar.xz" \
    "flac-src"
build_static_library "flac" "flac-src" "--enable-static --disable-shared"

# Opus
download_library "opus" \
    "https://github.com/xiph/opus/releases/download/v1.4/opus-1.4.tar.gz" \
    "opus-src"
build_static_library "opus" "opus-src" "--enable-static --disable-shared"

echo -e "${BLUE}=== Scripting Libraries ===${NC}"

# Lua
download_library "lua" \
    "https://www.lua.org/ftp/lua-5.4.6.tar.gz" \
    "lua-src"
build_static_library "lua" "lua-src" "linux"

echo -e "${BLUE}=== Compression Libraries ===${NC}"

# zlib
download_library "zlib" \
    "https://github.com/madler/zlib/releases/download/v1.3/zlib-1.3.tar.gz" \
    "zlib-src"
build_static_library "zlib" "zlib-src" ""

# bzip2
download_library "bzip2" \
    "https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz" \
    "bzip2-src"
build_static_library "bzip2" "bzip2-src" ""

# lz4
download_library "lz4" \
    "https://github.com/lz4/lz4/releases/download/v1.9.4/lz4-1.9.4.tar.gz" \
    "lz4-src"
build_static_library "lz4" "lz4-src" ""

# zstd
download_library "zstd" \
    "https://github.com/facebook/zstd/releases/download/v1.5.5/zstd-1.5.5.tar.gz" \
    "zstd-src"
build_static_library "zstd" "zstd-src" ""

echo -e "${BLUE}=== Image Libraries ===${NC}"

# libpng
download_library "libpng" \
    "https://download.sourceforge.net/libpng/libpng-1.6.40.tar.gz" \
    "libpng-src"
build_static_library "libpng" "libpng-src" ""

# libjpeg-turbo
download_library "libjpeg-turbo" \
    "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.0.1/libjpeg-turbo-3.0.1.tar.gz" \
    "libjpeg-turbo-src"
build_static_library "libjpeg-turbo" "libjpeg-turbo-src" "-DENABLE_SHARED=OFF"

# FreeType
download_library "freetype" \
    "https://download.savannah.gnu.org/releases/freetype/freetype-2.13.2.tar.gz" \
    "freetype-src"
build_static_library "freetype" "freetype-src" ""

echo -e "${BLUE}=== Data Format Libraries ===${NC}"

# yaml-cpp
download_library "yaml-cpp" \
    "https://github.com/jbeder/yaml-cpp/archive/0.8.0.tar.gz" \
    "yaml-cpp-src"
build_static_library "yaml-cpp" "yaml-cpp-src" "-DYAML_BUILD_SHARED_LIBS=OFF"

# pugixml
download_library "pugixml" \
    "https://github.com/zeux/pugixml/releases/download/v1.14/pugixml-1.14.tar.gz" \
    "pugixml-src"
build_static_library "pugixml" "pugixml-src" "-DBUILD_SHARED_LIBS=OFF"

echo ""
echo -e "${GREEN}=== Linux Library Setup Complete ===${NC}"
echo -e "${GREEN}All static libraries have been downloaded and built${NC}"
echo -e "${BLUE}Libraries installed in: $LINUX_LIB_DIR${NC}"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo -e "1. Run cmake to configure the build system"
echo -e "2. Build the engine with Linux export support"
echo -e "3. Test Linux export functionality"
