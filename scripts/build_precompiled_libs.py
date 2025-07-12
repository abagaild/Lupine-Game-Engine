#!/usr/bin/env python3
"""
Script to build and package precompiled libraries for Lupine Engine
This creates downloadable packages for fast setup across platforms
"""

import os
import sys
import subprocess
import shutil
import zipfile
import json
import datetime
from pathlib import Path
from typing import Dict, List

class PrecompiledLibraryBuilder:
    def __init__(self):
        self.root_dir = Path(__file__).parent.parent.absolute()
        self.thirdparty_dir = self.root_dir / "thirdparty"
        self.output_dir = self.root_dir / "precompiled_packages"
        self.output_dir.mkdir(exist_ok=True)
        
        # Define target platforms and their triplets
        self.platforms = {
            "Windows": "x64-windows-static",
            "Linux": "x64-linux", 
            "Mac-OSX": "x64-osx",
            "Mac-ARM64": "arm64-osx"
        }
        
        # Bare minimum essential libraries that must be included
        self.essential_libraries = [
            "SDL2", "SDL2-image", "SDL2-mixer", "SDL2-ttf",
            "Assimp", "Box2D", "Bullet3", "Lua", "Yaml-cpp", "Pugixml",
            "glad", "spdlog", "nlohmann-json", "minizip", "libstk",
            "libvorbis", "libogg", "libflac", "mp3lame", "mpg123", "opus",
            "bzip2", "lz4", "wavpack", "sqlite3", "openssl", "glm",
            "boost-uuid", "stb", "libsndfile", "qtbase", "pybind11"
        ]

    def build_platform_package(self, platform: str, triplet: str) -> bool:
        """Build a precompiled library package for a specific platform."""
        print(f"\n=== Building {platform} package ({triplet}) ===")

        platform_dir = self.thirdparty_dir / platform
        if not platform_dir.exists():
            print(f"[WARN] Platform directory {platform_dir} does not exist")
            print(f"Creating platform directory...")
            platform_dir.mkdir(exist_ok=True)

        # Check if essential libraries exist
        lib_dir = platform_dir / "lib"
        include_dir = platform_dir / "include"

        # Ensure directories exist
        lib_dir.mkdir(exist_ok=True)
        include_dir.mkdir(exist_ok=True)

        # Debug: Show what's actually in the directories
        print(f"[DEBUG] Contents of {lib_dir}:")
        if lib_dir.exists():
            lib_files = list(lib_dir.iterdir())
            if lib_files:
                for f in lib_files[:10]:  # Show first 10 files
                    print(f"  - {f.name}")
                if len(lib_files) > 10:
                    print(f"  ... and {len(lib_files) - 10} more files")
            else:
                print("  (empty)")
        else:
            print("  (directory does not exist)")

        print(f"[DEBUG] Contents of {include_dir}:")
        if include_dir.exists():
            include_dirs = [d for d in include_dir.iterdir() if d.is_dir()]
            if include_dirs:
                for d in include_dirs[:10]:  # Show first 10 directories
                    print(f"  - {d.name}/")
                if len(include_dirs) > 10:
                    print(f"  ... and {len(include_dirs) - 10} more directories")
            else:
                print("  (empty)")
        else:
            print("  (directory does not exist)")

        # Check for essential libraries (but don't fail if some are missing)
        missing_libs = self._check_essential_libraries(lib_dir, platform)
        if missing_libs:
            print(f"[WARN] Some libraries missing for {platform}: {missing_libs}")
            print(f"[INFO] Continuing with available libraries...")

        # Create package
        package_name = f"lupine-libs-{triplet}.zip"
        package_path = self.output_dir / package_name

        print(f"Creating package: {package_path}")

        try:
            with zipfile.ZipFile(package_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as zipf:
                # Add all files from the platform directory
                files_added = 0
                for root, dirs, files in os.walk(platform_dir):
                    for file in files:
                        try:
                            file_path = Path(root) / file
                            # Calculate relative path from platform directory (not thirdparty)
                            rel_path = file_path.relative_to(platform_dir)
                            # Store with platform prefix to maintain structure
                            archive_path = Path(platform) / rel_path
                            zipf.write(file_path, archive_path)
                            files_added += 1
                        except Exception as e:
                            print(f"[WARN] Failed to add file {file_path} to archive: {e}")

                print(f"Added {files_added} files to package")

                # If no files were added, create a minimal structure
                if files_added == 0:
                    print(f"[WARN] No files found in {platform_dir}, creating minimal package structure")
                    # Add empty directories to maintain structure
                    zipf.writestr(f"{platform}/lib/.gitkeep", "")
                    zipf.writestr(f"{platform}/include/.gitkeep", "")
                    files_added = 2
        except Exception as e:
            print(f"[ERROR] Failed to create package: {e}")
            return False

        # Create package info
        info = self._create_package_info(platform, triplet, lib_dir)
        info_path = self.output_dir / f"lupine-libs-{triplet}.json"
        info_path.write_text(json.dumps(info, indent=2))

        print(f"[OK] Package created: {package_path}")
        print(f"[INFO] Package info: {info_path}")
        return True

    def _check_essential_libraries(self, lib_dir: Path, platform: str) -> List[str]:
        """Check which essential libraries are missing."""
        missing = []

        # Create a mapping of library names to possible file patterns
        lib_patterns = {
            "SDL2": ["SDL2", "sdl2"],
            "SDL2-image": ["SDL2_image", "sdl2_image", "SDL2-image"],
            "SDL2-mixer": ["SDL2_mixer", "sdl2_mixer", "SDL2-mixer"],
            "SDL2-ttf": ["SDL2_ttf", "sdl2_ttf", "SDL2-ttf"],
            "Assimp": ["assimp", "Assimp"],
            "Box2D": ["Box2D", "box2d"],
            "Bullet3": ["Bullet3", "bullet", "BulletCollision", "BulletDynamics"],
            "Lua": ["lua", "lua5", "lua54"],
            "Yaml-cpp": ["yaml-cpp", "yamlcpp"],
            "Pugixml": ["pugixml"],
            "glad": ["glad"],
            "spdlog": ["spdlog"],
            "nlohmann-json": ["nlohmann_json", "nlohmann-json"],
            "minizip": ["minizip"],
            "libstk": ["stk", "libstk"],
            "libvorbis": ["vorbis", "libvorbis"],
            "libogg": ["ogg", "libogg"],
            "libflac": ["flac", "libflac", "FLAC"],
            "mp3lame": ["mp3lame", "lame"],
            "mpg123": ["mpg123"],
            "opus": ["opus"],
            "bzip2": ["bz2", "bzip2"],
            "lz4": ["lz4"],
            "wavpack": ["wavpack"],
            "sqlite3": ["sqlite3", "sqlite"],
            "openssl": ["ssl", "crypto", "openssl"],
            "glm": ["glm"],
            "boost-uuid": ["boost_uuid", "boost-uuid"],
            "stb": ["stb"],
            "libsndfile": ["sndfile", "libsndfile"],
            "qtbase": ["Qt5Core", "Qt6Core", "qtbase"],
            "pybind11": ["pybind11"]
        }

        for lib in self.essential_libraries:
            patterns = lib_patterns.get(lib, [lib.lower()])

            found = False
            for pattern in patterns:
                # Different platforms have different library naming conventions
                if platform == "Windows":
                    search_patterns = [f"{pattern}.lib", f"lib{pattern}.lib", f"{pattern}-static.lib",
                                     f"{pattern}d.lib", f"lib{pattern}d.lib"]
                else:
                    search_patterns = [f"lib{pattern}.a", f"{pattern}.a", f"lib{pattern}-static.a",
                                     f"lib{pattern}.so", f"{pattern}.so"]

                for search_pattern in search_patterns:
                    matches = list(lib_dir.glob(f"*{search_pattern}*")) + list(lib_dir.glob(f"*{pattern}*"))
                    if matches:
                        found = True
                        print(f"[DEBUG] Found {lib}: {[m.name for m in matches[:3]]}")
                        break

                if found:
                    break

            if not found:
                missing.append(lib)

        return missing

    def _create_package_info(self, platform: str, triplet: str, lib_dir: Path) -> Dict:
        """Create package information metadata."""
        lib_files = list(lib_dir.rglob("*"))
        lib_count = len([f for f in lib_files if f.is_file()])
        total_size = sum(f.stat().st_size for f in lib_files if f.is_file())
        
        return {
            "platform": platform,
            "triplet": triplet,
            "version": "1.0.0",
            "build_date": str(datetime.datetime.now()),
            "library_count": lib_count,
            "total_size_bytes": total_size,
            "total_size_mb": round(total_size / (1024 * 1024), 2),
            "essential_libraries": self.essential_libraries,
            "description": f"Precompiled static libraries for Lupine Engine on {platform}"
        }

    def _build_libraries_for_platform(self, platform: str, triplet: str) -> bool:
        """Build libraries for a specific platform using the setup script."""
        print(f"Building libraries for {platform}...")

        # Create platform directories
        platform_dir = self.thirdparty_dir / platform
        platform_dir.mkdir(exist_ok=True)
        (platform_dir / "lib").mkdir(exist_ok=True)
        (platform_dir / "include").mkdir(exist_ok=True)

        # Run the setup script without --dev-only to build cross-platform libraries
        setup_script = self.root_dir / "scripts" / "setup_build_environment.py"
        if not setup_script.exists():
            print(f"[ERROR] Setup script not found: {setup_script}")
            return False

        try:
            # Try different Python executables (prioritize py on Windows)
            import platform
            if platform.system().lower() == 'windows':
                python_executables = [sys.executable, 'py', 'python', 'python3']
            else:
                python_executables = [sys.executable, 'python3', 'python', 'py']

            python_cmd = None
            for py_exe in python_executables:
                if shutil.which(py_exe):
                    python_cmd = py_exe
                    break

            if not python_cmd:
                print(f"[ERROR] No Python executable found")
                return False

            # Run setup script to build libraries
            cmd = [python_cmd, str(setup_script), "--force"]
            print(f"Running: {' '.join(cmd)}")
            result = subprocess.run(cmd, cwd=str(self.root_dir), capture_output=True, text=True, timeout=1800)  # 30 minute timeout

            if result.returncode != 0:
                print(f"[WARN] Setup script had issues:")
                print(f"STDOUT: {result.stdout}")
                print(f"STDERR: {result.stderr}")
                print(f"[INFO] Continuing anyway - will package available libraries")

            print(f"[OK] Setup script completed")
            return True

        except subprocess.TimeoutExpired:
            print(f"[ERROR] Setup script timed out after 30 minutes")
            return False
        except Exception as e:
            print(f"[ERROR] Failed to run setup script: {e}")
            return False

    def build_all_packages(self):
        """Build packages for all available platforms."""
        print("Building precompiled library packages for all platforms...")

        successful_builds = []
        failed_builds = []

        for platform, triplet in self.platforms.items():
            try:
                if self.build_platform_package(platform, triplet):
                    successful_builds.append(platform)
                else:
                    failed_builds.append(platform)
            except Exception as e:
                print(f"[ERROR] Error building {platform}: {e}")
                failed_builds.append(platform)

        # Create summary
        print(f"\n=== Build Summary ===")
        print(f"[OK] Successful: {len(successful_builds)} platforms")
        for platform in successful_builds:
            print(f"   - {platform}")

        if failed_builds:
            print(f"[ERROR] Failed: {len(failed_builds)} platforms")
            for platform in failed_builds:
                print(f"   - {platform}")
                
        # Create master manifest
        self._create_master_manifest(successful_builds)
        
        return len(successful_builds) > 0

    def _create_master_manifest(self, successful_platforms: List[str]):
        """Create a master manifest of all available packages."""
        manifest = {
            "version": "1.0.0",
            "generated": str(datetime.datetime.now()),
            "available_platforms": successful_platforms,
            "packages": {}
        }
        
        for platform in successful_platforms:
            triplet = self.platforms[platform]
            package_name = f"lupine-libs-{triplet}.zip"
            info_file = self.output_dir / f"lupine-libs-{triplet}.json"
            
            if info_file.exists():
                package_info = json.loads(info_file.read_text())
                manifest["packages"][platform] = {
                    "triplet": triplet,
                    "package_file": package_name,
                    "info_file": f"lupine-libs-{triplet}.json",
                    "size_mb": package_info.get("total_size_mb", 0)
                }
                
        manifest_path = self.output_dir / "manifest.json"
        manifest_path.write_text(json.dumps(manifest, indent=2))
        print(f"[INFO] Master manifest created: {manifest_path}")

    def upload_to_github_releases(self, repo: str, tag: str, token: str):
        """Upload packages to GitHub releases (requires GitHub CLI)."""
        print(f"\nUploading packages to GitHub releases...")

        # Check if GitHub CLI is available
        if not shutil.which("gh"):
            print("[ERROR] GitHub CLI (gh) not found. Please install it to upload releases.")
            return False

        # Verify we have files to upload
        zip_files = list(self.output_dir.glob("*.zip"))
        json_files = list(self.output_dir.glob("*.json"))

        if not zip_files and not json_files:
            print("[ERROR] No files found to upload")
            return False

        try:
            # Create or update release
            cmd = [
                "gh", "release", "create", tag,
                "--repo", repo,
                "--title", f"Precompiled Libraries {tag}",
                "--notes", "Precompiled static libraries for fast Lupine Engine setup"
            ]

            # Add all package files
            for file in zip_files:
                cmd.append(str(file))
            for file in json_files:
                cmd.append(str(file))

            print(f"Uploading {len(zip_files)} zip files and {len(json_files)} json files...")
            subprocess.run(cmd, check=True, env={**os.environ, "GITHUB_TOKEN": token}, timeout=600)
            print("[OK] Packages uploaded successfully!")
            return True

        except subprocess.TimeoutExpired:
            print(f"[ERROR] Upload timed out after 10 minutes")
            return False
        except subprocess.CalledProcessError as e:
            print(f"[ERROR] Failed to upload packages: {e}")
            return False

def main():
    import argparse
    import datetime
    
    parser = argparse.ArgumentParser(description="Build precompiled library packages")
    parser.add_argument("--platform", choices=["Windows", "Linux", "Mac-OSX", "Mac-ARM64"], 
                       help="Build package for specific platform only")
    parser.add_argument("--upload", action="store_true", help="Upload to GitHub releases")
    parser.add_argument("--repo", default="Kurokamori/lupine-game-engine",
                       help="GitHub repository for uploads")
    parser.add_argument("--tag", default=f"v{datetime.datetime.now().strftime('%Y%m%d')}", 
                       help="Release tag")
    parser.add_argument("--token", help="GitHub token for uploads")
    
    args = parser.parse_args()
    
    builder = PrecompiledLibraryBuilder()
    
    if args.platform:
        # Build single platform
        triplet = builder.platforms[args.platform]
        success = builder.build_platform_package(args.platform, triplet)
    else:
        # Build all platforms
        success = builder.build_all_packages()
        
    if not success:
        print("[ERROR] Build failed!")
        sys.exit(1)

    if args.upload:
        if not args.token:
            print("[ERROR] GitHub token required for upload")
            sys.exit(1)

        builder.upload_to_github_releases(args.repo, args.tag, args.token)

    print("[OK] Build completed successfully!")

if __name__ == "__main__":
    main()
