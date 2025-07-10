#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <map>

namespace Lupine {

/**
 * @brief Supported Linux package formats
 */
enum class LinuxPackageFormat {
    AppImage,      // Universal Linux package
    Flatpak,       // Sandboxed application package
    Snap,          // Ubuntu's universal package format
    Deb,           // Debian/Ubuntu package
    Rpm,           // Red Hat/Fedora package
    TarGz,         // Portable archive
    All            // Build all supported formats
};

/**
 * @brief Linux package metadata
 */
struct LinuxPackageMetadata {
    std::string name;
    std::string version;
    std::string description;
    std::string maintainer;
    std::string homepage;
    std::string license;
    std::string category;
    std::vector<std::string> dependencies;
    std::vector<std::string> keywords;
    std::string icon_path;
    std::string desktop_file_name;
    
    // Package-specific metadata
    struct {
        std::string section = "games";
        std::string priority = "optional";
        std::string architecture = "amd64";
        int installed_size = 0;
    } deb;
    
    struct {
        std::string group = "Amusements/Games";
        std::string build_arch = "x86_64";
        std::string summary;
    } rpm;
    
    struct {
        std::string app_id;
        std::string runtime = "org.freedesktop.Platform";
        std::string runtime_version = "23.08";
        std::string sdk = "org.freedesktop.Sdk";
        std::vector<std::string> finish_args;
        std::vector<std::string> modules;
    } flatpak;
    
    struct {
        std::string grade = "stable";
        std::string confinement = "strict";
        std::vector<std::string> plugs;
        std::vector<std::string> slots;
    } snap;
};

/**
 * @brief Result of package building operation
 */
struct PackageBuildResult {
    bool success = false;
    std::string error_message;
    std::string package_path;
    size_t package_size = 0;
    LinuxPackageFormat format;
    std::string format_version;
};

/**
 * @brief Progress callback for package building
 */
using PackageBuildProgressCallback = std::function<void(float progress, const std::string& status)>;

/**
 * @brief Linux package builder for various distribution formats
 */
class LinuxPackageBuilder {
public:
    LinuxPackageBuilder();
    ~LinuxPackageBuilder();

    /**
     * @brief Check if a specific package format is supported on this system
     * @param format Package format to check
     * @return True if format is supported
     */
    bool IsFormatSupported(LinuxPackageFormat format) const;

    /**
     * @brief Get list of all supported formats on this system
     * @return Vector of supported package formats
     */
    std::vector<LinuxPackageFormat> GetSupportedFormats() const;

    /**
     * @brief Set package metadata
     * @param metadata Package metadata structure
     */
    void SetMetadata(const LinuxPackageMetadata& metadata);

    /**
     * @brief Set source directory containing the application files
     * @param source_dir Path to directory with application files
     */
    void SetSourceDirectory(const std::filesystem::path& source_dir);

    /**
     * @brief Set output directory for generated packages
     * @param output_dir Path to output directory
     */
    void SetOutputDirectory(const std::filesystem::path& output_dir);

    /**
     * @brief Add a file to be included in the package
     * @param source_path Source file path
     * @param dest_path Destination path in package
     * @param permissions File permissions (octal)
     */
    void AddFile(const std::filesystem::path& source_path, 
                 const std::string& dest_path, 
                 int permissions = 0644);

    /**
     * @brief Add a directory to be included in the package
     * @param source_dir Source directory path
     * @param dest_dir Destination directory in package
     * @param recursive Include subdirectories recursively
     */
    void AddDirectory(const std::filesystem::path& source_dir,
                      const std::string& dest_dir,
                      bool recursive = true);

    /**
     * @brief Add static library dependencies
     * @param lib_paths Vector of static library paths
     */
    void AddStaticLibraries(const std::vector<std::filesystem::path>& lib_paths);

    /**
     * @brief Build package in specified format
     * @param format Package format to build
     * @param progress_callback Optional progress callback
     * @return Build result
     */
    PackageBuildResult BuildPackage(LinuxPackageFormat format,
                                   PackageBuildProgressCallback progress_callback = nullptr);

