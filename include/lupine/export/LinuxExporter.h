#pragma once

#include "ExportManager.h"
#include "LinuxPackageBuilder.h"
#include <filesystem>

namespace Lupine {

// Forward declarations
class EmbeddedLibraryManager;

/**
 * @brief Linux multi-format exporter (AppImage, Flatpak, Snap, Deb, RPM, etc.)
 */
class LinuxExporter : public BaseExporter {
public:
    LinuxExporter();
    ~LinuxExporter() override = default;
    
    ExportResult Export(const Project* project, const ExportConfig& config, 
                       ExportProgressCallback progress_callback = nullptr) override;
    
    ExportTarget GetTarget() const override { return ExportTarget::Linux_x64; }
    bool IsAvailable() const override;
    std::string GetAvailabilityError() const override;
    std::string GetName() const override { return "Linux x64 (Multi-format)"; }

private:
    /**
     * @brief Create AppDir structure for AppImage
     * @param output_dir Output directory
     * @param app_name Application name
     * @return Path to AppDir
     */
    std::filesystem::path CreateAppDir(const std::filesystem::path& output_dir,
                                      const std::string& app_name);
    
    /**
     * @brief Copy runtime executable to AppDir
     * @param appdir_path AppDir path
     * @param executable_name Name for the executable
     * @return True if successful
     */
    bool CopyRuntimeToAppDir(const std::filesystem::path& appdir_path,
                            const std::string& executable_name);
    
    /**
     * @brief Copy required shared libraries to AppDir
     * @param appdir_path AppDir path
     * @return True if successful
     */
    bool CopySharedLibraries(const std::filesystem::path& appdir_path);
    
    /**
     * @brief Create desktop file for AppImage
     * @param appdir_path AppDir path
     * @param config Export configuration
     * @return True if successful
     */
    bool CreateDesktopFile(const std::filesystem::path& appdir_path,
                          const ExportConfig& config);
    
    /**
     * @brief Create AppRun script
     * @param appdir_path AppDir path
     * @param executable_name Executable name
     * @return True if successful
     */
    bool CreateAppRunScript(const std::filesystem::path& appdir_path,
                           const std::string& executable_name);
    
    /**
     * @brief Copy application icon
     * @param appdir_path AppDir path
     * @param icon_path Source icon path
     * @param app_name Application name
     * @return True if successful
     */
    bool CopyApplicationIcon(const std::filesystem::path& appdir_path,
                            const std::filesystem::path& icon_path,
                            const std::string& app_name);
    
    /**
     * @brief Build AppImage from AppDir
     * @param appdir_path AppDir path
     * @param output_path Output AppImage path
     * @return True if successful
     */
    bool BuildAppImage(const std::filesystem::path& appdir_path,
                      const std::filesystem::path& output_path);
    
    /**
     * @brief Check if appimagetool is available
     * @return True if appimagetool can be found
     */
    bool IsAppImageToolAvailable() const;
    
    /**
     * @brief Get path to appimagetool
     * @return Path to appimagetool or empty if not found
     */
    std::filesystem::path GetAppImageToolPath() const;
    
    /**
     * @brief Get list of required shared libraries (deprecated)
     * @return Vector of library file paths
     */
    std::vector<std::filesystem::path> GetRequiredLibraries() const;

    /**
     * @brief Build or copy Linux runtime executable
     * @param output_dir Directory to place the runtime
     * @return Path to runtime executable or empty if failed
     */
    std::filesystem::path BuildLinuxRuntime(const std::filesystem::path& output_dir) const;

    /**
     * @brief Get list of required static libraries for Linux
     * @return Vector of static library file paths
     */
    std::vector<std::filesystem::path> GetRequiredStaticLibraries() const;

    /**
     * @brief Create package metadata from project and config
     * @param project Project to export
     * @param config Export configuration
     * @return Package metadata structure
     */
    LinuxPackageMetadata CreatePackageMetadata(const Project* project, const ExportConfig& config) const;

    /**
     * @brief Get executable name without package extension
     * @param config Export configuration
     * @return Base executable name
     */
    std::string GetExecutableName(const ExportConfig& config) const;

    /**
     * @brief Determine package format from executable name
     * @param executable_name Name with potential extension
     * @return Package format to build
     */
    LinuxPackageFormat DeterminePackageFormat(const std::string& executable_name) const;

    /**
     * @brief Get embedded library manager instance
     * @return Reference to embedded library manager
     */
    EmbeddedLibraryManager& GetEmbeddedLibraryManager() const;
};

} // namespace Lupine
