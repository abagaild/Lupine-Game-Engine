#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "lupine/core/Project.h"

namespace Lupine {

/**
 * @brief Export target platforms
 */
enum class ExportTarget {
    Windows_x64,
    Linux_x64,
    Web_HTML5
};

/**
 * @brief Export configuration settings
 */
struct ExportConfig {
    ExportTarget target = ExportTarget::Windows_x64;
    std::string output_directory;
    std::string executable_name;
    bool include_debug_symbols = false;
    bool optimize_assets = true;
    bool embed_assets = true;
    bool create_installer = false;
    
    // Platform-specific settings
    struct WindowsSettings {
        std::string icon_path;
        std::string version_info;
        bool console_subsystem = false;
    } windows;
    
    struct LinuxSettings {
        std::string desktop_file_name;
        std::string app_category = "Game";
        std::vector<std::string> dependencies;
        std::vector<std::string> package_formats; // e.g., {"AppImage", "Deb", "RPM"}
        std::string maintainer = "Game Developer";
        std::string homepage = "";
        std::string license = "Proprietary";
        std::vector<std::string> keywords;

        // Package-specific settings
        struct {
            std::string section = "games";
            std::string priority = "optional";
            std::string architecture = "amd64";
        } deb;

        struct {
            std::string group = "Amusements/Games";
            std::string build_arch = "x86_64";
        } rpm;

        struct {
            std::string app_id;
            std::string runtime = "org.freedesktop.Platform";
            std::string runtime_version = "23.08";
            std::string sdk = "org.freedesktop.Sdk";
        } flatpak;

        struct {
            std::string grade = "stable";
            std::string confinement = "strict";
        } snap;
    } linux_config;
    
    struct WebSettings {
        std::string canvas_size = "1920x1080";
        bool enable_threads = false;
        bool enable_simd = true;
        int memory_size_mb = 512;
    } web;
};

/**
 * @brief Progress callback for export operations
 */
using ExportProgressCallback = std::function<void(float progress, const std::string& status)>;

/**
 * @brief Export result information
 */
struct ExportResult {
    bool success = false;
    std::string error_message;
    std::string output_path;
    size_t total_size_bytes = 0;
    std::vector<std::string> generated_files;
};

/**
 * @brief Base class for platform-specific exporters
 */
class BaseExporter {
public:
    virtual ~BaseExporter() = default;
    
    /**
     * @brief Export project to target platform
     * @param project Project to export
     * @param config Export configuration
     * @param progress_callback Progress callback function
     * @return Export result
     */
    virtual ExportResult Export(const Project* project, const ExportConfig& config, 
                               ExportProgressCallback progress_callback = nullptr) = 0;
    
    /**
     * @brief Get the target platform for this exporter
     * @return Target platform
     */
    virtual ExportTarget GetTarget() const = 0;
    
    /**
     * @brief Check if this exporter is available on current system
     * @return True if exporter can be used
     */
    virtual bool IsAvailable() const = 0;
    
    /**
     * @brief Get human-readable name for this exporter
     * @return Exporter name
     */
    virtual std::string GetName() const = 0;
};

/**
 * @brief Main export manager class
 */
class ExportManager {
public:
    ExportManager();
    ~ExportManager();
    
    /**
     * @brief Get all available exporters
     * @return Vector of available exporters
     */
    std::vector<BaseExporter*> GetAvailableExporters() const;
    
    /**
     * @brief Get exporter for specific target
     * @param target Target platform
     * @return Exporter or nullptr if not available
     */
    BaseExporter* GetExporter(ExportTarget target) const;
    
    /**
     * @brief Export project using specified configuration
     * @param project Project to export
     * @param config Export configuration
     * @param progress_callback Progress callback function
     * @return Export result
     */
    ExportResult ExportProject(const Project* project, const ExportConfig& config,
                              ExportProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Get default export configuration for target
     * @param target Target platform
     * @return Default configuration
     */
    ExportConfig GetDefaultConfig(ExportTarget target) const;
    
    /**
     * @brief Validate export configuration
     * @param config Configuration to validate
     * @return Error message or empty string if valid
     */
    std::string ValidateConfig(const ExportConfig& config) const;

private:
    std::vector<std::unique_ptr<BaseExporter>> m_exporters;
    
    void InitializeExporters();
};

} // namespace Lupine
