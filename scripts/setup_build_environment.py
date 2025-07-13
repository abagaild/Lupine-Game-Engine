#!/usr/bin/env python3
"""
Lupine Game Engine Build Environment Setup Script

This script downloads and sets up precompiled libraries for building the Lupine Game Engine.
It supports Windows, Linux, macOS x64, and macOS ARM64 platforms.
"""

import os
import sys
import platform
import argparse
import subprocess
import urllib.request
import zipfile
import tarfile
import json
import shutil
from pathlib import Path

# Configuration
GITHUB_REPO = "Kurokamori/Lupine-Game-Engine"
PRECOMPILED_TAG = "precompiled-libs-v1.0"
SCRIPT_DIR = Path(__file__).parent
PROJECT_ROOT = SCRIPT_DIR.parent
THIRDPARTY_DIR = PROJECT_ROOT / "thirdparty"

# Platform mapping
PLATFORM_MAP = {
    "Windows": "Windows",
    "Linux": "Linux", 
    "Darwin": "Mac-OSX"  # Will be updated to Mac-ARM64 if needed
}

def get_platform_info():
    """Get current platform information"""
    system = platform.system()
    machine = platform.machine().lower()
    
    platform_name = PLATFORM_MAP.get(system, system)
    
    # Handle macOS ARM64
    if system == "Darwin" and machine in ["arm64", "aarch64"]:
        platform_name = "Mac-ARM64"
    
    return {
        "system": system,
        "machine": machine,
        "platform": platform_name,
        "python_version": f"{sys.version_info.major}.{sys.version_info.minor}"
    }

def log(message, level="INFO"):
    """Log a message with timestamp"""
    import datetime
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] [{level}] {message}")

