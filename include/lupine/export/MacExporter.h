#pragma once

#include "ExportManager.h"
#include <filesystem>

namespace Lupine {

// Forward declarations
class EmbeddedLibraryManager;

/**
 * @brief Mac application bundle exporter
 */
class MacExporter : public BaseExporter {
public:
    MacExporter();
    ~MacExporter() override = default;
    
    ExportResult Export(const Project* project, const ExportConfig& config, 
                       ExportProgressCallback progress_callback = nullptr) override;
    
    ExportTarget GetTarget() const override { return ExportTarget::Mac_x64; }
    bool IsAvailable() const override;
    std::string GetAvailabilityError() const;
    std::string GetName() const override { return "Mac x64 (App Bundle)"; }

private:
    /**
     * @brief Check if Xcode command line tools are available
     * @return True if available
     */
    bool IsXcodeToolsAvailable() const;
    
    /**
     * @brief Check if Mac static libraries are available
     * @return True if available
     */
    bool IsMacLibrariesAvailable() const;
    
    /**
     * @brief Get runtime executable path for Mac
     * @return Path to lupine-runtime executable
     */
    std::filesystem::path GetRuntimeExecutablePath() const;
    
    /**
     * @brief Create Mac application bundle structure
     * @param bundle_path Path to .app bundle
     * @param config Export configuration
     * @return True if successful
     */
    bool CreateAppBundle(const std::filesystem::path& bundle_path, const ExportConfig& config) const;
    
    /**
     * @brief Copy runtime and project files to bundle
     * @param project Project to export
     * @param bundle_path Path to .app bundle
     * @param config Export configuration
     * @param asset_bundle_path Path to the asset bundle file
     * @return True if successful
     */
    bool CopyRuntimeToBundle(const Project* project, const std::filesystem::path& bundle_path,
                            const ExportConfig& config, const std::filesystem::path& asset_bundle_path) const;
    
    /**
     * @brief Create Info.plist file for the bundle
     * @param bundle_path Path to .app bundle
     * @param config Export configuration
     * @return True if successful
     */
    bool CreateInfoPlist(const std::filesystem::path& bundle_path, const ExportConfig& config) const;
    
    /**
     * @brief Set application icon for the bundle
     * @param bundle_path Path to .app bundle
     * @param icon_path Path to icon file
     * @return True if successful
     */
    bool SetBundleIcon(const std::filesystem::path& bundle_path, 
                      const std::filesystem::path& icon_path) const;
    
    /**
     * @brief Create DMG disk image from app bundle
     * @param bundle_path Path to .app bundle
     * @param output_path Path for output DMG
     * @return True if successful
     */
    bool CreateDMG(const std::filesystem::path& bundle_path, 
                   const std::filesystem::path& output_path) const;
    
    /**
     * @brief Code sign the application bundle
     * @param bundle_path Path to .app bundle
     * @param developer_id Developer ID for signing
     * @return True if successful
     */
    bool CodeSignBundle(const std::filesystem::path& bundle_path, 
                       const std::string& developer_id) const;
    
    /**
     * @brief Get embedded library manager instance
     * @return Reference to embedded library manager
     */
    EmbeddedLibraryManager& GetEmbeddedLibraryManager() const;
    
    /**
     * @brief Calculate total export size
     * @param project Project to calculate size for
     * @param config Export configuration
     * @return Total size in bytes
     */
    size_t CalculateExportSize(const Project* project, const ExportConfig& config) const;
};

} // namespace Lupine
