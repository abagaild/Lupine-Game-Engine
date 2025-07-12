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
import urllib.error
import tarfile
import zipfile
import json
import argparse
import datetime
import concurrent.futures
import threading
import time
from pathlib import Path
from typing import Dict, List, Optional, Tuple

class BuildEnvironmentSetup:
    def __init__(self, force: bool = False, no_qt: bool = False, dev_only: bool = False):
        self.force = force
        self.no_qt = no_qt
        self.dev_only = dev_only
        # If script is in scripts/ directory, go up one level to repository root
        script_dir = Path(__file__).parent.absolute()
        if script_dir.name == "scripts":
            self.root_dir = script_dir.parent
        else:
            self.root_dir = script_dir
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
                    "arch": "x86_64",  # Use proper macOS architecture name
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

    def _run_command(self, cmd: List[str], check: bool = True, capture: bool = False, cwd: Optional[Path] = None, env: Optional[dict] = None) -> Optional[str]:
        """Run a command and optionally capture output."""
        try:
            # Convert Path objects to strings for subprocess
            str_cmd = [str(c) for c in cmd]
            working_dir = str(cwd) if cwd else None

            if capture:
                result = subprocess.run(str_cmd, check=check, capture_output=True, text=True, cwd=working_dir, env=env)
                return result.stdout.strip()
            else:
                subprocess.run(str_cmd, check=check, cwd=working_dir, env=env)
                return None
        except subprocess.CalledProcessError as e:
            if check:
                print(f"Command failed: {' '.join(str_cmd)}")
                print(f"Error: {e}")
                if e.stderr:
                    print(f"Stderr: {e.stderr}")
                sys.exit(1)
            return None
        except FileNotFoundError as e:
            if check:
                print(f"Command not found: {' '.join(str_cmd)}")
                print(f"Error: {e}")
                sys.exit(1)
            return None

    def _check_command_exists(self, command: str) -> bool:
        """Check if a command exists in PATH."""
        return shutil.which(command) is not None

    def _check_visual_studio_compiler(self) -> bool:
        """Check if Visual Studio compiler is available on Windows."""
        if self.system_info['system'] != 'windows':
            return False

        # First check if cl is directly available
        if self._check_command_exists('cl'):
            return True

        # Check common Visual Studio installation paths
        vs_paths = [
            "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC",
            "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/MSVC",
            "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC",
            "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC",
            "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/VC/Tools/MSVC",
            "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Tools/MSVC"
        ]

        for vs_path in vs_paths:
            vs_dir = Path(vs_path)
            if vs_dir.exists():
                # Look for any MSVC version directory
                for version_dir in vs_dir.iterdir():
                    if version_dir.is_dir():
                        cl_path = version_dir / "bin" / "Hostx64" / "x64" / "cl.exe"
                        if cl_path.exists():
                            print(f"Found Visual Studio compiler at: {cl_path}")
                            return True

        # Check if we can find vswhere to locate Visual Studio
        vswhere_path = Path("C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe")
        if vswhere_path.exists():
            try:
                result = self._run_command([str(vswhere_path), "-latest", "-property", "installationPath"], capture=True)
                if result:
                    vs_install_path = Path(result.strip())
                    cl_path = vs_install_path / "VC" / "Tools" / "MSVC"
                    if cl_path.exists():
                        # Look for any MSVC version
                        for version_dir in cl_path.iterdir():
                            if version_dir.is_dir():
                                cl_exe = version_dir / "bin" / "Hostx64" / "x64" / "cl.exe"
                                if cl_exe.exists():
                                    print(f"Found Visual Studio compiler via vswhere at: {cl_exe}")
                                    return True
            except:
                pass

        return False

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
                try:
                    self._run_command(['choco', 'install', 'cmake', '-y'])
                except:
                    print("Failed to install CMake via chocolatey")
            else:
                print("Please install CMake manually from https://cmake.org/download/")

        if not self._check_command_exists('git'):
            print("Installing Git...")
            if self._check_command_exists('choco'):
                try:
                    self._run_command(['choco', 'install', 'git', '-y'])
                except:
                    print("Failed to install Git via chocolatey")
            else:
                print("Please install Git manually from https://git-scm.com/download/win")

        # Install vcpkg if not present
        vcpkg_dir = self.thirdparty_dir / "vcpkg"
        if not vcpkg_dir.exists() or self.force:
            print("Setting up vcpkg...")
            if vcpkg_dir.exists():
                try:
                    shutil.rmtree(vcpkg_dir)
                except PermissionError:
                    print("Warning: Could not remove existing vcpkg directory. Continuing...")

            try:
                self._run_command(['git', 'clone', 'https://github.com/Microsoft/vcpkg.git', str(vcpkg_dir)])
                bootstrap_script = vcpkg_dir / "bootstrap-vcpkg.bat"
                if bootstrap_script.exists():
                    self._run_command([str(bootstrap_script)], cwd=vcpkg_dir)
                else:
                    print("Warning: vcpkg bootstrap script not found")
            except Exception as e:
                print(f"Warning: Failed to setup vcpkg: {e}")
                print("Continuing without vcpkg...")

        # Get triplet for this platform
        triplet = self.system_info['triplet']

        # Check if precompiled libraries are available
        # Update this URL to point to your actual GitHub repository
        precompiled_url = "https://github.com/Kurokamori/Lupine-Game-Engine/releases/download/latest"
        precompiled_archive = f"lupine-libs-{triplet}.zip"
        precompiled_path = self.thirdparty_dir / precompiled_archive

        if self._download_precompiled_libraries(precompiled_url, precompiled_archive, precompiled_path):
            print("Using precompiled libraries - setup complete!")
            return

        print("Precompiled libraries not available, building from source...")

        # Install dependencies via vcpkg with optimizations
        vcpkg_exe = vcpkg_dir / "vcpkg.exe"

        # Enable binary caching for faster builds
        self._setup_vcpkg_binary_caching(vcpkg_dir)

        # Essential dependencies for basic functionality
        essential_dependencies = [
            'sdl2', 'glm', 'nlohmann-json', 'glad', 'lua',
            'pybind11'  # For Python scripting support
        ]

        # Install static Qt separately if needed
        if not self.no_qt:
            qt_dir = self.thirdparty_dir / "Qt"
            if not self._install_static_qt_windows(qt_dir):
                print("Static Qt installation failed, trying vcpkg as fallback...")
                try:
                    self._run_command([str(vcpkg_exe), 'install', 'qtbase:' + triplet])
                    essential_dependencies.append('qtbase')
                except:
                    print("Warning: Both static and vcpkg Qt installation failed")

        # Install essential dependencies in parallel
        print("Installing essential dependencies...")
        self._install_vcpkg_packages_parallel(vcpkg_exe, essential_dependencies, triplet)

        # Optional dependencies (install in background or skip if time is critical)
        optional_dependencies = [
            'libjpeg-turbo', 'freetype', 'sqlite3', 'pugixml',
            'libogg', 'libvorbis', 'libflac', 'opus', 'mpg123',
            'bzip2', 'liblzma', 'lz4', 'zstd'
        ]

        if not self.dev_only:
            print("Installing optional dependencies...")
            self._install_vcpkg_packages_parallel(vcpkg_exe, optional_dependencies, triplet, ignore_failures=True)

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
            'libpng', 'jpeg', 'freetype', 'zlib', 'openssl',
            'glm',  # OpenGL Mathematics library
            'nlohmann-json',  # JSON library
            'pybind11'  # Python bindings
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

        # Install dependencies with error handling (excluding Qt - handled separately)
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

        # Install static Qt separately if needed
        if not self.no_qt:
            qt_dir = self.thirdparty_dir / "Qt"
            if not self._install_static_qt_macos(qt_dir):
                print("Static Qt installation failed, trying Homebrew as fallback...")
                try:
                    self._run_command(['brew', 'install', 'qt6'])
                    print("[OK] Homebrew Qt6 installed as fallback")
                except:
                    print("Warning: Both static and Homebrew Qt installation failed")

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
            'libmpg123-dev', 'libsndfile1-dev', 'libwavpack-dev',
            'nlohmann-json3-dev', 'pybind11-dev', 'libglad-dev'  # Additional libraries
        ]

        # Additional development libraries
        dev_deps = [
            'libbz2-dev', 'liblzma-dev', 'liblz4-dev', 'libzstd-dev',
            'libx11-dev', 'libxext-dev', 'libxrender-dev', 'libgl1-mesa-dev',
            'libasound2-dev', 'libpulse-dev', 'libglfw3-dev', 'libglm-dev'
        ]

        # Qt will be handled separately via static installation
        qt_deps = []

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

        # Install static Qt6 if not using --no-qt
        if not self.no_qt:
            self._install_static_qt_linux()

    def _detect_existing_qt(self) -> Optional[Path]:
        """Detect existing Qt installations on the system."""
        print("Checking for existing Qt installations...")

        # Common Qt installation paths
        qt_paths = []

        if self.system_info['system'] == 'windows':
            # Common Windows Qt installation paths
            qt_paths = [
                Path("C:/Qt"),
                Path("C:/Qt6"),
                Path("C:/Program Files/Qt"),
                Path("C:/Program Files (x86)/Qt"),
                self.thirdparty_dir / "Qt",
                Path.home() / "Qt",
                # Check for Qt in PATH
            ]
        elif self.system_info['system'] == 'macos':
            # Common macOS Qt installation paths
            qt_paths = [
                Path("/usr/local/opt/qt6"),
                Path("/opt/homebrew/opt/qt6"),
                Path("/usr/local/Qt"),
                Path("/Applications/Qt"),
                self.thirdparty_dir / "Qt",
                Path.home() / "Qt",
            ]
        elif self.system_info['system'] == 'linux':
            # Common Linux Qt installation paths
            qt_paths = [
                Path("/usr/lib/x86_64-linux-gnu/qt6"),
                Path("/usr/lib/qt6"),
                Path("/usr/share/qt6"),
                Path("/opt/qt6"),
                self.thirdparty_dir / "Qt",
                Path.home() / "Qt",
            ]

        # Check each path for Qt installation
        for qt_path in qt_paths:
            if not qt_path.exists():
                continue

            print(f"Checking Qt path: {qt_path}")

            # Look for Qt version directories or direct installation
            if self._is_valid_qt_installation(qt_path):
                print(f"Found valid Qt installation at: {qt_path}")
                return qt_path

            # Look for version subdirectories
            for item in qt_path.iterdir():
                if item.is_dir() and item.name.startswith(('6.', '5.')):
                    if self._is_valid_qt_installation(item):
                        print(f"Found valid Qt installation at: {item}")
                        return item

        print("No existing Qt installation found")
        return None

    def _is_valid_qt_installation(self, qt_path: Path) -> bool:
        """Check if a path contains a valid Qt installation."""
        # Look for key Qt files/directories
        if self.system_info['system'] == 'windows':
            # Check for Qt6Core.dll or lib files
            indicators = [
                qt_path / "bin" / "Qt6Core.dll",
                qt_path / "lib" / "Qt6Core.lib",
                qt_path / "include" / "QtCore",
                qt_path / "lib" / "cmake" / "Qt6",
            ]
        else:
            # Unix-like systems
            indicators = [
                qt_path / "lib" / "libQt6Core.so",
                qt_path / "lib" / "libQt6Core.a",
                qt_path / "lib" / "libQt6Core.dylib",
                qt_path / "include" / "QtCore",
                qt_path / "lib" / "cmake" / "Qt6",
            ]

        return any(indicator.exists() for indicator in indicators)

    def _install_python_dependencies(self):
        """Install Python packages needed for the build system."""
        print("Installing Python dependencies...")

        python_packages = [
            'pybind11', 'numpy', 'requests', 'cmake'
        ]

        # Try different Python executables
        python_executables = ['python3', 'python', 'py']
        python_cmd = None

        for py_exe in python_executables:
            if self._check_command_exists(py_exe):
                python_cmd = py_exe
                break

        if not python_cmd:
            print("Warning: No Python executable found. Skipping Python package installation.")
            return

        for package in python_packages:
            try:
                self._run_command([python_cmd, '-m', 'pip', 'install', '--user', package])
                print(f"Successfully installed {package}")
            except:
                print(f"Warning: Failed to install Python package {package}")
                # Try without --user flag
                try:
                    self._run_command([python_cmd, '-m', 'pip', 'install', package])
                    print(f"Successfully installed {package} (system-wide)")
                except:
                    print(f"Warning: Failed to install Python package {package} (both user and system-wide)")

    def _install_static_qt_linux(self):
        """Install Qt6 on Linux using system packages (more reliable than aqtinstall)."""
        print("Installing Qt6 for Linux...")

        # First check for existing Qt installations
        existing_qt = self._detect_existing_qt()
        if existing_qt and not self.force:
            print(f"Using existing Qt installation at: {existing_qt}")
            return

        # Skip aqtinstall for Linux CI environments - use system packages directly
        print("Using system Qt packages for Linux (more reliable than aqtinstall)...")

        try:
            # Try Qt6 packages first
            qt6_deps = [
                'qt6-base-dev', 'qt6-tools-dev', 'qt6-tools-dev-tools',
                'qt6-declarative-dev', 'libqt6opengl6-dev'
            ]
            print("Installing Qt6 system packages...")
            self._run_command(['sudo', 'apt', 'install', '-y'] + qt6_deps)
            print("[OK] System Qt6 packages installed")

        except Exception as qt6_e:
            print(f"[WARN] Qt6 system packages failed: {qt6_e}")
            print("Trying Qt5 as fallback...")
            try:
                qt5_deps = ['qtbase5-dev', 'qttools5-dev', 'qtdeclarative5-dev']
                self._run_command(['sudo', 'apt', 'install', '-y'] + qt5_deps)
                print("[OK] Qt5 packages installed as fallback")
            except Exception as qt5_e:
                print(f"[ERROR] All Qt installation methods failed: {qt5_e}")
                print("Continuing without Qt - some features may not work")

    def _install_static_qt_windows(self, qt_dir: Path) -> bool:
        """Install static Qt6 on Windows using aqtinstall."""
        print("Installing static Qt6 for Windows...")

        qt_version = "6.9.1"  # Use Qt 6.9.1 which is what the codebase uses
        qt_arch = "win64_msvc2022_64"  # Command line argument for aqtinstall
        qt_modules = ["qtbase", "qttools", "qtdeclarative"]

        # First check for existing Qt installations
        existing_qt = self._detect_existing_qt()
        if existing_qt and not self.force:
            # Create a symlink to the existing installation
            if not qt_dir.exists():
                qt_dir.parent.mkdir(parents=True, exist_ok=True)
                qt_dir.symlink_to(existing_qt)
            print(f"Using existing Qt installation at: {existing_qt}")
            return True

        # Check if Qt is already installed in our target location
        qt_install_dir = qt_dir / qt_version / qt_arch
        if qt_install_dir.exists() and not self.force:
            print(f"Static Qt6 already installed at {qt_install_dir}")
            return True

        try:
            # Install aqtinstall if not present
            if not shutil.which("aqt"):
                print("Installing aqtinstall...")
                self._run_command([sys.executable, "-m", "pip", "install", "--user", "aqtinstall"])
            else:
                print("aqtinstall already present")

            # Create destination directory
            qt_dir.mkdir(parents=True, exist_ok=True)

            # Download and install static Qt (base installation without extra modules)
            cmd = [
                sys.executable, "-m", "aqt", "install-qt", "windows", "desktop",
                qt_version, qt_arch,
                "--outputdir", str(qt_dir)
            ]

            print(f"Downloading static Qt {qt_version} for Windows...")
            print(f"Command: {' '.join(cmd)}")
            self._run_command(cmd)

            print(f"[OK] Static Qt {qt_version} installed in {qt_dir}")
            return True

        except Exception as e:
            print(f"[WARN] Failed to install static Qt6 for Windows: {e}")
            return False

    def _install_static_qt_macos(self, qt_dir: Path) -> bool:
        """Install static Qt6 on macOS using aqtinstall."""
        print("Installing static Qt6 for macOS...")

        qt_version = "6.9.1"  # Use Qt 6.9.1 which is what the codebase uses
        # Detect architecture
        if self.system_info['arch'] == "arm64":
            qt_arch = "clang_64"  # Apple Silicon
        else:
            qt_arch = "clang_64"  # Intel
        qt_modules = ["qtbase", "qttools", "qtdeclarative"]

        # First check for existing Qt installations
        existing_qt = self._detect_existing_qt()
        if existing_qt and not self.force:
            # Create a symlink to the existing installation
            if not qt_dir.exists():
                qt_dir.parent.mkdir(parents=True, exist_ok=True)
                qt_dir.symlink_to(existing_qt)
            print(f"Using existing Qt installation at: {existing_qt}")
            return True

        # Check if Qt is already installed in our target location
        qt_install_dir = qt_dir / qt_version / qt_arch
        if qt_install_dir.exists() and not self.force:
            print(f"Static Qt6 already installed at {qt_install_dir}")
            return True

        try:
            # Install aqtinstall if not present
            if not shutil.which("aqt"):
                print("Installing aqtinstall...")
                self._run_command([sys.executable, "-m", "pip", "install", "--user", "aqtinstall"])
            else:
                print("aqtinstall already present")

            # Create destination directory
            qt_dir.mkdir(parents=True, exist_ok=True)

            # Download and install static Qt (base installation without extra modules)
            cmd = [
                sys.executable, "-m", "aqt", "install-qt", "mac", "desktop",
                qt_version, qt_arch,
                "--outputdir", str(qt_dir)
            ]

            print(f"Downloading static Qt {qt_version} for macOS...")
            print(f"Command: {' '.join(cmd)}")
            self._run_command(cmd)

            print(f"[OK] Static Qt {qt_version} installed in {qt_dir}")
            return True

        except Exception as e:
            print(f"[WARN] Failed to install static Qt6 for macOS: {e}")
            return False

    def _download_precompiled_libraries(self, base_url: str, archive_name: str, local_path: Path) -> bool:
        """Download and extract precompiled libraries if available."""
        try:
            download_url = f"{base_url}/{archive_name}"
            print(f"Checking for precompiled libraries at {download_url}...")

            # Try to download the precompiled archive with timeout
            request = urllib.request.Request(download_url)
            request.add_header('User-Agent', 'Lupine-Engine-Setup/1.0')

            with urllib.request.urlopen(request, timeout=30) as response:
                if response.status == 200:
                    with open(local_path, 'wb') as f:
                        shutil.copyfileobj(response, f)
                else:
                    print(f"Download failed with status: {response.status}")
                    return False

            # Verify the downloaded file
            if not local_path.exists() or local_path.stat().st_size == 0:
                print("Downloaded file is empty or missing")
                return False

            # Extract to thirdparty directory
            with zipfile.ZipFile(local_path, 'r') as zip_ref:
                zip_ref.extractall(self.thirdparty_dir)

            # Clean up downloaded archive
            local_path.unlink()

            print("Precompiled libraries downloaded and extracted successfully!")
            return True

        except urllib.error.HTTPError as e:
            print(f"HTTP error downloading precompiled libraries: {e.code} {e.reason}")
            return False
        except urllib.error.URLError as e:
            print(f"URL error downloading precompiled libraries: {e.reason}")
            return False
        except Exception as e:
            print(f"Precompiled libraries not available: {e}")
            return False

    def _setup_vcpkg_binary_caching(self, vcpkg_dir: Path):
        """Set up vcpkg binary caching for faster builds."""
        try:
            # Create binary cache directory
            cache_dir = vcpkg_dir / "bincache"
            cache_dir.mkdir(exist_ok=True)

            # Set environment variable for binary caching
            import os
            os.environ['VCPKG_BINARY_SOURCES'] = f"clear;files,{cache_dir},readwrite"

            print(f"Binary caching enabled at {cache_dir}")
        except Exception as e:
            print(f"Warning: Could not set up binary caching: {e}")

    def _install_vcpkg_packages_parallel(self, vcpkg_exe: Path, packages: list, triplet: str, ignore_failures: bool = False):
        """Install vcpkg packages with parallel processing."""
        if not vcpkg_exe.exists():
            print(f"Warning: vcpkg executable not found at {vcpkg_exe}")
            return

        def install_package(package):
            try:
                cmd = [str(vcpkg_exe), 'install', f'{package}:{triplet}']
                result = self._run_command(cmd, capture=True, check=False)
                return package, True, result
            except Exception as e:
                return package, False, str(e)

        # Use thread pool for parallel installation
        max_workers = min(2, len(packages))  # Limit to 2 parallel jobs to avoid overwhelming the system

        print(f"Installing {len(packages)} packages with {max_workers} workers...")

        with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
            # Submit all package installations
            future_to_package = {executor.submit(install_package, pkg): pkg for pkg in packages}

            # Process results as they complete
            completed = 0
            for future in concurrent.futures.as_completed(future_to_package):
                package, success, result = future.result()
                completed += 1

                if success:
                    print(f"[OK] {package} installed successfully ({completed}/{len(packages)})")
                else:
                    if ignore_failures:
                        print(f"[WARN] {package} failed to install (optional): {result} ({completed}/{len(packages)})")
                    else:
                        print(f"[ERROR] {package} failed to install: {result} ({completed}/{len(packages)})")

    def setup_cross_compilation_libraries(self):
        """Set up libraries for all target platforms to enable cross-compilation."""
        print("Setting up cross-compilation libraries...")

        # Define all target platforms
        target_platforms = {
            "Windows": "x64-windows-static",
            "Linux": "x64-linux",
            "Mac-OSX": "x64-osx",
            "Mac-ARM64": "arm64-osx"
        }

        # Try to download precompiled libraries for each platform
        for platform, triplet in target_platforms.items():
            platform_dir = self.thirdparty_dir / platform
            platform_dir.mkdir(exist_ok=True)

            # Check if we already have libraries for this platform
            if self._platform_libraries_exist(platform_dir):
                print(f"[OK] {platform} libraries already available")
                continue

            # Try to download precompiled libraries
            precompiled_url = "https://github.com/Kurokamori/Lupine-Game-Engine/releases/download/latest"
            archive_name = f"lupine-libs-{triplet}.zip"
            local_path = self.thirdparty_dir / archive_name

            if self._download_precompiled_libraries(precompiled_url, archive_name, local_path):
                print(f"[OK] {platform} precompiled libraries downloaded")
            else:
                print(f"[WARN] {platform} precompiled libraries not available")
                # Could add logic here to build from source for other platforms

        # Create cross-compilation manifest
        self._create_cross_compilation_manifest()

    def _platform_libraries_exist(self, platform_dir: Path) -> bool:
        """Check if platform libraries already exist."""
        lib_dir = platform_dir / "lib"
        if not lib_dir.exists():
            return False

        # Check for essential libraries
        essential_libs = ["SDL2", "glad", "lua", "yaml-cpp"]
        for lib in essential_libs:
            lib_files = list(lib_dir.glob(f"*{lib}*"))
            if not lib_files:
                return False

        return True

    def _create_cross_compilation_manifest(self):
        """Create a manifest file describing available cross-compilation targets."""
        manifest = {
            "version": "1.0",
            "generated": str(datetime.datetime.now()),
            "platforms": {}
        }

        for platform_dir in self.thirdparty_dir.iterdir():
            if platform_dir.is_dir() and platform_dir.name in ["Windows", "Linux", "Mac-OSX", "Mac-ARM64"]:
                lib_dir = platform_dir / "lib"
                if lib_dir.exists():
                    lib_files = [f.name for f in lib_dir.iterdir() if f.is_file()]
                    manifest["platforms"][platform_dir.name] = {
                        "available": True,
                        "library_count": len(lib_files),
                        "path": str(platform_dir)
                    }
                else:
                    manifest["platforms"][platform_dir.name] = {
                        "available": False,
                        "library_count": 0,
                        "path": str(platform_dir)
                    }

        manifest_file = self.thirdparty_dir / "cross_compilation_manifest.json"
        manifest_file.write_text(json.dumps(manifest, indent=2))
        print(f"Cross-compilation manifest created: {manifest_file}")

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
        """Set up Qt on Windows using static installation."""
        # Try static Qt installation first
        if self._install_static_qt_windows(qt_dir):
            return

        # Fall back to vcpkg if static installation fails
        print("Falling back to vcpkg Qt installation...")
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
        """Set up Qt on macOS using static installation."""
        # Try static Qt installation first
        if self._install_static_qt_macos(qt_dir):
            return

        # Fall back to Homebrew if static installation fails
        print("Falling back to Homebrew Qt installation...")
        brew_qt = Path('/opt/homebrew/opt/qt6') if Path('/opt/homebrew').exists() else Path('/usr/local/opt/qt6')

        if brew_qt.exists():
            if not qt_dir.exists():
                qt_dir.symlink_to(brew_qt)
            print(f"Qt linked from Homebrew: {brew_qt}")
        else:
            print("Warning: Qt not found in Homebrew installation")

    def _setup_qt_linux(self, qt_dir: Path):
        """Set up Qt on Linux."""
        # First check if we have static Qt installed via aqtinstall
        static_qt_dir = qt_dir / "6.8.0" / "gcc_64"
        if static_qt_dir.exists():
            print(f"Using static Qt6 from: {static_qt_dir}")
            # Create a symlink for easier access
            qt_link = qt_dir / "current"
            if qt_link.exists():
                qt_link.unlink()
            qt_link.symlink_to(static_qt_dir)
            return

        # Fall back to system Qt
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

            # Fix Python command for Windows
            if self.system_info['system'] == 'windows':
                # Patch emsdk.bat to use 'py' instead of 'python'
                emsdk_bat_path = emsdk_dir / 'emsdk.bat'
                if emsdk_bat_path.exists():
                    content = emsdk_bat_path.read_text()
                    # Replace the fallback python command with py
                    content = content.replace('set EMSDK_PY=python', 'set EMSDK_PY=py')
                    emsdk_bat_path.write_text(content)
                    print("Patched emsdk.bat to use 'py' command")

                self._run_command([emsdk_cmd, 'install', 'latest'])
                self._run_command([emsdk_cmd, 'activate', 'latest'])
            else:
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
# Note: find_package calls should be done after project() declaration
# find_package(Threads REQUIRED) - moved to main CMakeLists.txt
# find_package(PkgConfig REQUIRED) - moved to main CMakeLists.txt

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

