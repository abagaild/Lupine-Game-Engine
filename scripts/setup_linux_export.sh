#!/bin/bash

# Lupine Engine - Linux Export Setup Script
# This script sets up the complete Linux export system including:
# - Downloading and building Linux static libraries
# - Generating embedded library headers
# - Configuring the build system

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Lupine Engine Linux Export Setup ===${NC}"
echo -e "${BLUE}Setting up comprehensive Linux export system${NC}"
echo ""

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check system requirements
check_requirements() {
    echo -e "${YELLOW}Checking system requirements...${NC}"
    
    local missing_tools=()
    
    # Check for Python 3
    if ! command_exists python3; then
        missing_tools+=("python3")
    fi
    
    # Check for build tools
    if ! command_exists make; then
        missing_tools+=("make")
    fi
    
    if ! command_exists cmake; then
        missing_tools+=("cmake")
    fi
    
    # Check for download tools
    if ! command_exists wget && ! command_exists curl; then
        missing_tools+=("wget or curl")
    fi
    
    # Check for compression tools
    if ! command_exists tar; then
        missing_tools+=("tar")
    fi
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        echo -e "${RED}Missing required tools:${NC}"
        for tool in "${missing_tools[@]}"; do
            echo -e "  - $tool"
        done
        echo ""
        echo -e "${YELLOW}Please install the missing tools and run this script again.${NC}"
        
        # Provide installation instructions for common systems
        echo -e "${BLUE}Installation instructions:${NC}"
        echo -e "${BLUE}Ubuntu/Debian:${NC} sudo apt install python3 build-essential cmake wget tar"
        echo -e "${BLUE}Fedora:${NC} sudo dnf install python3 gcc-c++ cmake wget tar"
        echo -e "${BLUE}macOS:${NC} brew install python cmake wget"
        echo -e "${BLUE}Windows (WSL):${NC} sudo apt install python3 build-essential cmake wget tar"
        
        return 1
    fi
    
    echo -e "${GREEN}✓ All required tools are available${NC}"
    return 0
}

# Function to download Linux libraries
download_linux_libraries() {
    echo -e "${YELLOW}Downloading and building Linux static libraries...${NC}"
    
    local download_script="$SCRIPT_DIR/download_linux_libraries.sh"
    
    if [ ! -f "$download_script" ]; then
        echo -e "${RED}Download script not found: $download_script${NC}"
        return 1
    fi
    
    # Make script executable
    chmod +x "$download_script"
    
    # Run the download script
    if "$download_script"; then
        echo -e "${GREEN}✓ Linux libraries downloaded and built successfully${NC}"
        return 0
    else
        echo -e "${RED}✗ Failed to download Linux libraries${NC}"
        return 1
    fi
}

# Function to generate embedded library headers
generate_embedded_headers() {
    echo -e "${YELLOW}Generating embedded library headers...${NC}"
    
    local generator_script="$SCRIPT_DIR/generate_embedded_libraries.py"
    
    if [ ! -f "$generator_script" ]; then
        echo -e "${RED}Generator script not found: $generator_script${NC}"
        return 1
    fi
    
    # Run the generator script
    cd "$PROJECT_ROOT"
    
    if python3 "$generator_script" --thirdparty-dir thirdparty --output-dir src/export --force; then
        echo -e "${GREEN}✓ Embedded library headers generated successfully${NC}"
        return 0
    else
        echo -e "${RED}✗ Failed to generate embedded library headers${NC}"
        return 1
    fi
}

# Function to verify setup
verify_setup() {
    echo -e "${YELLOW}Verifying Linux export setup...${NC}"
    
    local success=true
    
    # Check if Linux libraries directory exists
    if [ -d "$PROJECT_ROOT/thirdparty/linux_x64-static" ]; then
        local lib_count=$(find "$PROJECT_ROOT/thirdparty/linux_x64-static" -name "*.a" | wc -l)
        echo -e "${GREEN}✓ Linux static libraries directory exists ($lib_count libraries)${NC}"
    else
        echo -e "${RED}✗ Linux static libraries directory not found${NC}"
        success=false
    fi
    
    # Check if embedded headers exist
    if [ -f "$PROJECT_ROOT/src/export/embedded_linux_libraries.h" ]; then
        echo -e "${GREEN}✓ Embedded library headers exist${NC}"
    else
        echo -e "${RED}✗ Embedded library headers not found${NC}"
        success=false
    fi
    
    # Check CMakeLists.txt configuration
    if grep -q "LUPINE_EMBED_LINUX_LIBRARIES" "$PROJECT_ROOT/CMakeLists.txt"; then
        echo -e "${GREEN}✓ CMakeLists.txt configured for embedded libraries${NC}"
    else
        echo -e "${RED}✗ CMakeLists.txt not configured for embedded libraries${NC}"
        success=false
    fi
    
    if $success; then
        echo -e "${GREEN}✓ Linux export setup verification passed${NC}"
        return 0
    else
        echo -e "${RED}✗ Linux export setup verification failed${NC}"
        return 1
    fi
}

