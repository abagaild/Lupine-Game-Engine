#include "lupine/export/LinuxPackageBuilder.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <algorithm>

namespace Lupine {

LinuxPackageBuilder::LinuxPackageBuilder() {
    // Create temporary directory for build operations
    m_temp_dir = std::filesystem::temp_directory_path() / "lupine_package_build";
    std::filesystem::create_directories(m_temp_dir);
}

LinuxPackageBuilder::~LinuxPackageBuilder() {
    CleanupTempFiles();
}

bool LinuxPackageBuilder::IsFormatSupported(LinuxPackageFormat format) const {
    switch (format) {
        case LinuxPackageFormat::AppImage:
            return CheckToolAvailability("appimagetool");
        case LinuxPackageFormat::Flatpak:
            return CheckToolAvailability("flatpak-builder");
        case LinuxPackageFormat::Snap:
            return CheckToolAvailability("snapcraft");
        case LinuxPackageFormat::Deb:
            return CheckToolAvailability("dpkg-deb");
        case LinuxPackageFormat::Rpm:
            return CheckToolAvailability("rpmbuild");
        case LinuxPackageFormat::TarGz:
            return CheckToolAvailability("tar");
        case LinuxPackageFormat::All:
            return true; // Always supported - will build what's available
    }
    return false;
}

std::vector<LinuxPackageFormat> LinuxPackageBuilder::GetSupportedFormats() const {
    std::vector<LinuxPackageFormat> supported;
    
    std::vector<LinuxPackageFormat> all_formats = {
        LinuxPackageFormat::AppImage,
        LinuxPackageFormat::Flatpak,
        LinuxPackageFormat::Snap,
        LinuxPackageFormat::Deb,
        LinuxPackageFormat::Rpm,
        LinuxPackageFormat::TarGz
    };
    
    for (auto format : all_formats) {
        if (IsFormatSupported(format)) {
            supported.push_back(format);
        }
    }
    
    return supported;
}

void LinuxPackageBuilder::SetMetadata(const LinuxPackageMetadata& metadata) {
    m_metadata = metadata;
}

void LinuxPackageBuilder::SetSourceDirectory(const std::filesystem::path& source_dir) {
    m_source_dir = source_dir;
}

void LinuxPackageBuilder::SetOutputDirectory(const std::filesystem::path& output_dir) {
    m_output_dir = output_dir;
    std::filesystem::create_directories(m_output_dir);
}

void LinuxPackageBuilder::AddFile(const std::filesystem::path& source_path, 
                                 const std::string& dest_path, 
                                 int permissions) {
    FileEntry entry;
    entry.source_path = source_path;
    entry.dest_path = dest_path;
    entry.permissions = permissions;
    m_files.push_back(entry);
}

void LinuxPackageBuilder::AddDirectory(const std::filesystem::path& source_dir,
                                      const std::string& dest_dir,
                                      bool recursive) {
    if (!std::filesystem::exists(source_dir)) {
        return;
    }
    
    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(source_dir)) {
            if (entry.is_regular_file()) {
                auto relative_path = std::filesystem::relative(entry.path(), source_dir);
                auto dest_path = dest_dir + "/" + relative_path.string();
                AddFile(entry.path(), dest_path, 0644);
            }
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(source_dir)) {
            if (entry.is_regular_file()) {
                auto dest_path = dest_dir + "/" + entry.path().filename().string();
                AddFile(entry.path(), dest_path, 0644);
            }
        }
    }
}

void LinuxPackageBuilder::AddStaticLibraries(const std::vector<std::filesystem::path>& lib_paths) {
    m_static_libraries = lib_paths;
}