# Qt static linking setup for all platforms
set(QT_VERSION "6.9.1")

if(WIN32)
    # Windows static Qt setup
    set(QT_STATIC_DIR "${QT_DIR}/${QT_VERSION}/msvc2022_64")
    if(EXISTS "${QT_STATIC_DIR}")
        set(Qt6_DIR "${QT_STATIC_DIR}/lib/cmake/Qt6")
        list(APPEND CMAKE_PREFIX_PATH "${QT_STATIC_DIR}")
        add_compile_definitions(QT_STATIC_BUILD)
        message(STATUS "Using static Qt6 from: ${QT_STATIC_DIR}")
    else()
        # Fall back to vcpkg Qt
        set(QT_VCPKG_DIR "${PLATFORM_DIR}/qtbase_x64-windows-static")
        if(EXISTS "${QT_VCPKG_DIR}")
            set(Qt6_DIR "${QT_VCPKG_DIR}/share/Qt6")
            list(APPEND CMAKE_PREFIX_PATH "${QT_VCPKG_DIR}")
            add_compile_definitions(QT_STATIC_BUILD)
            message(STATUS "Using vcpkg Qt6 from: ${QT_VCPKG_DIR}")
        endif()
    endif()
elseif(APPLE)
    # macOS Homebrew paths setup
    if(EXISTS "/opt/homebrew")
        set(HOMEBREW_PREFIX "/opt/homebrew")
    else()
        set(HOMEBREW_PREFIX "/usr/local")
    endif()

    # Add Homebrew to CMAKE_PREFIX_PATH for package discovery
    list(APPEND CMAKE_PREFIX_PATH "${HOMEBREW_PREFIX}")

    # Add specific library paths for common Homebrew packages
    set(HOMEBREW_LIB_PATHS
        "${HOMEBREW_PREFIX}/opt/sdl2/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/sdl2_image/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/sdl2_ttf/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/sdl2_mixer/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/assimp/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/bullet/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/lua/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/yaml-cpp/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/spdlog/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/glm/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/nlohmann-json/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/pybind11/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/python@3.11/lib/cmake"
        "${HOMEBREW_PREFIX}/opt/python@3.12/lib/cmake"
    )
    list(APPEND CMAKE_PREFIX_PATH ${HOMEBREW_LIB_PATHS})

    # Add include directories for libraries that don't have proper CMake configs
    include_directories(
        "${HOMEBREW_PREFIX}/include"
        "${HOMEBREW_PREFIX}/include/SDL2"
        "${HOMEBREW_PREFIX}/include/box2d"
    )

    # Add vcpkg paths for libraries not available via Homebrew
    if(EXISTS "${THIRDPARTY_DIR}/vcpkg/installed/x64-osx")
        list(APPEND CMAKE_PREFIX_PATH "${THIRDPARTY_DIR}/vcpkg/installed/x64-osx")
        include_directories("${THIRDPARTY_DIR}/vcpkg/installed/x64-osx/include")
    endif()

    # Add Python include paths
    if(EXISTS "${HOMEBREW_PREFIX}/opt/python@3.11/include/python3.11")
        include_directories("${HOMEBREW_PREFIX}/opt/python@3.11/include/python3.11")
    elseif(EXISTS "${HOMEBREW_PREFIX}/opt/python@3.12/include/python3.12")
        include_directories("${HOMEBREW_PREFIX}/opt/python@3.12/include/python3.12")
    endif()

    # macOS static Qt setup
    set(QT_STATIC_DIR "${QT_DIR}/${QT_VERSION}/clang_64")
    if(EXISTS "${QT_STATIC_DIR}")
        set(Qt6_DIR "${QT_STATIC_DIR}/lib/cmake/Qt6")
        list(APPEND CMAKE_PREFIX_PATH "${QT_STATIC_DIR}")
        message(STATUS "Using static Qt6 from: ${QT_STATIC_DIR}")
    else()
        # Fall back to Homebrew Qt
        set(QT_BREW_DIR "${HOMEBREW_PREFIX}/opt/qt6")
        if(EXISTS "${QT_BREW_DIR}")
            list(APPEND CMAKE_PREFIX_PATH "${QT_BREW_DIR}")
            message(STATUS "Using Homebrew Qt6 from: ${QT_BREW_DIR}")
        endif()
    endif()

    message(STATUS "Using Homebrew prefix: ${HOMEBREW_PREFIX}")
    message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
