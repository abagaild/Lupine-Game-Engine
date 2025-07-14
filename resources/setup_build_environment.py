#!/usr/bin/env python3
"""
Lupine Engine Build Environment Setup Script

This script automatically sets up the build environment for Lupine Engine
across Windows, macOS, and Linux platforms. It handles:
- System detection
- Dependency installation via platform package managers
- Toolchain setup
- CMake configuration for static linking
- Qt installation and configuration

Usage:
    python setup_build_environment.py [--force] [--no-qt] [--dev-only]
    
Options:
    --force     Force reinstallation of dependencies
    --no-qt     Skip Qt installation (runtime-only build)
    --dev-only  Install only development dependencies
"""

import os
import sys
import platform
import subprocess
import shutil
import urllib.request
import tarfile
import zipfile
import json
import argparse
from pathlib import Path
from typing import Dict, List, Optional, Tuple

class BuildEnvironmentSetup:
    def __init__(self, force: bool = False, no_qt: bool = False, dev_only: bool = False):
        self.force = force
        self.no_qt = no_qt
        self.dev_only = dev_only
        self.root_dir = Path(__file__).parent.absolute()
        self.thirdparty_dir = self.root_dir / "thirdparty"
        self.system_info = self._detect_system()
        
        # Create thirdparty directory structure
        self.thirdparty_dir.mkdir(exist_ok=True)
        (self.thirdparty_dir / self.system_info['platform_dir']).mkdir(exist_ok=True)
        
    def _detect_system(self) -> Dict[str, str]:
        """Detect the current system and return platform information."""
        system = platform.system().lower()
        machine = platform.machine().lower()
        
        if system == "windows":
            return {
                "system": "windows",
                "platform_dir": "Windows",
                "arch": "x64",
                "triplet": "x64-windows-static",
                "package_manager": "vcpkg",
                "lib_ext": ".lib",
                "exe_ext": ".exe"
            }
        elif system == "darwin":
            if machine in ["arm64", "aarch64"]:
                return {
                    "system": "macos",
                    "platform_dir": "Mac-ARM64", 
                    "arch": "arm64",
                    "triplet": "arm64-osx",
                    "package_manager": "brew",
                    "lib_ext": ".a",
                    "exe_ext": ""
                }
            else:
                return {
                    "system": "macos",
                    "platform_dir": "Mac-OSX",
                    "arch": "x64", 
                    "triplet": "x64-osx",
                    "package_manager": "brew",
                    "lib_ext": ".a",
                    "exe_ext": ""
                }
        elif system == "linux":
            return {
                "system": "linux",
                "platform_dir": "Linux",
                "arch": "x64",
                "triplet": "x64-linux",
                "package_manager": "apt",
                "lib_ext": ".a", 
                "exe_ext": ""
            }
        else:
            raise RuntimeError(f"Unsupported system: {system}")

    def _run_command(self, cmd: List[str], check: bool = True, capture: bool = False) -> Optional[str]:
        """Run a command and optionally capture output."""
        try:
            if capture:
                result = subprocess.run(cmd, check=check, capture_output=True, text=True)
                return result.stdout.strip()
            else:
                subprocess.run(cmd, check=check)
                return None
        except subprocess.CalledProcessError as e:
            if check:
                print(f"Command failed: {' '.join(cmd)}")
                print(f"Error: {e}")
                sys.exit(1)
            return None

    def _check_command_exists(self, command: str) -> bool:
        """Check if a command exists in PATH."""
        return shutil.which(command) is not None

    def install_system_dependencies(self):
        """Install system dependencies using the appropriate package manager."""
        print(f"Installing system dependencies for {self.system_info['system']}...")
        
        if self.system_info['system'] == 'windows':
            self._install_windows_dependencies()
        elif self.system_info['system'] == 'macos':
            self._install_macos_dependencies()
        elif self.system_info['system'] == 'linux':
            self._install_linux_dependencies()

    def _install_windows_dependencies(self):
        """Install Windows dependencies using vcpkg and chocolatey."""
        # Check for required tools
        if not self._check_command_exists('cmake'):
            print("Installing CMake...")
            if self._check_command_exists('choco'):
                self._run_command(['choco', 'install', 'cmake', '-y'])
            else:
                print("Please install CMake manually from https://cmake.org/download/")
                
        if not self._check_command_exists('git'):
            print("Installing Git...")
            if self._check_command_exists('choco'):
                self._run_command(['choco', 'install', 'git', '-y'])
            else:
                print("Please install Git manually from https://git-scm.com/download/win")

        # Install vcpkg if not present
        vcpkg_dir = self.thirdparty_dir / "vcpkg"
        if not vcpkg_dir.exists() or self.force:
            print("Setting up vcpkg...")
            if vcpkg_dir.exists():
                shutil.rmtree(vcpkg_dir)
            self._run_command(['git', 'clone', 'https://github.com/Microsoft/vcpkg.git', str(vcpkg_dir)])
            self._run_command([str(vcpkg_dir / "bootstrap-vcpkg.bat")])

        # Install dependencies via vcpkg
        vcpkg_exe = vcpkg_dir / "vcpkg.exe"
        triplet = self.system_info['triplet']

        # Core dependencies
        core_dependencies = [
            'sdl2', 'sdl2-image', 'sdl2-ttf', 'sdl2-mixer',
            'glad', 'glm', 'assimp', 'bullet3', 'box2d',
            'lua', 'yaml-cpp', 'spdlog', 'nlohmann-json',
            'libpng', 'libjpeg-turbo', 'freetype', 'zlib',
            'openssl', 'sqlite3', 'pugixml'
        ]

        # Audio dependencies
        audio_dependencies = [
            'libogg', 'libvorbis', 'libflac', 'opus',
            'mpg123', 'libsndfile', 'wavpack'
        ]

        # Compression dependencies
        compression_dependencies = [
            'bzip2', 'liblzma', 'lz4', 'zstd', 'brotli'
        ]

        # Python dependencies
        python_dependencies = ['python3', 'pybind11']

        all_dependencies = core_dependencies + audio_dependencies + compression_dependencies + python_dependencies

        if not self.no_qt:
            all_dependencies.extend(['qtbase'])

        # Install dependencies in batches to handle potential failures
        failed_deps = []
        for dep in all_dependencies:
            print(f"Installing {dep}...")
            try:
                self._run_command([str(vcpkg_exe), 'install', f'{dep}:{triplet}'])
            except:
                print(f"Warning: Failed to install {dep}, continuing...")
                failed_deps.append(dep)

        if failed_deps:
            print(f"Warning: The following dependencies failed to install: {', '.join(failed_deps)}")
            print("You may need to install these manually or the build may fail.")

        # Integrate vcpkg with Visual Studio
        print("Integrating vcpkg with Visual Studio...")
        self._run_command([str(vcpkg_exe), 'integrate', 'install'], check=False)

    def _install_macos_dependencies(self):
        """Install macOS dependencies using Homebrew."""
        # Check for Homebrew
        if not self._check_command_exists('brew'):
            print("Installing Homebrew...")
            install_script = urllib.request.urlopen(
                'https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh'
            ).read().decode('utf-8')
            subprocess.run(['bash', '-c', install_script])

        # Install Xcode Command Line Tools
        if not Path('/usr/bin/clang').exists():
            print("Installing Xcode Command Line Tools...")
            self._run_command(['xcode-select', '--install'], check=False)
            print("Please complete Xcode Command Line Tools installation and re-run this script.")
            sys.exit(1)

        # Core dependencies
        core_dependencies = [
            'cmake', 'pkg-config', 'python3', 'ninja',
            'sdl2', 'sdl2_image', 'sdl2_ttf', 'sdl2_mixer',
            'assimp', 'bullet', 'lua', 'yaml-cpp', 'spdlog',
            'libpng', 'jpeg', 'freetype', 'zlib', 'openssl'
        ]

        # Audio dependencies
        audio_dependencies = [
            'libogg', 'libvorbis', 'flac', 'opus', 'mpg123', 'libsndfile', 'wavpack'
        ]

        # Additional dependencies
        additional_dependencies = [
            'bzip2', 'xz', 'lz4', 'zstd', 'glfw'
        ]

        all_dependencies = core_dependencies + audio_dependencies + additional_dependencies

        if not self.no_qt:
            all_dependencies.append('qt6')

        # Install dependencies with error handling
        failed_deps = []
        for dep in all_dependencies:
            print(f"Installing {dep}...")
            try:
                self._run_command(['brew', 'install', dep])
            except:
                print(f"Warning: Failed to install {dep}, continuing...")
                failed_deps.append(dep)

        if failed_deps:
            print(f"Warning: The following dependencies failed to install: {', '.join(failed_deps)}")

        # Install Python packages
        self._install_python_dependencies()

    def _install_linux_dependencies(self):
        """Install Linux dependencies using apt (Ubuntu/Debian)."""
        # Update package list
        print("Updating package list...")
        self._run_command(['sudo', 'apt', 'update'])

        # Install build tools
        build_deps = [
            'build-essential', 'cmake', 'pkg-config', 'git',
            'python3-dev', 'python3-pip', 'ninja-build'
        ]

        # Install runtime dependencies
        runtime_deps = [
            'libsdl2-dev', 'libsdl2-image-dev', 'libsdl2-ttf-dev', 'libsdl2-mixer-dev',
            'libassimp-dev', 'libbullet-dev', 'liblua5.4-dev', 'libyaml-cpp-dev',
            'libspdlog-dev', 'libpng-dev', 'libjpeg-dev', 'libfreetype-dev',
            'zlib1g-dev', 'libssl-dev', 'libsqlite3-dev', 'libpugixml-dev',
            'libogg-dev', 'libvorbis-dev', 'libflac-dev', 'libopus-dev',
            'libmpg123-dev', 'libsndfile1-dev', 'libwavpack-dev'
        ]

        # Additional development libraries
        dev_deps = [
            'libbz2-dev', 'liblzma-dev', 'liblz4-dev', 'libzstd-dev',
            'libx11-dev', 'libxext-dev', 'libxrender-dev', 'libgl1-mesa-dev',
            'libasound2-dev', 'libpulse-dev', 'libglfw3-dev'
        ]

        qt_deps = []
        if not self.no_qt:
            qt_deps = [
                'qt6-base-dev', 'qt6-tools-dev', 'qt6-tools-dev-tools',
                'qt6-opengl-dev', 'libqt6opengl6-dev'
            ]

        all_deps = build_deps + runtime_deps + dev_deps + qt_deps

        print("Installing dependencies...")
        # Install in chunks to handle potential failures
        chunk_size = 10
        for i in range(0, len(all_deps), chunk_size):
            chunk = all_deps[i:i + chunk_size]
            try:
                self._run_command(['sudo', 'apt', 'install', '-y'] + chunk)
            except:
                print(f"Warning: Failed to install some packages in chunk: {chunk}")
                # Try installing individually
                for dep in chunk:
                    try:
                        self._run_command(['sudo', 'apt', 'install', '-y', dep])
                    except:
                        print(f"Warning: Failed to install {dep}")

        # Install Python packages
        self._install_python_dependencies()

    def _install_python_dependencies(self):
        """Install Python packages needed for the build system."""
        print("Installing Python dependencies...")

        python_packages = [
            'pybind11', 'numpy', 'requests', 'cmake'
        ]

        for package in python_packages:
            try:
                self._run_command(['python3', '-m', 'pip', 'install', '--user', package])
            except:
                print(f"Warning: Failed to install Python package {package}")

    def setup_qt(self):
        """Set up Qt for the current platform."""
        if self.no_qt:
            print("Skipping Qt setup (--no-qt specified)")
            return
            
        print("Setting up Qt...")
        qt_dir = self.thirdparty_dir / "Qt"
        
        if self.system_info['system'] == 'windows':
            self._setup_qt_windows(qt_dir)
        elif self.system_info['system'] == 'macos':
            self._setup_qt_macos(qt_dir)
        elif self.system_info['system'] == 'linux':
            self._setup_qt_linux(qt_dir)

    def _setup_qt_windows(self, qt_dir: Path):
        """Set up Qt on Windows."""
        # Qt should be installed via vcpkg, create symlink structure
        vcpkg_dir = self.thirdparty_dir / "vcpkg"
        vcpkg_qt = vcpkg_dir / "installed" / self.system_info['triplet']
        
        if vcpkg_qt.exists():
            if not qt_dir.exists():
                qt_dir.mkdir(parents=True)
            
            # Create expected directory structure
            (qt_dir / "bin").mkdir(exist_ok=True)
            (qt_dir / "lib").mkdir(exist_ok=True)
            (qt_dir / "include").mkdir(exist_ok=True)
            (qt_dir / "share").mkdir(exist_ok=True)
            
            print("Qt installed via vcpkg")
        else:
            print("Warning: Qt not found in vcpkg installation")

    def _setup_qt_macos(self, qt_dir: Path):
        """Set up Qt on macOS."""
        # Qt should be installed via Homebrew
        brew_qt = Path('/opt/homebrew/opt/qt6') if Path('/opt/homebrew').exists() else Path('/usr/local/opt/qt6')
        
        if brew_qt.exists():
            if not qt_dir.exists():
                qt_dir.symlink_to(brew_qt)
            print(f"Qt linked from Homebrew: {brew_qt}")
        else:
            print("Warning: Qt not found in Homebrew installation")

    def _setup_qt_linux(self, qt_dir: Path):
        """Set up Qt on Linux."""
        # Qt should be installed via apt, find system Qt
        qt_paths = ['/usr/lib/x86_64-linux-gnu/qt6', '/usr/lib/qt6', '/usr/share/qt6']
        
        for qt_path in qt_paths:
            if Path(qt_path).exists():
                if not qt_dir.exists():
                    qt_dir.symlink_to(qt_path)
                print(f"Qt linked from system: {qt_path}")
                return
                
        print("Warning: Qt not found in system installation")

    def setup_emscripten(self):
        """Set up Emscripten SDK for web exports."""
        if self.dev_only:
            print("Skipping Emscripten setup (--dev-only specified)")
            return
            
        print("Setting up Emscripten SDK...")
        emsdk_dir = self.thirdparty_dir / "emsdk"
        
        if not emsdk_dir.exists() or self.force:
            if emsdk_dir.exists():
                shutil.rmtree(emsdk_dir)
                
            print("Cloning Emscripten SDK...")
            self._run_command(['git', 'clone', 'https://github.com/emscripten-core/emsdk.git', str(emsdk_dir)])
            
            # Install and activate latest Emscripten
            emsdk_cmd = str(emsdk_dir / ('emsdk.bat' if self.system_info['system'] == 'windows' else 'emsdk'))
            self._run_command([emsdk_cmd, 'install', 'latest'])
            self._run_command([emsdk_cmd, 'activate', 'latest'])
            
            print("Emscripten SDK installed successfully")
        else:
            print("Emscripten SDK already exists")

    def generate_cmake_config(self):
        """Generate CMake configuration file with proper paths."""
        print("Generating CMake configuration...")
        
        config_content = f"""# Auto-generated CMake configuration for Lupine Engine
# Generated by setup_build_environment.py

# Platform detection
set(LUPINE_PLATFORM "{self.system_info['system'].upper()}")
set(LUPINE_ARCH "{self.system_info['arch']}")
set(LUPINE_TRIPLET "{self.system_info['triplet']}")

# Third-party paths
set(THIRDPARTY_DIR "${{CMAKE_CURRENT_SOURCE_DIR}}/thirdparty")
set(PLATFORM_DIR "${{THIRDPARTY_DIR}}/{self.system_info['platform_dir']}")

# Enable static linking
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES "{self.system_info['lib_ext']}")

# Compiler flags for static linking
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/Zc:__cplusplus /permissive- /W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Platform-specific configurations
"""

        if self.system_info['system'] == 'windows':
            config_content += """
# Windows-specific settings
set(VCPKG_DIR "${THIRDPARTY_DIR}/vcpkg")
if(EXISTS "${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")
    set(VCPKG_TARGET_TRIPLET "${LUPINE_TRIPLET}")
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Windows system libraries
set(WINDOWS_SYSTEM_LIBS
    user32 gdi32 shell32 advapi32 kernel32 ws2_32 opengl32
    dwmapi uxtheme winmm imm32 wtsapi32 setupapi version
    netapi32 userenv crypt32 secur32 bcrypt
)
"""
        elif self.system_info['system'] == 'macos':
            config_content += f"""
# macOS-specific settings
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
set(CMAKE_OSX_ARCHITECTURES "{self.system_info['arch']}")

# macOS frameworks
find_library(COCOA_FRAMEWORK Cocoa)
find_library(IOKIT_FRAMEWORK IOKit)
find_library(OPENGL_FRAMEWORK OpenGL)
find_library(COREAUDIO_FRAMEWORK CoreAudio)
find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
find_library(CARBON_FRAMEWORK Carbon)
find_library(SECURITY_FRAMEWORK Security)
find_library(CORESERVICES_FRAMEWORK CoreServices)

set(MACOS_SYSTEM_FRAMEWORKS
    ${{COCOA_FRAMEWORK}} ${{IOKIT_FRAMEWORK}} ${{OPENGL_FRAMEWORK}}
    ${{COREAUDIO_FRAMEWORK}} ${{AUDIOTOOLBOX_FRAMEWORK}} ${{CARBON_FRAMEWORK}}
    ${{SECURITY_FRAMEWORK}} ${{CORESERVICES_FRAMEWORK}}
)
"""
        elif self.system_info['system'] == 'linux':
            config_content += """
# Linux-specific settings
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Linux system libraries
set(LINUX_SYSTEM_LIBS
    pthread dl X11 Xext Xrender GL asound pulse m
)
"""

        if not self.no_qt:
            config_content += """
# Qt configuration
set(QT_DIR "${THIRDPARTY_DIR}/Qt")
list(APPEND CMAKE_PREFIX_PATH "${QT_DIR}")

# Qt static linking setup
if(WIN32)
    set(QT_STATIC_DIR "${PLATFORM_DIR}/qtbase_x64-windows-static")
    if(EXISTS "${QT_STATIC_DIR}")
        set(Qt6_DIR "${QT_STATIC_DIR}/share/Qt6")
        list(APPEND CMAKE_PREFIX_PATH "${QT_STATIC_DIR}")
        add_compile_definitions(QT_STATIC_BUILD)
    endif()
endif()
"""

        config_content += """
# Emscripten configuration
set(EMSDK_DIR "${THIRDPARTY_DIR}/emsdk")
if(EXISTS "${EMSDK_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "${EMSDK_DIR}")
endif()

# Library search paths
list(APPEND CMAKE_PREFIX_PATH "${PLATFORM_DIR}")

# Add all platform-specific package directories
if(EXISTS "${PLATFORM_DIR}")
    file(GLOB PLATFORM_PACKAGES "${PLATFORM_DIR}/*")
    foreach(package IN LISTS PLATFORM_PACKAGES)
        if(IS_DIRECTORY "${package}")
            list(APPEND CMAKE_PREFIX_PATH "${package}")
        endif()
    endforeach()
endif()

# Export functionality
option(LUPINE_ENABLE_EXPORT "Enable cross-platform export functionality" ON)
if(LUPINE_ENABLE_EXPORT)
    add_compile_definitions(
        LUPINE_EMBED_WINDOWS_LIBRARIES
        LUPINE_EMBED_LINUX_LIBRARIES
        LUPINE_EMBED_MACOS_LIBRARIES
        LUPINE_EMBED_ARM64_LIBRARIES
    )
endif()

# Debug information
message(STATUS "Lupine Platform: ${LUPINE_PLATFORM}")
message(STATUS "Lupine Architecture: ${LUPINE_ARCH}")
message(STATUS "Platform Directory: ${PLATFORM_DIR}")
message(STATUS "Third-party Directory: ${THIRDPARTY_DIR}")
"""

        config_file = self.root_dir / "cmake" / "PlatformConfig.cmake"
        config_file.parent.mkdir(exist_ok=True)
        config_file.write_text(config_content)
        
        print(f"CMake configuration written to {config_file}")

    def verify_dependencies(self) -> bool:
        """Verify that all required dependencies are available."""
        print("Verifying dependencies...")

        missing_deps = []

        # Check basic build tools
        required_tools = ['cmake', 'git']
        if self.system_info['system'] == 'windows':
            required_tools.append('cl')  # MSVC compiler
        else:
            required_tools.extend(['gcc', 'g++'])

        for tool in required_tools:
            if not self._check_command_exists(tool):
                missing_deps.append(f"Command: {tool}")

        # Check platform-specific dependencies
        if self.system_info['system'] == 'windows':
            vcpkg_dir = self.thirdparty_dir / "vcpkg"
            if not (vcpkg_dir / "vcpkg.exe").exists():
                missing_deps.append("vcpkg not installed")
        elif self.system_info['system'] == 'macos':
            if not self._check_command_exists('brew'):
                missing_deps.append("Homebrew not installed")
        elif self.system_info['system'] == 'linux':
            if not self._check_command_exists('apt'):
                missing_deps.append("apt package manager not available")

        # Check Qt if required
        if not self.no_qt:
            qt_found = False
            if self.system_info['system'] == 'windows':
                vcpkg_qt = self.thirdparty_dir / "vcpkg" / "installed" / self.system_info['triplet'] / "include" / "QtCore"
                qt_found = vcpkg_qt.exists()
            elif self.system_info['system'] == 'macos':
                brew_qt = Path('/opt/homebrew/opt/qt6') if Path('/opt/homebrew').exists() else Path('/usr/local/opt/qt6')
                qt_found = brew_qt.exists()
            elif self.system_info['system'] == 'linux':
                qt_found = self._check_command_exists('qmake6') or self._check_command_exists('qmake')

            if not qt_found:
                missing_deps.append("Qt6 development libraries")

        if missing_deps:
            print("❌ Missing dependencies:")
            for dep in missing_deps:
                print(f"  - {dep}")
            return False
        else:
            print("✅ All dependencies verified")
            return True

    def setup_cross_platform_libraries(self):
        """Set up libraries for cross-platform export."""
        if self.dev_only:
            print("Skipping cross-platform library setup (--dev-only specified)")
            return

        print("Setting up cross-platform export libraries...")

        # Create platform directories
        platforms = ["Windows", "Linux", "Mac-OSX", "Mac-ARM64"]
        for platform in platforms:
            platform_dir = self.thirdparty_dir / platform
            platform_dir.mkdir(exist_ok=True)
            (platform_dir / "lib").mkdir(exist_ok=True)
            (platform_dir / "include").mkdir(exist_ok=True)

        # Download or copy libraries for other platforms
        self._setup_export_libraries()

    def _setup_export_libraries(self):
        """Download and set up export libraries for all platforms."""
        # This would typically download precompiled static libraries
        # For now, we'll create placeholder structure

        library_manifest = {
            "Windows": {
                "required_libs": [
                    "SDL2.lib", "SDL2main.lib", "SDL2_image.lib", "SDL2_ttf.lib", "SDL2_mixer.lib",
                    "glad.lib", "assimp.lib", "BulletDynamics.lib", "BulletCollision.lib", "LinearMath.lib",
                    "box2d.lib", "lua.lib", "yaml-cpp.lib", "spdlog.lib", "libpng.lib", "zlib.lib"
                ]
            },
            "Linux": {
                "required_libs": [
                    "libSDL2.a", "libSDL2main.a", "libSDL2_image.a", "libSDL2_ttf.a", "libSDL2_mixer.a",
                    "libglad.a", "libassimp.a", "libBulletDynamics.a", "libBulletCollision.a", "libLinearMath.a",
                    "libbox2d.a", "liblua.a", "libyaml-cpp.a", "libspdlog.a", "libpng.a", "libz.a"
                ]
            },
            "Mac-OSX": {
                "required_libs": [
                    "libSDL2.a", "libSDL2main.a", "libSDL2_image.a", "libSDL2_ttf.a", "libSDL2_mixer.a",
                    "libglad.a", "libassimp.a", "libBulletDynamics.a", "libBulletCollision.a", "libLinearMath.a",
                    "libbox2d.a", "liblua.a", "libyaml-cpp.a", "libspdlog.a", "libpng.a", "libz.a"
                ]
            },
            "Mac-ARM64": {
                "required_libs": [
                    "libSDL2.a", "libSDL2main.a", "libSDL2_image.a", "libSDL2_ttf.a", "libSDL2_mixer.a",
                    "libglad.a", "libassimp.a", "libBulletDynamics.a", "libBulletCollision.a", "libLinearMath.a",
                    "libbox2d.a", "liblua.a", "libyaml-cpp.a", "libspdlog.a", "libpng.a", "libz.a"
                ]
            }
        }

        # Save manifest for reference
        manifest_file = self.thirdparty_dir / "library_manifest.json"
        manifest_file.write_text(json.dumps(library_manifest, indent=2))
        print(f"Library manifest saved to {manifest_file}")

    def run_initial_build(self):
        """Run an initial build to verify everything works."""
        print("Running initial build test...")

        build_dir = self.root_dir / "build"
        if build_dir.exists() and self.force:
            shutil.rmtree(build_dir)

        # Configure
        cmake_args = [
            'cmake', '-B', str(build_dir), '-S', str(self.root_dir),
            '-DCMAKE_BUILD_TYPE=Release'
        ]

        if self.system_info['system'] == 'windows':
            cmake_args.extend(['-G', 'Visual Studio 16 2019', '-A', 'x64'])
            # Add vcpkg toolchain
            vcpkg_toolchain = self.thirdparty_dir / "vcpkg" / "scripts" / "buildsystems" / "vcpkg.cmake"
            if vcpkg_toolchain.exists():
                cmake_args.extend(['-DCMAKE_TOOLCHAIN_FILE', str(vcpkg_toolchain)])
                cmake_args.extend(['-DVCPKG_TARGET_TRIPLET', self.system_info['triplet']])

        if not self.no_qt:
            cmake_args.append('-DLUPINE_ENABLE_EDITOR=ON')
        else:
            cmake_args.append('-DLUPINE_ENABLE_EDITOR=OFF')

        # Add platform config
        platform_config = self.root_dir / "cmake" / "PlatformConfig.cmake"
        if platform_config.exists():
            cmake_args.extend(['-C', str(platform_config)])

        self._run_command(cmake_args)

        # Build
        self._run_command(['cmake', '--build', str(build_dir), '--config', 'Release'])

        print("Initial build completed successfully!")

    def create_build_scripts(self):
        """Create convenient build scripts for the platform."""
        print("Creating build scripts...")

        if self.system_info['system'] == 'windows':
            script_content = f"""@echo off
REM Auto-generated build script for Lupine Engine
echo Building Lupine Engine for Windows...

cmake -B build -S . -G "Visual Studio 16 2019" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=thirdparty/vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET={self.system_info['triplet']} ^
    -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

cmake --build build --config Release

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b 1
)

echo Build completed successfully!
echo Executables are in build/bin/
"""
            script_file = self.root_dir / "build.bat"
            script_file.write_text(script_content)

        else:
            script_content = f"""#!/bin/bash
# Auto-generated build script for Lupine Engine
echo "Building Lupine Engine for {self.system_info['system']}..."

cmake -B build -S . \\
    -DCMAKE_BUILD_TYPE=Release \\
    -DLUPINE_PLATFORM={self.system_info['system'].upper()}

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

cmake --build build --config Release

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build completed successfully!"
echo "Executables are in build/bin/"
"""
            script_file = self.root_dir / "build.sh"
            script_file.write_text(script_content)
            script_file.chmod(0o755)  # Make executable

        print(f"Build script created: {script_file}")