PackageBuildResult LinuxPackageBuilder::BuildPackage(LinuxPackageFormat format,
                                                   PackageBuildProgressCallback progress_callback) {
    if (!ValidateMetadata()) {
        PackageBuildResult result;
        result.success = false;
        result.error_message = "Invalid package metadata";
        result.format = format;
        return result;
    }
    
    switch (format) {
        case LinuxPackageFormat::AppImage:
            return BuildAppImage(progress_callback);
        case LinuxPackageFormat::Flatpak:
            return BuildFlatpak(progress_callback);
        case LinuxPackageFormat::Snap:
            return BuildSnap(progress_callback);
        case LinuxPackageFormat::Deb:
            return BuildDeb(progress_callback);
        case LinuxPackageFormat::Rpm:
            return BuildRpm(progress_callback);
        case LinuxPackageFormat::TarGz:
            return BuildTarGz(progress_callback);
        case LinuxPackageFormat::All:
            // This should not be called directly - use BuildAllPackages instead
            break;
    }
    
    PackageBuildResult result;
    result.success = false;
    result.error_message = "Unsupported package format";
    result.format = format;
    return result;
}

std::vector<PackageBuildResult> LinuxPackageBuilder::BuildAllPackages(PackageBuildProgressCallback progress_callback) {
    std::vector<PackageBuildResult> results;
    auto supported_formats = GetSupportedFormats();
    
    float progress_per_format = 1.0f / supported_formats.size();
    
    for (size_t i = 0; i < supported_formats.size(); ++i) {
        auto format = supported_formats[i];
        
        auto result = BuildPackage(format, [&](float format_progress, const std::string& status) {
            if (progress_callback) {
                float total_progress = (i * progress_per_format) + (format_progress * progress_per_format);
                std::string format_status = PackageFormatToString(format) + ": " + status;
                progress_callback(total_progress, format_status);
            }
        });
        
        results.push_back(result);
    }
    
    return results;
}

bool LinuxPackageBuilder::ValidateMetadata() const {
    if (m_metadata.name.empty()) {
        std::cerr << "Package name is required" << std::endl;
        return false;
    }
    
    if (m_metadata.version.empty()) {
        std::cerr << "Package version is required" << std::endl;
        return false;
    }
    
    if (m_metadata.description.empty()) {
        std::cerr << "Package description is required" << std::endl;
        return false;
    }
    
    return true;
}

size_t LinuxPackageBuilder::GetEstimatedPackageSize(LinuxPackageFormat format) const {
    size_t total_size = 0;
    
    // Calculate size of all files
    for (const auto& file_entry : m_files) {
        if (std::filesystem::exists(file_entry.source_path)) {
            total_size += std::filesystem::file_size(file_entry.source_path);
        }
    }
    
    // Add size of static libraries
    for (const auto& lib_path : m_static_libraries) {
        if (std::filesystem::exists(lib_path)) {
            total_size += std::filesystem::file_size(lib_path);
        }
    }
    
    // Apply format-specific overhead estimates
    switch (format) {
        case LinuxPackageFormat::AppImage:
            total_size = static_cast<size_t>(total_size * 1.1); // 10% overhead
            break;
        case LinuxPackageFormat::Flatpak:
            total_size = static_cast<size_t>(total_size * 1.2); // 20% overhead
            break;
        case LinuxPackageFormat::Snap:
            total_size = static_cast<size_t>(total_size * 1.15); // 15% overhead
            break;
        case LinuxPackageFormat::Deb:
        case LinuxPackageFormat::Rpm:
            total_size = static_cast<size_t>(total_size * 1.05); // 5% overhead
            break;
        case LinuxPackageFormat::TarGz:
            total_size = static_cast<size_t>(total_size * 0.7); // 30% compression
            break;
        case LinuxPackageFormat::All:
            break;
    }
    
    return total_size;
}

void LinuxPackageBuilder::CleanupTempFiles() {
    if (std::filesystem::exists(m_temp_dir)) {
        std::filesystem::remove_all(m_temp_dir);
    }
}

bool LinuxPackageBuilder::CheckToolAvailability(const std::string& tool) const {
    // Check if tool path is cached
    if (m_tool_paths.find(tool) != m_tool_paths.end()) {
        return !m_tool_paths[tool].empty();
    }
    
    // Try to find tool in PATH
    std::string command = "which " + tool + " > /dev/null 2>&1";
    bool available = (std::system(command.c_str()) == 0);
    
    // Cache result
    m_tool_paths[tool] = available ? tool : "";
    
    return available;
}

