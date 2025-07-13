#include "lupine/export/ExportManager.h"
#include "lupine/export/WindowsExporter.h"
#include "lupine/export/LinuxExporter.h"
#include "lupine/export/MacExporter.h"
#include "lupine/export/WebExporter.h"
#include <iostream>
#include <filesystem>

namespace Lupine {

ExportManager::ExportManager() {
    InitializeExporters();
}

ExportManager::~ExportManager() = default;

void ExportManager::InitializeExporters() {
    // Add all available exporters
    m_exporters.push_back(std::make_unique<WindowsExporter>());
    m_exporters.push_back(std::make_unique<LinuxExporter>());
    m_exporters.push_back(std::make_unique<MacExporter>());
    m_exporters.push_back(std::make_unique<WebExporter>());
}

std::vector<BaseExporter*> ExportManager::GetAvailableExporters() const {
    std::vector<BaseExporter*> available;

    for (const auto& exporter : m_exporters) {
        if (exporter->IsAvailable()) {
            available.push_back(exporter.get());
        }
    }

    return available;
}

std::vector<BaseExporter*> ExportManager::GetAllExporters() const {
    std::vector<BaseExporter*> all;

    for (const auto& exporter : m_exporters) {
        all.push_back(exporter.get());
    }

    return all;
}

BaseExporter* ExportManager::GetExporter(ExportTarget target) const {
    for (const auto& exporter : m_exporters) {
        if (exporter->GetTarget() == target && exporter->IsAvailable()) {
            return exporter.get();
        }
    }
    return nullptr;
}

ExportResult ExportManager::ExportProject(const Project* project, const ExportConfig& config,
                                         ExportProgressCallback progress_callback) {
    ExportResult result;
    
    if (!project) {
        result.error_message = "Project is null";
        return result;
    }
    
    // Validate configuration
    std::string validation_error = ValidateConfig(config);
    if (!validation_error.empty()) {
        result.error_message = "Configuration validation failed: " + validation_error;
        return result;
    }
    
    // Get appropriate exporter
    BaseExporter* exporter = GetExporter(config.target);
    if (!exporter) {
        result.error_message = "No exporter available for target platform";
        return result;
    }
    
    // Create output directory if it doesn't exist
    try {
        std::filesystem::create_directories(config.output_directory);
    } catch (const std::exception& e) {
        result.error_message = "Failed to create output directory: " + std::string(e.what());
        return result;
    }
    
    // Perform export
    if (progress_callback) {
        progress_callback(0.0f, "Starting export...");
    }
    
    result = exporter->Export(project, config, progress_callback);
    
    if (progress_callback) {
        if (result.success) {
            progress_callback(1.0f, "Export completed successfully");
        } else {
            progress_callback(1.0f, "Export failed: " + result.error_message);
        }
    }
    
    return result;
}

ExportConfig ExportManager::GetDefaultConfig(ExportTarget target) const {
    ExportConfig config;
    config.target = target;

    switch (target) {
        case ExportTarget::Windows_x64:
            config.executable_name = "Game.exe";
            config.windows.console_subsystem = false;
            break;

        case ExportTarget::Linux_x64:
            config.executable_name = "Game.AppImage";
            config.linux_config.app_category = "Game";
            break;

        case ExportTarget::Mac_x64:
            config.executable_name = "Game.app";
            config.mac.minimum_os_version = "10.15";
            config.mac.create_dmg = true;
            config.mac.code_sign = false;
            break;

        case ExportTarget::Web_HTML5:
            config.executable_name = "index.html";
            config.web.canvas_size = "1920x1080";
            config.web.memory_size_mb = 512;
            config.web.enable_simd = true;
            break;
    }

    return config;
}

std::string ExportManager::ValidateConfig(const ExportConfig& config) const {
    // Check output directory
    if (config.output_directory.empty()) {
        return "Output directory is required";
    }
    
    // Check executable name
    if (config.executable_name.empty()) {
        return "Executable name is required";
    }
    
    // Platform-specific validation
    switch (config.target) {
        case ExportTarget::Windows_x64:
            if (!config.windows.icon_path.empty()) {
                if (!std::filesystem::exists(config.windows.icon_path)) {
                    return "Windows icon file does not exist: " + config.windows.icon_path;
                }
            }
            break;
            
        case ExportTarget::Linux_x64:
            if (config.linux_config.desktop_file_name.empty()) {
                return "Linux desktop file name is required";
            }
            break;
            
        case ExportTarget::Web_HTML5:
            if (config.web.memory_size_mb < 64) {
                return "Web memory size must be at least 64MB";
            }
            if (config.web.memory_size_mb > 2048) {
                return "Web memory size cannot exceed 2048MB";
            }
            break;
    }
    
    return ""; // Valid
}

} // namespace Lupine
