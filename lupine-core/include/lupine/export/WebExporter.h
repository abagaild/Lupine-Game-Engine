#pragma once

#include "ExportManager.h"
#include <filesystem>

namespace Lupine {

/**
 * @brief Web HTML5/WebAssembly exporter
 */
class WebExporter : public BaseExporter {
public:
    WebExporter();
    ~WebExporter() override = default;
    
    ExportResult Export(const Project* project, const ExportConfig& config, 
                       ExportProgressCallback progress_callback = nullptr) override;
    
    ExportTarget GetTarget() const override { return ExportTarget::Web_HTML5; }
    bool IsAvailable() const override;
    std::string GetName() const override { return "Web (HTML5/WebAssembly)"; }

private:
    /**
     * @brief Compile project to WebAssembly using Emscripten
     * @param project Project to compile
     * @param config Export configuration
     * @param output_dir Output directory
     * @return True if successful
     */
    bool CompileToWebAssembly(const Project* project, 
                             const ExportConfig& config,
                             const std::filesystem::path& output_dir);
    
    /**
     * @brief Generate HTML wrapper file
     * @param project Project to get settings from
     * @param output_dir Output directory
     * @param config Export configuration
     * @return True if successful
     */
    bool GenerateHTMLWrapper(const Project* project,
                            const std::filesystem::path& output_dir,
                            const ExportConfig& config);
    
    /**
     * @brief Generate JavaScript loader
     * @param output_dir Output directory
     * @param config Export configuration
     * @return True if successful
     */
    bool GenerateJavaScriptLoader(const std::filesystem::path& output_dir,
                                 const ExportConfig& config);
    
    /**
     * @brief Package assets for web deployment
     * @param project Project to package assets from
     * @param output_dir Output directory
     * @return True if successful
     */
    bool PackageWebAssets(const Project* project,
                         const std::filesystem::path& output_dir);
    
    /**
     * @brief Create web manifest file
     * @param output_dir Output directory
     * @param config Export configuration
     * @return True if successful
     */
    bool CreateWebManifest(const std::filesystem::path& output_dir,
                          const ExportConfig& config);
    
    /**
     * @brief Generate service worker for offline support
     * @param output_dir Output directory
     * @param config Export configuration
     * @return True if successful
     */
    bool GenerateServiceWorker(const std::filesystem::path& output_dir,
                              const ExportConfig& config);
    
    /**
     * @brief Check if Emscripten is available
     * @return True if Emscripten toolchain is found
     */
    bool IsEmscriptenAvailable() const;
    
    /**
     * @brief Get Emscripten compiler path
     * @return Path to emcc or empty if not found
     */
    std::filesystem::path GetEmscriptenCompilerPath() const;
    
    /**
     * @brief Get Emscripten compile flags
     * @param config Export configuration
     * @return Vector of compile flags
     */
    std::vector<std::string> GetEmscriptenFlags(const ExportConfig& config) const;
    
    /**
     * @brief Get list of source files to compile
     * @param project Project to get sources from
     * @return Vector of source file paths
     */
    std::vector<std::filesystem::path> GetSourceFiles(const Project* project) const;
    
    /**
     * @brief Parse canvas size string
     * @param canvas_size Canvas size string (e.g., "1920x1080")
     * @return Pair of width and height
     */
    std::pair<int, int> ParseCanvasSize(const std::string& canvas_size) const;

    /**
     * @brief Create asset manifest for web deployment
     * @param assets_dir Assets directory path
     * @return True if successful
     */
    bool CreateAssetManifest(const std::filesystem::path& assets_dir) const;

    /**
     * @brief Generate default icon for web app
     * @param output_dir Output directory
     * @return True if successful
     */
    bool GenerateDefaultIcon(const std::filesystem::path& output_dir) const;

    /**
     * @brief Get include directories for compilation
     * @return Vector of include directory flags
     */
    std::vector<std::string> GetIncludeDirectories() const;

    /**
     * @brief Get library directories for compilation
     * @return Vector of library directory flags
     */
    std::vector<std::string> GetLibraryDirectories() const;

    /**
     * @brief Get libraries for linking
     * @return Vector of library flags
     */
    std::vector<std::string> GetLibraries() const;
};

} // namespace Lupine