std::string LinuxPackageBuilder::GetToolPath(const std::string& tool) const {
    if (CheckToolAvailability(tool)) {
        return m_tool_paths[tool];
    }
    return "";
}

std::string LinuxPackageBuilder::GeneratePackageFileName(LinuxPackageFormat format) const {
    std::string filename = m_metadata.name + "-" + m_metadata.version;
    
    switch (format) {
        case LinuxPackageFormat::AppImage:
            filename += ".AppImage";
            break;
        case LinuxPackageFormat::Flatpak:
            filename += ".flatpak";
            break;
        case LinuxPackageFormat::Snap:
            filename += ".snap";
            break;
        case LinuxPackageFormat::Deb:
            filename += "_" + m_metadata.deb.architecture + ".deb";
            break;
        case LinuxPackageFormat::Rpm:
            filename += "." + m_metadata.rpm.build_arch + ".rpm";
            break;
        case LinuxPackageFormat::TarGz:
            filename += ".tar.gz";
            break;
        case LinuxPackageFormat::All:
            break;
    }
    
    return filename;
}

std::string PackageFormatToString(LinuxPackageFormat format) {
    switch (format) {
        case LinuxPackageFormat::AppImage: return "AppImage";
        case LinuxPackageFormat::Flatpak: return "Flatpak";
        case LinuxPackageFormat::Snap: return "Snap";
        case LinuxPackageFormat::Deb: return "Deb";
        case LinuxPackageFormat::Rpm: return "RPM";
        case LinuxPackageFormat::TarGz: return "TarGz";
        case LinuxPackageFormat::All: return "All";
    }
    return "Unknown";
}

LinuxPackageFormat StringToPackageFormat(const std::string& format_str) {
    std::string lower_format = format_str;
    std::transform(lower_format.begin(), lower_format.end(), lower_format.begin(), ::tolower);
    
    if (lower_format == "appimage") return LinuxPackageFormat::AppImage;
    if (lower_format == "flatpak") return LinuxPackageFormat::Flatpak;
    if (lower_format == "snap") return LinuxPackageFormat::Snap;
    if (lower_format == "deb") return LinuxPackageFormat::Deb;
    if (lower_format == "rpm") return LinuxPackageFormat::Rpm;
    if (lower_format == "targz" || lower_format == "tar.gz") return LinuxPackageFormat::TarGz;
    if (lower_format == "all") return LinuxPackageFormat::All;
    
    return LinuxPackageFormat::AppImage; // Default
}

// Package format implementations

PackageBuildResult LinuxPackageBuilder::BuildAppImage(PackageBuildProgressCallback progress_callback) {
    PackageBuildResult result;
    result.format = LinuxPackageFormat::AppImage;

    try {
        if (progress_callback) progress_callback(0.1f, "Creating AppDir structure...");

        // Create AppDir structure
        auto appdir_path = CreateAppDir(m_temp_dir / "appimage");
        if (appdir_path.empty()) {
            result.error_message = "Failed to create AppDir structure";
            return result;
        }

        if (progress_callback) progress_callback(0.3f, "Copying files to AppDir...");

        // Copy all files to AppDir
        for (const auto& file_entry : m_files) {
            auto dest_path = appdir_path / file_entry.dest_path;
            std::filesystem::create_directories(dest_path.parent_path());
            std::filesystem::copy_file(file_entry.source_path, dest_path,
                                      std::filesystem::copy_options::overwrite_existing);
            std::filesystem::permissions(dest_path, static_cast<std::filesystem::perms>(file_entry.permissions));
        }

        if (progress_callback) progress_callback(0.5f, "Copying static libraries...");

        // Copy static libraries to lib directory
        if (!CopyStaticLibraries(appdir_path / "usr" / "lib")) {
            result.error_message = "Failed to copy static libraries";
            return result;
        }

        if (progress_callback) progress_callback(0.6f, "Creating desktop file...");

        // Create desktop file
        if (!CreateDesktopFile(appdir_path / (m_metadata.desktop_file_name + ".desktop"))) {
            result.error_message = "Failed to create desktop file";
            return result;
        }

        if (progress_callback) progress_callback(0.7f, "Creating AppRun script...");

        // Create AppRun script
        if (!CreateAppRunScript(appdir_path / "AppRun", m_metadata.name)) {
            result.error_message = "Failed to create AppRun script";
            return result;
        }

        if (progress_callback) progress_callback(0.8f, "Building AppImage...");

        // Build AppImage
        auto output_path = m_output_dir / GeneratePackageFileName(LinuxPackageFormat::AppImage);
        if (!BuildAppImageFromAppDir(appdir_path, output_path)) {
            result.error_message = "Failed to build AppImage";
            return result;
        }

        result.success = true;
        result.package_path = output_path.string();
        result.package_size = std::filesystem::file_size(output_path);
        result.format_version = "1.0";

    } catch (const std::exception& e) {
        result.error_message = "AppImage build failed: " + std::string(e.what());
    }

    return result;
}