    /**
     * @brief Build packages in all supported formats
     * @param progress_callback Optional progress callback
     * @return Vector of build results for each format
     */
    std::vector<PackageBuildResult> BuildAllPackages(PackageBuildProgressCallback progress_callback = nullptr);

    /**
     * @brief Validate package metadata
     * @return True if metadata is valid
     */
    bool ValidateMetadata() const;

    /**
     * @brief Get estimated package size for a format
     * @param format Package format
     * @return Estimated size in bytes
     */
    size_t GetEstimatedPackageSize(LinuxPackageFormat format) const;

    /**
     * @brief Clean up temporary build files
     */
    void CleanupTempFiles();

private:
    // Package format builders
    PackageBuildResult BuildAppImage(PackageBuildProgressCallback progress_callback);
    PackageBuildResult BuildFlatpak(PackageBuildProgressCallback progress_callback);
    PackageBuildResult BuildSnap(PackageBuildProgressCallback progress_callback);
    PackageBuildResult BuildDeb(PackageBuildProgressCallback progress_callback);
    PackageBuildResult BuildRpm(PackageBuildProgressCallback progress_callback);
    PackageBuildResult BuildTarGz(PackageBuildProgressCallback progress_callback);

    // Helper methods
    bool CheckToolAvailability(const std::string& tool) const;
    std::string GetToolPath(const std::string& tool) const;
    bool CreateDesktopFile(const std::filesystem::path& output_path) const;
    bool CreateAppRunScript(const std::filesystem::path& output_path, const std::string& executable_name) const;
    bool CopyStaticLibraries(const std::filesystem::path& lib_dir) const;
    bool StripBinaries(const std::filesystem::path& bin_dir) const;
    std::string GeneratePackageFileName(LinuxPackageFormat format) const;
    
    // AppImage specific
    std::filesystem::path CreateAppDir(const std::filesystem::path& base_dir) const;
    bool BuildAppImageFromAppDir(const std::filesystem::path& appdir, 
                                const std::filesystem::path& output_path) const;
    
    // Flatpak specific
    bool CreateFlatpakManifest(const std::filesystem::path& manifest_path) const;
    bool BuildFlatpakFromManifest(const std::filesystem::path& manifest_path,
                                 const std::filesystem::path& output_path) const;
    
    // Snap specific
    bool CreateSnapcraftYaml(const std::filesystem::path& yaml_path) const;
    bool BuildSnapFromYaml(const std::filesystem::path& yaml_path,
                          const std::filesystem::path& output_path) const;
    
    // Deb specific
    bool CreateDebianControlFiles(const std::filesystem::path& debian_dir) const;
    bool BuildDebFromControl(const std::filesystem::path& package_dir,
                            const std::filesystem::path& output_path) const;
    
    // RPM specific
    bool CreateRpmSpec(const std::filesystem::path& spec_path) const;
    bool BuildRpmFromSpec(const std::filesystem::path& spec_path,
                         const std::filesystem::path& output_path) const;

    // Member variables
    LinuxPackageMetadata m_metadata;
    std::filesystem::path m_source_dir;
    std::filesystem::path m_output_dir;
    std::filesystem::path m_temp_dir;
    
    struct FileEntry {
        std::filesystem::path source_path;
        std::string dest_path;
        int permissions;
    };
    
    std::vector<FileEntry> m_files;
    std::vector<std::filesystem::path> m_static_libraries;
    
    // Tool paths cache
    mutable std::map<std::string, std::string> m_tool_paths;
};

/**
 * @brief Convert package format enum to string
 * @param format Package format
 * @return String representation
 */
std::string PackageFormatToString(LinuxPackageFormat format);

/**
 * @brief Convert string to package format enum
 * @param format_str String representation
 * @return Package format enum
 */
LinuxPackageFormat StringToPackageFormat(const std::string& format_str);

} // namespace Lupine
