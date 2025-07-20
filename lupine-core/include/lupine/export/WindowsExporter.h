#pragma once

#include "ExportManager.h"
#include <filesystem>

namespace Lupine {

// Forward declarations
class EmbeddedLibraryManager;

/**
 * @brief Windows executable exporter
 */
class WindowsExporter : public BaseExporter {
public:
    WindowsExporter();
    ~WindowsExporter() override = default;
    
    ExportResult Export(const Project* project, const ExportConfig& config, 
                       ExportProgressCallback progress_callback = nullptr) override;
    
    ExportTarget GetTarget() const override { return ExportTarget::Windows_x64; }
    bool IsAvailable() const override;
    std::string GetName() const override { return "Windows x64"; }

private:
    /**
     * @brief Copy runtime executable to output directory
     * @param output_dir Output directory
     * @param executable_name Name for the executable
     * @param include_debug_symbols Whether to include debug symbols
     * @return True if successful
     */
    bool CopyRuntimeExecutable(const std::filesystem::path& output_dir, 
                              const std::string& executable_name,
                              bool include_debug_symbols);
    
    /**
     * @brief Copy required DLLs to output directory
     * @param output_dir Output directory
     * @return True if successful
     */
    bool CopyRequiredDLLs(const std::filesystem::path& output_dir);
    
    /**
     * @brief Copy project files and assets
     * @param project Project to copy from
     * @param output_dir Output directory
     * @param embed_assets Whether to embed assets in executable
     * @return True if successful
     */
    bool CopyProjectAssets(const Project* project,
                          const std::filesystem::path& output_dir,
                          bool embed_assets);

    /**
     * @brief Create project launcher executable
     * @param project Project to create launcher for
     * @param output_dir Output directory
     * @param executable_name Name for the launcher executable
     * @return True if successful
     */
    bool CreateProjectLauncher(const Project* project,
                              const std::filesystem::path& output_dir,
                              const std::string& executable_name);


    
    /**
     * @brief Create embedded runtime executable with project data
     * @param project Project to embed
     * @param runtime_path Path to runtime executable
     * @param output_path Output executable path
     * @return True if successful
     */
    bool CreateEmbeddedExecutable(const Project* project,
                                 const std::filesystem::path& runtime_path,
                                 const std::filesystem::path& output_path);
    
    /**
     * @brief Set executable icon (Windows-specific)
     * @param executable_path Path to executable
     * @param icon_path Path to icon file
     * @return True if successful
     */
    bool SetExecutableIcon(const std::filesystem::path& executable_path,
                          const std::filesystem::path& icon_path);
    
    /**
     * @brief Get list of required DLL files
     * @return Vector of DLL file paths
     */
    std::vector<std::filesystem::path> GetRequiredDLLs() const;
    
    /**
     * @brief Get runtime executable path
     * @return Path to lupine-runtime.exe
     */
    std::filesystem::path GetRuntimeExecutablePath() const;
    
    /**
     * @brief Calculate total export size
     * @param project Project to calculate size for
     * @param config Export configuration
     * @return Total size in bytes
     */
    size_t CalculateExportSize(const Project* project, const ExportConfig& config) const;

    /**
     * @brief Get embedded library manager instance
     * @return Reference to embedded library manager
     */
    EmbeddedLibraryManager& GetEmbeddedLibraryManager() const;
};

} // namespace Lupine
