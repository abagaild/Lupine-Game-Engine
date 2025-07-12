#include "lupine/export/MacExporter.h"
#include "lupine/export/AssetBundler.h"
#include "lupine/export/EmbeddedLibraryManager.h"
#include "lupine/core/Project.h"
#include "lupine/core/Scene.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>

#ifdef __APPLE__
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace Lupine {

MacExporter::MacExporter() = default;

bool MacExporter::IsAvailable() const {
#ifdef __APPLE__
    // On Mac, check if Xcode tools are available
    return IsXcodeToolsAvailable();
#else
    // On other platforms, check for embedded Mac libraries
    return IsMacLibrariesAvailable();
#endif
}

std::string MacExporter::GetAvailabilityError() const {
    if (IsAvailable()) {
        return "";
    }
    
#ifdef __APPLE__
    if (!IsXcodeToolsAvailable()) {
        return "Xcode Command Line Tools not found. Install with: xcode-select --install";
    }
    return "Unknown Mac export availability issue";
#else
    if (!IsMacLibrariesAvailable()) {
        auto& embedded_manager = GetEmbeddedLibraryManager();
        if (!embedded_manager.HasLibrariesForPlatform("mac", "x64")) {
            return "Mac static libraries not found in embedded libraries or thirdparty/mac_x64-static directory";
        }
        return "Mac runtime executable not found";
    }
    return "Mac export not available on this platform";
#endif
}

bool MacExporter::IsXcodeToolsAvailable() const {
#ifdef __APPLE__
    // Check if xcodebuild is available
    int result = std::system("which xcodebuild > /dev/null 2>&1");
    if (result == 0) {
        return true;
    }
    
    // Check if clang is available (minimal requirement)
    result = std::system("which clang > /dev/null 2>&1");
    return result == 0;
#else
    return false;
#endif
}

bool MacExporter::IsMacLibrariesAvailable() const {
#ifndef __APPLE__
    // Check embedded libraries first
    auto& embedded_manager = GetEmbeddedLibraryManager();
    if (embedded_manager.HasLibrariesForPlatform("mac", "x64")) {
        return true;
    }
    
    // Check for Mac static libraries in thirdparty directory
    std::filesystem::path mac_libs = "thirdparty/mac_x64-static";
    if (std::filesystem::exists(mac_libs)) {
        return true;
    }
#endif
    
    // Check if runtime executable exists
    auto runtime_path = GetRuntimeExecutablePath();
    return std::filesystem::exists(runtime_path);
}

std::filesystem::path MacExporter::GetRuntimeExecutablePath() const {
#ifdef __APPLE__
    // On Mac, look for the Mac runtime
    std::filesystem::path runtime_path = "build/Release/lupine-runtime";
    if (std::filesystem::exists(runtime_path)) {
        return runtime_path;
    }
    
    runtime_path = "build/lupine-runtime";
    if (std::filesystem::exists(runtime_path)) {
        return runtime_path;
    }
#else
    // On other platforms, look for cross-compiled Mac runtime
    std::filesystem::path runtime_path = "build/mac/lupine-runtime";
    if (std::filesystem::exists(runtime_path)) {
        return runtime_path;
    }
    
    runtime_path = "thirdparty/mac_x64-static/lupine-runtime";
    if (std::filesystem::exists(runtime_path)) {
        return runtime_path;
    }
#endif
    
    return "";
}

ExportResult MacExporter::Export(const Project* project, const ExportConfig& config,
                                ExportProgressCallback progress_callback) {
    ExportResult result;
    
    try {
        std::filesystem::path output_dir(config.output_directory);
        std::filesystem::create_directories(output_dir);
        
        if (progress_callback) {
            progress_callback(0.1f, "Preparing Mac export...");
        }
        
        // Determine bundle name
        std::string bundle_name = config.executable_name;
        if (!bundle_name.ends_with(".app")) {
            bundle_name += ".app";
        }
        
        std::filesystem::path bundle_path = output_dir / bundle_name;
        
        if (progress_callback) {
            progress_callback(0.2f, "Creating application bundle structure...");
        }
        
        // Create app bundle structure
        if (!CreateAppBundle(bundle_path, config)) {
            result.error_message = "Failed to create application bundle structure";
            return result;
        }
        
        if (progress_callback) {
            progress_callback(0.4f, "Creating asset bundle...");
        }

        // Create asset bundle with project files
        AssetBundler bundler;
        bundler.SetOptimizeAssets(config.optimize_assets);

        if (!bundler.AddProject(project->GetFilePath(), true)) {
            result.error_message = "Failed to add project to bundle";
            return result;
        }

        // Create bundle file
        std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "lupine_mac_export";
        std::filesystem::create_directories(temp_dir);
        auto bundle_file_path = temp_dir / "assets.bundle";

        if (!bundler.CreateBundle(bundle_file_path)) {
            result.error_message = "Failed to create asset bundle";
            return result;
        }

        if (progress_callback) {
            progress_callback(0.5f, "Copying runtime and project files...");
        }

        // Copy runtime and project files
        if (!CopyRuntimeToBundle(project, bundle_path, config, bundle_file_path)) {
            result.error_message = "Failed to copy runtime to bundle";
            return result;
        }
        
        if (progress_callback) {
            progress_callback(0.6f, "Creating Info.plist...");
        }
        
        // Create Info.plist
        if (!CreateInfoPlist(bundle_path, config)) {
            result.error_message = "Failed to create Info.plist";
            return result;
        }
        
        if (progress_callback) {
            progress_callback(0.7f, "Setting application icon...");
        }
        
        // Set icon if specified
        if (!config.mac.icon_path.empty()) {
            SetBundleIcon(bundle_path, config.mac.icon_path);
        } else {
            // Look for icon.icns in project root
            std::filesystem::path project_icon = project->GetProjectDirectory() + "/icon.icns";
            if (std::filesystem::exists(project_icon)) {
                SetBundleIcon(bundle_path, project_icon);
            }
        }
        
        if (progress_callback) {
            progress_callback(0.8f, "Code signing (if enabled)...");
        }
        
        // Code sign if requested
        if (config.mac.code_sign && !config.mac.developer_id.empty()) {
            if (!CodeSignBundle(bundle_path, config.mac.developer_id)) {
                std::cout << "Warning: Code signing failed, but export will continue" << std::endl;
            }
        }
        
        if (progress_callback) {
            progress_callback(0.9f, "Creating DMG (if enabled)...");
        }
        
        // Create DMG if requested
        std::filesystem::path final_output = bundle_path;
        if (config.mac.create_dmg) {
            std::filesystem::path dmg_path = output_dir / (bundle_name.substr(0, bundle_name.length() - 4) + ".dmg");
            if (CreateDMG(bundle_path, dmg_path)) {
                final_output = dmg_path;
            }
        }
        
        result.success = true;
        result.output_path = final_output.string();
        
        // Calculate total size
        size_t total_size = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(bundle_path)) {
            if (entry.is_regular_file()) {
                total_size += entry.file_size();
                result.generated_files.push_back(entry.path().string());
            }
        }
        result.total_size_bytes = total_size;
        
        if (progress_callback) {
            progress_callback(1.0f, "Mac export completed successfully");
        }
        
    } catch (const std::exception& e) {
        result.error_message = "Export failed: " + std::string(e.what());
    }
    
    return result;
}