def main():
    parser = argparse.ArgumentParser(description='Set up Lupine Engine build environment')
    parser.add_argument('--force', action='store_true', help='Force reinstallation of dependencies')
    parser.add_argument('--no-qt', action='store_true', help='Skip Qt installation (runtime-only build)')
    parser.add_argument('--dev-only', action='store_true', help='Install only development dependencies')
    parser.add_argument('--verify-only', action='store_true', help='Only verify dependencies without installing')

    args = parser.parse_args()

    setup = BuildEnvironmentSetup(force=args.force, no_qt=args.no_qt, dev_only=args.dev_only)

    print(f"Setting up build environment for {setup.system_info['system']} ({setup.system_info['arch']})")
    print(f"Platform directory: {setup.system_info['platform_dir']}")
    print(f"Package manager: {setup.system_info['package_manager']}")

    try:
        if args.verify_only:
            if setup.verify_dependencies():
                print("\n✅ All dependencies are available!")
            else:
                print("\n❌ Some dependencies are missing. Run without --verify-only to install them.")
                sys.exit(1)
            return

        setup.install_system_dependencies()
        setup.setup_qt()
        setup.setup_emscripten()
        setup.setup_cross_platform_libraries()
        setup.generate_cmake_config()
        setup.create_build_scripts()

        # Verify everything is working
        if setup.verify_dependencies():
            if not args.dev_only:
                setup.run_initial_build()

            print("\n✅ Build environment setup completed successfully!")
            print(f"You can now build the project with:")
            if setup.system_info['system'] == 'windows':
                print(f"  build.bat")
            else:
                print(f"  ./build.sh")
            print(f"Or manually with:")
            print(f"  cmake --build build --config Release")
        else:
            print("\n⚠️  Setup completed but some dependencies may be missing.")
            print("Run with --verify-only to check what's missing.")

    except Exception as e:
        print(f"\n❌ Setup failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()