def run_command(cmd, cwd=None, check=True):
    """Run a command and return the result"""
    log(f"Running: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    try:
        result = subprocess.run(
            cmd, 
            cwd=cwd, 
            check=check, 
            capture_output=True, 
            text=True,
            shell=isinstance(cmd, str)
        )
        if result.stdout:
            log(f"Output: {result.stdout.strip()}")
        return result
    except subprocess.CalledProcessError as e:
        log(f"Command failed: {e}", "ERROR")
        if e.stdout:
            log(f"Stdout: {e.stdout}", "ERROR")
        if e.stderr:
            log(f"Stderr: {e.stderr}", "ERROR")
        if check:
            raise
        return e

def download_file(url, dest_path, chunk_size=8192):
    """Download a file with progress indication"""
    log(f"Downloading {url} to {dest_path}")
    
    try:
        with urllib.request.urlopen(url) as response:
            total_size = int(response.headers.get('Content-Length', 0))
            downloaded = 0
            
            dest_path.parent.mkdir(parents=True, exist_ok=True)
            
            with open(dest_path, 'wb') as f:
                while True:
                    chunk = response.read(chunk_size)
                    if not chunk:
                        break
                    f.write(chunk)
                    downloaded += len(chunk)
                    
                    if total_size > 0:
                        percent = (downloaded / total_size) * 100
                        print(f"\rProgress: {percent:.1f}%", end='', flush=True)
            
            print()  # New line after progress
            log(f"Downloaded {downloaded} bytes")
            return True
            
    except Exception as e:
        log(f"Download failed: {e}", "ERROR")
        return False

def extract_archive(archive_path, extract_to):
    """Extract an archive (zip or tar.gz)"""
    log(f"Extracting {archive_path} to {extract_to}")
    
    extract_to.mkdir(parents=True, exist_ok=True)
    
    try:
        if archive_path.suffix.lower() == '.zip':
            with zipfile.ZipFile(archive_path, 'r') as zip_ref:
                zip_ref.extractall(extract_to)
        elif archive_path.name.endswith('.tar.gz') or archive_path.suffix == '.tgz':
            with tarfile.open(archive_path, 'r:gz') as tar_ref:
                tar_ref.extractall(extract_to)
        else:
            log(f"Unsupported archive format: {archive_path}", "ERROR")
            return False
            
        log("Extraction completed")
        return True
        
    except Exception as e:
        log(f"Extraction failed: {e}", "ERROR")
        return False

def setup_cmake():
    """Ensure CMake is available"""
    log("Checking CMake availability...")
    
    try:
        result = run_command(["cmake", "--version"])
        log("CMake is available")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        log("CMake not found, attempting to install...", "WARN")
        
        platform_info = get_platform_info()
        
        if platform_info["system"] == "Windows":
            log("Please install CMake from https://cmake.org/download/")
            return False
        elif platform_info["system"] == "Linux":
            try:
                run_command(["sudo", "apt-get", "update"])
                run_command(["sudo", "apt-get", "install", "-y", "cmake"])
                return True
            except:
                log("Failed to install CMake via apt", "ERROR")
                return False
        elif platform_info["system"] == "Darwin":
            try:
                run_command(["brew", "install", "cmake"])
                return True
            except:
                log("Failed to install CMake via brew", "ERROR")
                return False
        
        return False

def download_precompiled_libraries(platform_name, force=False):
    """Download precompiled libraries for the specified platform"""
    platform_dir = THIRDPARTY_DIR / platform_name
    
    if platform_dir.exists() and not force:
        log(f"Platform libraries already exist at {platform_dir}")
        return True
    
    # Construct download URL
    archive_name = f"lupine-libs-{platform_name.lower()}.tar.gz"
    download_url = f"https://github.com/{GITHUB_REPO}/releases/download/{PRECOMPILED_TAG}/{archive_name}"
    
    # Download to temporary location
    temp_dir = THIRDPARTY_DIR / "temp"
    archive_path = temp_dir / archive_name
    
    log(f"Downloading precompiled libraries for {platform_name}")
    
    if not download_file(download_url, archive_path):
        log(f"Failed to download precompiled libraries for {platform_name}", "WARN")
        log("Will attempt to use system libraries instead", "WARN")
        return False
    
    # Extract libraries
    if not extract_archive(archive_path, platform_dir):
        log(f"Failed to extract libraries for {platform_name}", "ERROR")
        return False
    
    # Cleanup
    if archive_path.exists():
        archive_path.unlink()
    
    log(f"Precompiled libraries installed to {platform_dir}")
    return True

def setup_build_directory():
    """Set up the build directory"""
    build_dir = PROJECT_ROOT / "build"

    log(f"Setting up build directory at {build_dir}")
    build_dir.mkdir(exist_ok=True)

    return build_dir

def configure_cmake(build_dir, platform_info, additional_args=None):
    """Configure CMake for the project"""
    log("Configuring CMake...")

    cmake_args = [
        "cmake",
        "-B", str(build_dir),
        "-S", str(PROJECT_ROOT),
        "-DCMAKE_BUILD_TYPE=Release",
        "-DLUPINE_ENABLE_EDITOR=ON",
        "-DLUPINE_ENABLE_RUNTIME=ON",
        "-DLUPINE_STATIC_LINKING=ON"
    ]

    # Platform-specific configuration
    if platform_info["system"] == "Windows":
        cmake_args.extend([
            "-G", "Visual Studio 17 2022",
            "-A", "x64"
        ])
    elif platform_info["system"] == "Darwin":
        if "ARM64" in platform_info["platform"]:
            cmake_args.append("-DCMAKE_OSX_ARCHITECTURES=arm64")
        else:
            cmake_args.append("-DCMAKE_OSX_ARCHITECTURES=x86_64")
        cmake_args.append("-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15")

    # Add any additional arguments
    if additional_args:
        cmake_args.extend(additional_args)

    try:
        run_command(cmake_args, cwd=PROJECT_ROOT)
        log("CMake configuration completed successfully")
        return True
    except subprocess.CalledProcessError:
        log("CMake configuration failed", "ERROR")
        return False

def build_project(build_dir):
    """Build the project"""
    log("Building project...")

    try:
        run_command([
            "cmake",
            "--build", str(build_dir),
            "--config", "Release",
            "--parallel"
        ], cwd=PROJECT_ROOT)

        log("Build completed successfully")
        return True

    except subprocess.CalledProcessError:
        log("Build failed", "ERROR")
        return False

def verify_build(build_dir, platform_info):
    """Verify that the build outputs exist"""
    log("Verifying build outputs...")

    bin_dir = build_dir / "bin"
    exe_ext = ".exe" if platform_info["system"] == "Windows" else ""

    expected_files = [
        bin_dir / f"lupine-runtime{exe_ext}",
        bin_dir / f"lupine-editor{exe_ext}"
    ]

    all_exist = True
    for file_path in expected_files:
        if file_path.exists():
            log(f"✓ Found: {file_path}")
        else:
            log(f"✗ Missing: {file_path}", "ERROR")
            all_exist = False

    if all_exist:
        log("All expected build outputs found")
    else:
        log("Some build outputs are missing", "ERROR")

    return all_exist

def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description="Setup Lupine Game Engine build environment")
    parser.add_argument("--force", action="store_true", help="Force re-download of libraries")
    parser.add_argument("--dev-only", action="store_true", help="Setup for development only (no build)")
    parser.add_argument("--verify-only", action="store_true", help="Only verify existing setup")
    parser.add_argument("--build-only", action="store_true", help="Only build, skip setup")
    parser.add_argument("--platform", help="Override platform detection")

    args = parser.parse_args()

    # Get platform information
    platform_info = get_platform_info()
    if args.platform:
        platform_info["platform"] = args.platform

    log(f"Setting up build environment for {platform_info['platform']}")
    log(f"Python version: {platform_info['python_version']}")
    log(f"Project root: {PROJECT_ROOT}")

    success = True

    if args.verify_only:
        build_dir = PROJECT_ROOT / "build"
        if build_dir.exists():
            success = verify_build(build_dir, platform_info)
        else:
            log("Build directory does not exist", "ERROR")
            success = False
    elif args.build_only:
        build_dir = setup_build_directory()
        success = build_project(build_dir)
        if success:
            success = verify_build(build_dir, platform_info)
    else:
        # Full setup
        if not setup_cmake():
            log("CMake setup failed", "ERROR")
            success = False

        if success:
            # Download precompiled libraries
            download_precompiled_libraries(platform_info["platform"], args.force)

        if success and not args.dev_only:
            # Configure and build
            build_dir = setup_build_directory()

            if configure_cmake(build_dir, platform_info):
                if build_project(build_dir):
                    success = verify_build(build_dir, platform_info)
                else:
                    success = False
            else:
                success = False

    if success:
        log("Setup completed successfully!")
        return 0
    else:
        log("Setup failed!", "ERROR")
        return 1

if __name__ == "__main__":
    sys.exit(main())