EmbeddedLibraryManager& MacExporter::GetEmbeddedLibraryManager() const {
    static EmbeddedLibraryManager instance;
    return instance;
}

bool MacExporter::CreateAppBundle(const std::filesystem::path& bundle_path, const ExportConfig& config) const {
    try {
        // Create bundle directory structure
        std::filesystem::create_directories(bundle_path / "Contents");
        std::filesystem::create_directories(bundle_path / "Contents" / "MacOS");
        std::filesystem::create_directories(bundle_path / "Contents" / "Resources");
        std::filesystem::create_directories(bundle_path / "Contents" / "Frameworks");

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create app bundle structure: " << e.what() << std::endl;
        return false;
    }
}

bool MacExporter::CopyRuntimeToBundle(const Project* project, const std::filesystem::path& bundle_path,
                                     const ExportConfig& config, const std::filesystem::path& asset_bundle_path) const {
    try {
        auto runtime_path = GetRuntimeExecutablePath();
        if (runtime_path.empty() || !std::filesystem::exists(runtime_path)) {
            std::cerr << "Runtime executable not found" << std::endl;
            return false;
        }

        // Copy runtime to MacOS directory
        std::filesystem::path macos_dir = bundle_path / "Contents" / "MacOS";
        std::string executable_name = config.executable_name;
        if (executable_name.ends_with(".app")) {
            executable_name = executable_name.substr(0, executable_name.length() - 4);
        }

        std::filesystem::path target_executable = macos_dir / executable_name;
        std::filesystem::copy_file(runtime_path, target_executable,
                                  std::filesystem::copy_options::overwrite_existing);

        // Make executable
#ifdef __APPLE__
        chmod(target_executable.c_str(), 0755);
#endif

        // Copy asset bundle to Resources
        std::filesystem::path resources_dir = bundle_path / "Contents" / "Resources";
        if (std::filesystem::exists(asset_bundle_path)) {
            std::filesystem::copy_file(asset_bundle_path, resources_dir / "assets.bundle",
                                      std::filesystem::copy_options::overwrite_existing);
        }

        // Copy project file to Resources (for reference)
        std::filesystem::path project_file = project->GetFilePath();
        std::filesystem::copy_file(project_file, resources_dir / project_file.filename(),
                                  std::filesystem::copy_options::overwrite_existing);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy runtime to bundle: " << e.what() << std::endl;
        return false;
    }
}