PackageBuildResult LinuxPackageBuilder::BuildTarGz(PackageBuildProgressCallback progress_callback) {
    PackageBuildResult result;
    result.format = LinuxPackageFormat::TarGz;

    try {
        if (progress_callback) progress_callback(0.1f, "Creating package structure...");

        // Create package directory structure
        auto package_dir = m_temp_dir / "targz" / m_metadata.name;
        std::filesystem::create_directories(package_dir);

        if (progress_callback) progress_callback(0.3f, "Copying files...");

        // Copy all files to package directory
        for (const auto& file_entry : m_files) {
            auto dest_path = package_dir / file_entry.dest_path;
            std::filesystem::create_directories(dest_path.parent_path());
            std::filesystem::copy_file(file_entry.source_path, dest_path,
                                      std::filesystem::copy_options::overwrite_existing);
            std::filesystem::permissions(dest_path, static_cast<std::filesystem::perms>(file_entry.permissions));
        }

        if (progress_callback) progress_callback(0.5f, "Copying static libraries...");

        // Copy static libraries
        if (!CopyStaticLibraries(package_dir / "lib")) {
            result.error_message = "Failed to copy static libraries";
            return result;
        }

        if (progress_callback) progress_callback(0.7f, "Creating archive...");

        // Create tar.gz archive
        auto output_path = m_output_dir / GeneratePackageFileName(LinuxPackageFormat::TarGz);
        std::string command = "cd " + (m_temp_dir / "targz").string() + " && tar -czf " +
                             output_path.string() + " " + m_metadata.name;

        if (std::system(command.c_str()) != 0) {
            result.error_message = "Failed to create tar.gz archive";
            return result;
        }

        result.success = true;
        result.package_path = output_path.string();
        result.package_size = std::filesystem::file_size(output_path);
        result.format_version = "1.0";

    } catch (const std::exception& e) {
        result.error_message = "TarGz build failed: " + std::string(e.what());
    }

    return result;
}

PackageBuildResult LinuxPackageBuilder::BuildDeb(PackageBuildProgressCallback progress_callback) {
    PackageBuildResult result;
    result.format = LinuxPackageFormat::Deb;

    try {
        if (progress_callback) progress_callback(0.1f, "Creating Debian package structure...");

        // Create Debian package directory structure
        auto package_dir = m_temp_dir / "deb" / m_metadata.name;
        auto debian_dir = package_dir / "DEBIAN";
        std::filesystem::create_directories(debian_dir);

        if (progress_callback) progress_callback(0.2f, "Creating control files...");

        // Create Debian control files
        if (!CreateDebianControlFiles(debian_dir)) {
            result.error_message = "Failed to create Debian control files";
            return result;
        }

        if (progress_callback) progress_callback(0.4f, "Copying files...");

        // Copy all files to package directory
        for (const auto& file_entry : m_files) {
            auto dest_path = package_dir / file_entry.dest_path;
            std::filesystem::create_directories(dest_path.parent_path());
            std::filesystem::copy_file(file_entry.source_path, dest_path,
                                      std::filesystem::copy_options::overwrite_existing);
            std::filesystem::permissions(dest_path, static_cast<std::filesystem::perms>(file_entry.permissions));
        }

        if (progress_callback) progress_callback(0.6f, "Copying static libraries...");

        // Copy static libraries
        if (!CopyStaticLibraries(package_dir / "usr" / "lib")) {
            result.error_message = "Failed to copy static libraries";
            return result;
        }

        if (progress_callback) progress_callback(0.8f, "Building Debian package...");

        // Build Debian package
        auto output_path = m_output_dir / GeneratePackageFileName(LinuxPackageFormat::Deb);
        if (!BuildDebFromControl(package_dir, output_path)) {
            result.error_message = "Failed to build Debian package";
            return result;
        }

        result.success = true;
        result.package_path = output_path.string();
        result.package_size = std::filesystem::file_size(output_path);
        result.format_version = "1.0";

    } catch (const std::exception& e) {
        result.error_message = "Deb build failed: " + std::string(e.what());
    }

    return result;
}