# Function to show usage instructions
show_usage_instructions() {
    echo ""
    echo -e "${BLUE}=== Linux Export Setup Complete ===${NC}"
    echo ""
    echo -e "${GREEN}Your Lupine Engine is now configured for Linux export!${NC}"
    echo ""
    echo -e "${YELLOW}Next steps:${NC}"
    echo -e "1. Build the engine:"
    echo -e "   ${BLUE}mkdir -p build && cd build${NC}"
    echo -e "   ${BLUE}cmake ..${NC}"
    echo -e "   ${BLUE}make lupine-editor${NC}"
    echo ""
    echo -e "2. Use Linux export in the editor:"
    echo -e "   - Open your project in the Lupine Editor"
    echo -e "   - Go to File → Export Project"
    echo -e "   - Select 'Linux x64 (Multi-format)' as target"
    echo -e "   - Choose package formats (AppImage, Deb, RPM, etc.)"
    echo -e "   - Click Export"
    echo ""
    echo -e "3. Or use command line export:"
    echo -e "   ${BLUE}./lupine-cli export --target linux --project MyGame.lupine --output ./exports/${NC}"
    echo ""
    echo -e "${YELLOW}Supported Linux package formats:${NC}"
    echo -e "  - AppImage (universal, no installation required)"
    echo -e "  - Deb (Debian/Ubuntu packages)"
    echo -e "  - RPM (Red Hat/Fedora packages)"
    echo -e "  - Snap (Ubuntu universal packages)"
    echo -e "  - Flatpak (sandboxed applications)"
    echo -e "  - Tar.gz (portable archives)"
    echo ""
    echo -e "${BLUE}The Linux export system uses embedded static libraries,${NC}"
    echo -e "${BLUE}so it works on any platform without additional setup!${NC}"
}

# Main execution
main() {
    # Parse command line arguments
    local skip_download=false
    local skip_embed=false
    local force=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --skip-download)
                skip_download=true
                shift
                ;;
            --skip-embed)
                skip_embed=true
                shift
                ;;
            --force)
                force=true
                shift
                ;;
            --help|-h)
                echo "Usage: $0 [options]"
                echo ""
                echo "Options:"
                echo "  --skip-download    Skip downloading Linux libraries"
                echo "  --skip-embed       Skip generating embedded headers"
                echo "  --force            Force regeneration of existing files"
                echo "  --help, -h         Show this help message"
                exit 0
                ;;
            *)
                echo "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
    
    # Check system requirements
    if ! check_requirements; then
        exit 1
    fi
    
    # Download Linux libraries (unless skipped)
    if ! $skip_download; then
        if ! download_linux_libraries; then
            echo -e "${RED}Failed to download Linux libraries${NC}"
            echo -e "${YELLOW}You can skip this step with --skip-download if you already have the libraries${NC}"
            exit 1
        fi
    else
        echo -e "${YELLOW}Skipping Linux library download${NC}"
    fi
    
    # Generate embedded headers (unless skipped)
    if ! $skip_embed; then
        if ! generate_embedded_headers; then
            echo -e "${RED}Failed to generate embedded headers${NC}"
            echo -e "${YELLOW}You can skip this step with --skip-embed if you don't want embedded libraries${NC}"
            exit 1
        fi
    else
        echo -e "${YELLOW}Skipping embedded header generation${NC}"
    fi
    
    # Verify setup
    if ! verify_setup; then
        echo -e "${RED}Setup verification failed${NC}"
        exit 1
    fi
    
    # Show usage instructions
    show_usage_instructions
    
    echo -e "${GREEN}Linux export setup completed successfully!${NC}"
}

# Run main function with all arguments
main "$@"