bool MacExporter::CreateInfoPlist(const std::filesystem::path& bundle_path, const ExportConfig& config) const {
    try {
        std::filesystem::path plist_path = bundle_path / "Contents" / "Info.plist";
        std::ofstream plist(plist_path);

        if (!plist.is_open()) {
            return false;
        }

        std::string executable_name = config.executable_name;
        if (executable_name.ends_with(".app")) {
            executable_name = executable_name.substr(0, executable_name.length() - 4);
        }

        std::string bundle_id = config.mac.bundle_identifier;
        if (bundle_id.empty()) {
            bundle_id = "com.lupine." + executable_name;
        }

        plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        plist << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
        plist << "<plist version=\"1.0\">\n";
        plist << "<dict>\n";
        plist << "    <key>CFBundleExecutable</key>\n";
        plist << "    <string>" << executable_name << "</string>\n";
        plist << "    <key>CFBundleIdentifier</key>\n";
        plist << "    <string>" << bundle_id << "</string>\n";
        plist << "    <key>CFBundleName</key>\n";
        plist << "    <string>" << executable_name << "</string>\n";
        plist << "    <key>CFBundleDisplayName</key>\n";
        plist << "    <string>" << executable_name << "</string>\n";
        plist << "    <key>CFBundleVersion</key>\n";
        plist << "    <string>" << (config.mac.version_info.version.empty() ? "1.0.0" : config.mac.version_info.version) << "</string>\n";
        plist << "    <key>CFBundleShortVersionString</key>\n";
        plist << "    <string>" << (config.mac.version_info.version.empty() ? "1.0.0" : config.mac.version_info.version) << "</string>\n";
        plist << "    <key>CFBundlePackageType</key>\n";
        plist << "    <string>APPL</string>\n";
        plist << "    <key>CFBundleSignature</key>\n";
        plist << "    <string>????</string>\n";
        plist << "    <key>LSMinimumSystemVersion</key>\n";
        plist << "    <string>" << config.mac.minimum_os_version << "</string>\n";
        plist << "    <key>NSHighResolutionCapable</key>\n";
        plist << "    <true/>\n";

        if (!config.mac.icon_path.empty()) {
            plist << "    <key>CFBundleIconFile</key>\n";
            plist << "    <string>icon</string>\n";
        }

        plist << "</dict>\n";
        plist << "</plist>\n";

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create Info.plist: " << e.what() << std::endl;
        return false;
    }
}

bool MacExporter::SetBundleIcon(const std::filesystem::path& bundle_path,
                               const std::filesystem::path& icon_path) const {
    try {
        if (!std::filesystem::exists(icon_path)) {
            return false;
        }

        std::filesystem::path resources_dir = bundle_path / "Contents" / "Resources";
        std::filesystem::path target_icon = resources_dir / "icon.icns";

        // Copy icon file
        std::filesystem::copy_file(icon_path, target_icon,
                                  std::filesystem::copy_options::overwrite_existing);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to set bundle icon: " << e.what() << std::endl;
        return false;
    }
}

bool MacExporter::CreateDMG(const std::filesystem::path& bundle_path,
                           const std::filesystem::path& output_path) const {
#ifdef __APPLE__
    try {
        // Use hdiutil to create DMG
        std::string command = "hdiutil create -volname \"" + bundle_path.stem().string() +
                             "\" -srcfolder \"" + bundle_path.string() +
                             "\" -ov -format UDZO \"" + output_path.string() + "\"";

        int result = std::system(command.c_str());
        return result == 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create DMG: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "DMG creation is only supported on macOS" << std::endl;
    return false;
#endif
}

bool MacExporter::CodeSignBundle(const std::filesystem::path& bundle_path,
                                const std::string& developer_id) const {
#ifdef __APPLE__
    try {
        std::string command = "codesign --force --sign \"" + developer_id +
                             "\" --deep \"" + bundle_path.string() + "\"";

        int result = std::system(command.c_str());
        return result == 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to code sign bundle: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "Code signing is only supported on macOS" << std::endl;
    return false;
#endif
}

size_t MacExporter::CalculateExportSize(const Project* project, const ExportConfig& config) const {
    // This is a rough estimate
    size_t base_size = 50 * 1024 * 1024; // ~50MB for runtime and frameworks

    // Add project assets size
    // TODO: Implement proper asset size calculation

    return base_size;
}

} // namespace Lupine