PackageBuildResult LinuxPackageBuilder::BuildRpm(PackageBuildProgressCallback progress_callback) {
    PackageBuildResult result;
    result.format = LinuxPackageFormat::Rpm;
    result.error_message = "RPM package building not yet implemented";
    return result;
}

PackageBuildResult LinuxPackageBuilder::BuildFlatpak(PackageBuildProgressCallback progress_callback) {
    PackageBuildResult result;
    result.format = LinuxPackageFormat::Flatpak;
    result.error_message = "Flatpak package building not yet implemented";
    return result;
}

PackageBuildResult LinuxPackageBuilder::BuildSnap(PackageBuildProgressCallback progress_callback) {
    PackageBuildResult result;
    result.format = LinuxPackageFormat::Snap;
    result.error_message = "Snap package building not yet implemented";
    return result;
}

// Helper method implementations

std::filesystem::path LinuxPackageBuilder::CreateAppDir(const std::filesystem::path& base_dir) const {
    try {
        auto appdir_path = base_dir / (m_metadata.name + ".AppDir");

        // Create directory structure
        std::filesystem::create_directories(appdir_path);
        std::filesystem::create_directories(appdir_path / "usr" / "bin");
        std::filesystem::create_directories(appdir_path / "usr" / "lib");
        std::filesystem::create_directories(appdir_path / "usr" / "share" / "applications");
        std::filesystem::create_directories(appdir_path / "usr" / "share" / "icons" / "hicolor" / "256x256" / "apps");

        return appdir_path;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create AppDir: " << e.what() << std::endl;
        return {};
    }
}

bool LinuxPackageBuilder::CreateDesktopFile(const std::filesystem::path& output_path) const {
    try {
        std::ofstream desktop_file(output_path);
        if (!desktop_file) {
            return false;
        }

        desktop_file << "[Desktop Entry]\n";
        desktop_file << "Type=Application\n";
        desktop_file << "Name=" << m_metadata.name << "\n";
        desktop_file << "Comment=" << m_metadata.description << "\n";
        desktop_file << "Exec=" << m_metadata.name << "\n";
        desktop_file << "Icon=" << m_metadata.name << "\n";
        desktop_file << "Categories=" << m_metadata.category << ";\n";
        desktop_file << "Terminal=false\n";
        desktop_file << "Version=" << m_metadata.version << "\n";

        if (!m_metadata.keywords.empty()) {
            desktop_file << "Keywords=";
            for (size_t i = 0; i < m_metadata.keywords.size(); ++i) {
                if (i > 0) desktop_file << ";";
                desktop_file << m_metadata.keywords[i];
            }
            desktop_file << ";\n";
        }

        desktop_file.close();

        // Make executable
        std::filesystem::permissions(output_path,
                                   std::filesystem::perms::owner_all |
                                   std::filesystem::perms::group_read |
                                   std::filesystem::perms::group_exec |
                                   std::filesystem::perms::others_read |
                                   std::filesystem::perms::others_exec);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create desktop file: " << e.what() << std::endl;
        return false;
    }
}

