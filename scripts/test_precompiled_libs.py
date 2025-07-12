#!/usr/bin/env python3
"""
Test script to verify precompiled library packages
"""

import os
import sys
import zipfile
import json
from pathlib import Path

def test_package(package_path: Path) -> bool:
    """Test a precompiled library package."""
    print(f"Testing package: {package_path}")
    
    if not package_path.exists():
        print(f"[ERROR] Package not found: {package_path}")
        return False
    
    if package_path.stat().st_size == 0:
        print(f"[ERROR] Package is empty: {package_path}")
        return False
    
    try:
        with zipfile.ZipFile(package_path, 'r') as zipf:
            file_list = zipf.namelist()
            print(f"[INFO] Package contains {len(file_list)} files")
            
            # Check for basic structure
            has_lib = any('lib/' in f for f in file_list)
            has_include = any('include/' in f for f in file_list)
            
            if not has_lib:
                print(f"[WARN] No lib/ directory found in package")
            if not has_include:
                print(f"[WARN] No include/ directory found in package")
            
            # Show some sample files
            print(f"[INFO] Sample files:")
            for f in file_list[:10]:
                print(f"  - {f}")
            if len(file_list) > 10:
                print(f"  ... and {len(file_list) - 10} more files")
            
            return len(file_list) > 0
            
    except Exception as e:
        print(f"[ERROR] Failed to read package: {e}")
        return False

def main():
    """Test all precompiled library packages."""
    root_dir = Path(__file__).parent.parent
    packages_dir = root_dir / "precompiled_packages"
    
    if not packages_dir.exists():
        print(f"[ERROR] Packages directory not found: {packages_dir}")
        sys.exit(1)
    
    # Find all zip packages
    packages = list(packages_dir.glob("lupine-libs-*.zip"))
    
    if not packages:
        print(f"[ERROR] No packages found in {packages_dir}")
        sys.exit(1)
    
    print(f"Found {len(packages)} packages to test")
    
    success_count = 0
    for package in packages:
        if test_package(package):
            success_count += 1
            print(f"[OK] {package.name} passed tests")
        else:
            print(f"[FAIL] {package.name} failed tests")
        print()
    
    print(f"Test Results: {success_count}/{len(packages)} packages passed")
    
    if success_count == len(packages):
        print("[OK] All packages passed tests!")
        sys.exit(0)
    else:
        print("[ERROR] Some packages failed tests!")
        sys.exit(1)

if __name__ == "__main__":
    main()
