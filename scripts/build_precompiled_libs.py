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
        
        # Essential libraries that must be included
        self.essential_libraries = [
            "SDL2", "SDL2_image", "SDL2_ttf", "SDL2_mixer",
            "glad", "glm", "assimp", "bullet3", "box2d",
            "lua", "yaml-cpp", "spdlog", "nlohmann-json",
            "libpng", "zlib", "openssl", "freetype"
        ]

    def build_platform_package(self, platform: str, triplet: str) -> bool:
        """Build a precompiled library package for a specific platform."""
        print(f"\n=== Building {platform} package ({triplet}) ===")
        
        platform_dir = self.thirdparty_dir / platform
        if not platform_dir.exists():
            print(f"❌ Platform directory {platform_dir} does not exist")
            return False
            
        # Check if essential libraries exist
        lib_dir = platform_dir / "lib"
        include_dir = platform_dir / "include"
        
        if not lib_dir.exists() or not include_dir.exists():
            print(f"❌ Missing lib or include directories for {platform}")
            return False
            
        # Verify essential libraries are present
        missing_libs = self._check_essential_libraries(lib_dir, platform)
        if missing_libs:
            print(f"❌ Missing essential libraries for {platform}: {missing_libs}")
            return False
            
        # Create package
        package_name = f"lupine-libs-{triplet}.zip"
        package_path = self.output_dir / package_name
        
        print(f"Creating package: {package_path}")
        
        with zipfile.ZipFile(package_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
            # Add all files from the platform directory
            for root, dirs, files in os.walk(platform_dir):
                for file in files:
                    file_path = Path(root) / file
                    # Calculate relative path from thirdparty directory
                    rel_path = file_path.relative_to(self.thirdparty_dir)
                    zipf.write(file_path, rel_path)
                    
        # Create package info
        info = self._create_package_info(platform, triplet, lib_dir)
        info_path = self.output_dir / f"lupine-libs-{triplet}.json"
        info_path.write_text(json.dumps(info, indent=2))
        
        print(f"✅ Package created: {package_path}")
        print(f"📋 Package info: {info_path}")
        return True

    def _check_essential_libraries(self, lib_dir: Path, platform: str) -> List[str]:
        """Check which essential libraries are missing."""
        missing = []
        
        for lib in self.essential_libraries:
            # Different platforms have different library naming conventions
            if platform == "Windows":
                patterns = [f"{lib}.lib", f"lib{lib}.lib", f"{lib}-static.lib"]
            else:
                patterns = [f"lib{lib}.a", f"{lib}.a", f"lib{lib}-static.a"]
                
            found = False
            for pattern in patterns:
                if list(lib_dir.glob(f"*{pattern}*")):
                    found = True
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

    def build_all_packages(self):
        """Build packages for all available platforms."""
        print("🚀 Building precompiled library packages for all platforms...")
        
        successful_builds = []
        failed_builds = []
        
        for platform, triplet in self.platforms.items():
            try:
                if self.build_platform_package(platform, triplet):
                    successful_builds.append(platform)
                else:
                    failed_builds.append(platform)
            except Exception as e:
                print(f"❌ Error building {platform}: {e}")
                failed_builds.append(platform)
                
        # Create summary
        print(f"\n=== Build Summary ===")
        print(f"✅ Successful: {len(successful_builds)} platforms")
        for platform in successful_builds:
            print(f"   - {platform}")
            
        if failed_builds:
            print(f"❌ Failed: {len(failed_builds)} platforms")
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
        print(f"📋 Master manifest created: {manifest_path}")

    def upload_to_github_releases(self, repo: str, tag: str, token: str):
        """Upload packages to GitHub releases (requires GitHub CLI)."""
        print(f"\n🚀 Uploading packages to GitHub releases...")
        
        # Check if GitHub CLI is available
        if not shutil.which("gh"):
            print("❌ GitHub CLI (gh) not found. Please install it to upload releases.")
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
            for file in self.output_dir.glob("*.zip"):
                cmd.append(str(file))
            for file in self.output_dir.glob("*.json"):
                cmd.append(str(file))
                
            subprocess.run(cmd, check=True, env={**os.environ, "GITHUB_TOKEN": token})
            print("✅ Packages uploaded successfully!")
            return True
            
        except subprocess.CalledProcessError as e:
            print(f"❌ Failed to upload packages: {e}")
            return False

def main():
    import argparse
    import datetime
    
    parser = argparse.ArgumentParser(description="Build precompiled library packages")
    parser.add_argument("--platform", choices=["Windows", "Linux", "Mac-OSX", "Mac-ARM64"], 
                       help="Build package for specific platform only")
    parser.add_argument("--upload", action="store_true", help="Upload to GitHub releases")
    parser.add_argument("--repo", default="your-org/lupine-precompiled-libs", 
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
        print("❌ Build failed!")
        sys.exit(1)
        
    if args.upload:
        if not args.token:
            print("❌ GitHub token required for upload")
            sys.exit(1)
            
        builder.upload_to_github_releases(args.repo, args.tag, args.token)
        
    print("🎉 Build completed successfully!")

if __name__ == "__main__":
    main()