bool LinuxPackageBuilder::CreateAppRunScript(const std::filesystem::path& output_path, const std::string& executable_name) const {
    try {
        std::ofstream apprun_file(output_path);
        if (!apprun_file) {
            return false;
        }

        apprun_file << "#!/bin/bash\n";
        apprun_file << "HERE=\"$(dirname \"$(readlink -f \"${0}\")\")\" \n";
        apprun_file << "export LD_LIBRARY_PATH=\"${HERE}/usr/lib:${LD_LIBRARY_PATH}\"\n";
        apprun_file << "export PATH=\"${HERE}/usr/bin:${PATH}\"\n";
        apprun_file << "cd \"${HERE}\"\n";
        apprun_file << "exec \"${HERE}/usr/bin/" << executable_name << "\" \"$@\"\n";

        apprun_file.close();

        // Make executable
        std::filesystem::permissions(output_path,
                                   std::filesystem::perms::owner_all |
                                   std::filesystem::perms::group_read |
                                   std::filesystem::perms::group_exec |
                                   std::filesystem::perms::others_read |
                                   std::filesystem::perms::others_exec);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create AppRun script: " << e.what() << std::endl;
        return false;
    }
}

bool LinuxPackageBuilder::CopyStaticLibraries(const std::filesystem::path& lib_dir) const {
    try {
        std::filesystem::create_directories(lib_dir);

        for (const auto& lib_path : m_static_libraries) {
            if (std::filesystem::exists(lib_path)) {
                auto dest_path = lib_dir / lib_path.filename();
                std::filesystem::copy_file(lib_path, dest_path,
                                          std::filesystem::copy_options::overwrite_existing);
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy static libraries: " << e.what() << std::endl;
        return false;
    }
}

bool LinuxPackageBuilder::BuildAppImageFromAppDir(const std::filesystem::path& appdir,
                                                 const std::filesystem::path& output_path) const {
    try {
        auto appimagetool_path = GetToolPath("appimagetool");
        if (appimagetool_path.empty()) {
            return false;
        }

        // Build command
        std::string command = appimagetool_path + " \"" + appdir.string() + "\" \"" + output_path.string() + "\"";

        int result = std::system(command.c_str());
        return result == 0;

    } catch (const std::exception& e) {
        std::cerr << "Failed to build AppImage: " << e.what() << std::endl;
        return false;
    }
}

bool LinuxPackageBuilder::CreateDebianControlFiles(const std::filesystem::path& debian_dir) const {
    try {
        // Create control file
        std::ofstream control_file(debian_dir / "control");
        if (!control_file) {
            return false;
        }

        control_file << "Package: " << m_metadata.name << "\n";
        control_file << "Version: " << m_metadata.version << "\n";
        control_file << "Section: " << m_metadata.deb.section << "\n";
        control_file << "Priority: " << m_metadata.deb.priority << "\n";
        control_file << "Architecture: " << m_metadata.deb.architecture << "\n";
        control_file << "Maintainer: " << m_metadata.maintainer << "\n";
        control_file << "Description: " << m_metadata.description << "\n";
        control_file << "Homepage: " << m_metadata.homepage << "\n";

        if (!m_metadata.dependencies.empty()) {
            control_file << "Depends: ";
            for (size_t i = 0; i < m_metadata.dependencies.size(); ++i) {
                if (i > 0) control_file << ", ";
                control_file << m_metadata.dependencies[i];
            }
            control_file << "\n";
        }

        control_file.close();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create Debian control files: " << e.what() << std::endl;
        return false;
    }
}

bool LinuxPackageBuilder::BuildDebFromControl(const std::filesystem::path& package_dir,
                                             const std::filesystem::path& output_path) const {
    try {
        auto dpkg_deb_path = GetToolPath("dpkg-deb");
        if (dpkg_deb_path.empty()) {
            return false;
        }

        // Build command
        std::string command = dpkg_deb_path + " --build \"" + package_dir.string() + "\" \"" + output_path.string() + "\"";

        int result = std::system(command.c_str());
        return result == 0;

    } catch (const std::exception& e) {
        std::cerr << "Failed to build Debian package: " << e.what() << std::endl;
        return false;
    }
}

} // namespace Lupine