elseif(UNIX AND NOT APPLE)
    # Linux static Qt setup
    set(QT_STATIC_DIR "${QT_DIR}/${QT_VERSION}/gcc_64")
    if(EXISTS "${QT_STATIC_DIR}")
        set(Qt6_DIR "${QT_STATIC_DIR}/lib/cmake/Qt6")
        list(APPEND CMAKE_PREFIX_PATH "${QT_STATIC_DIR}")
        message(STATUS "Using static Qt6 from: ${QT_STATIC_DIR}")
    else()
        # Fall back to current symlink or system Qt
        set(QT_CURRENT_DIR "${QT_DIR}/current")
        if(EXISTS "${QT_CURRENT_DIR}")
            list(APPEND CMAKE_PREFIX_PATH "${QT_CURRENT_DIR}")
            message(STATUS "Using Qt6 from: ${QT_CURRENT_DIR}")
        endif()
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
            # Use specialized Visual Studio detection for Windows
            if not self._check_visual_studio_compiler():
                missing_deps.append("Command: cl (Visual Studio compiler)")
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
            qt_dir = self.thirdparty_dir / "Qt"

            # Check for aqtinstall Qt installation first (different versions per platform)
            if self.system_info['system'] == 'windows':
                qt_version = "6.9.1"  # Windows supports 6.9.1
                # aqtinstall creates directory with different name than command line arg
                qt_arch_dir = "msvc2022_64"  # Actual directory name created
                aqt_qt_dir = qt_dir / qt_version / qt_arch_dir
                qt_found = (aqt_qt_dir / "include" / "QtCore").exists() and (aqt_qt_dir / "lib" / "Qt6Core.lib").exists()
                if qt_found:
                    print(f"Found Qt 6.9.1 installation at: {aqt_qt_dir}")

                # Fallback to vcpkg Qt
                if not qt_found:
                    vcpkg_qt = self.thirdparty_dir / "vcpkg" / "installed" / self.system_info['triplet'] / "include" / "QtCore"
                    qt_found = vcpkg_qt.exists()

            elif self.system_info['system'] == 'macos':
                qt_version = "6.9.1"  # macOS supports 6.9.1
                qt_arch = "clang_64"
                aqt_qt_dir = qt_dir / qt_version / qt_arch
                qt_found = (aqt_qt_dir / "include" / "QtCore").exists() or (aqt_qt_dir / "lib" / "libQt6Core.a").exists()

                # Fallback to Homebrew Qt
                if not qt_found:
                    brew_qt = Path('/opt/homebrew/opt/qt6') if Path('/opt/homebrew').exists() else Path('/usr/local/opt/qt6')
                    qt_found = brew_qt.exists()

            elif self.system_info['system'] == 'linux':
                qt_version = "6.8.0"  # Linux uses 6.8.0 (6.9.1 not available)
                qt_arch = "gcc_64"
                aqt_qt_dir = qt_dir / qt_version / qt_arch
                qt_found = (aqt_qt_dir / "include" / "QtCore").exists() or (aqt_qt_dir / "lib" / "libQt6Core.a").exists()

                # Fallback to system Qt
                if not qt_found:
                    qt_found = self._check_command_exists('qmake6') or self._check_command_exists('qmake')

            # Also check for existing Qt using our detection function
            if not qt_found:
                existing_qt = self._detect_existing_qt()
                qt_found = existing_qt is not None

            if not qt_found:
                missing_deps.append("Qt6 development libraries")

        if missing_deps:
            print("[ERROR] Missing dependencies:")
            for dep in missing_deps:
                print(f"  - {dep}")
            return False
        else:
            print("[OK] All dependencies verified")
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
            cmake_args.extend(['-G', 'Visual Studio 17 2022', '-A', 'x64'])
            # Add vcpkg toolchain
            vcpkg_toolchain = self.thirdparty_dir / "vcpkg" / "scripts" / "buildsystems" / "vcpkg.cmake"
            if vcpkg_toolchain.exists():
                cmake_args.extend(['-DCMAKE_TOOLCHAIN_FILE', str(vcpkg_toolchain)])
                cmake_args.extend(['-DVCPKG_TARGET_TRIPLET', self.system_info['triplet']])

        if not self.no_qt:
            cmake_args.append('-DLUPINE_ENABLE_EDITOR=ON')
        else:
            cmake_args.append('-DLUPINE_ENABLE_EDITOR=OFF')

        # Platform config will be included automatically by CMakeLists.txt
        platform_config = self.root_dir / "cmake" / "PlatformConfig.cmake"
        if platform_config.exists():
            print(f"Platform config available: {platform_config}")
        else:
            print("Platform config not found, using default configuration")

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

cmake -B build -S . -G "Visual Studio 17 2022" -A x64 ^
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
                print("\n[OK] All dependencies are available!")
            else:
                print("\n[ERROR] Some dependencies are missing. Run without --verify-only to install them.")
                sys.exit(1)
            return

        setup.install_system_dependencies()
        setup.setup_qt()
        setup.setup_emscripten()
        setup.setup_cross_compilation_libraries()  # Always set up cross-compilation
        setup.generate_cmake_config()
        setup.create_build_scripts()

        # Verify everything is working
        if setup.verify_dependencies():
            if not args.dev_only:
                setup.run_initial_build()

            print("\n[OK] Build environment setup completed successfully!")
            print(f"You can now build the project with:")
            if setup.system_info['system'] == 'windows':
                print(f"  build.bat")
            else:
                print(f"  ./build.sh")
            print(f"Or manually with:")
            print(f"  cmake --build build --config Release")
        else:
            print("\n[WARN] Setup completed but some dependencies may be missing.")
            print("Run with --verify-only to check what's missing.")

    except Exception as e:
        print(f"\nSetup failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()
