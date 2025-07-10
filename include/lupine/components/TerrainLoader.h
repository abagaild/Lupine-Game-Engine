#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace Lupine {

// Forward declarations
class TerrainData;
class TerrainRenderer;
class TerrainCollider;

/**
 * @brief Terrain file format types
 */
enum class TerrainFileFormat {
    LupineTerrain,  // Native .terrain format with full data preservation
    Heightmap,      // Raw heightmap data (.raw, .r16, .r32)
    Image,          // Image-based heightmap (.png, .jpg, .tga, .exr)
    OBJ,            // Wavefront OBJ mesh export
    Custom          // Custom format with user-defined loader
};

/**
 * @brief Terrain loading progress callback
 */
using TerrainLoadProgressCallback = std::function<void(float progress, const std::string& status)>;

/**
 * @brief Terrain export options
 */
struct TerrainExportOptions {
    TerrainFileFormat format = TerrainFileFormat::LupineTerrain;
    bool include_textures = true;
    bool include_assets = true;
    bool compress_data = true;
    float mesh_resolution = 1.0f;      // For OBJ export
    bool export_materials = true;      // For OBJ export
    bool export_uv_mapping = true;     // For OBJ export
    std::string texture_output_dir;    // For OBJ export texture baking
    
    TerrainExportOptions() = default;
};

/**
 * @brief Terrain import options
 */
struct TerrainImportOptions {
    TerrainFileFormat format = TerrainFileFormat::LupineTerrain;
    float height_scale = 1.0f;         // Height multiplier for heightmaps
    float world_scale = 1.0f;          // World size multiplier
    bool auto_detect_format = true;    // Auto-detect format from file extension
    bool preserve_aspect_ratio = true; // Maintain aspect ratio when scaling
    glm::vec2 size_override = glm::vec2(0.0f); // Override terrain size (0 = use file data)
    
    TerrainImportOptions() = default;
};

/**
 * @brief Terrain loader component for file I/O operations
 * 
 * TerrainLoader handles loading and saving terrain data in various formats.
 * It can work with native .terrain files that preserve all data, or import/export
 * from standard formats like heightmaps and OBJ meshes.
 */
class TerrainLoader : public Component {
public:
    /**
     * @brief Constructor
     */
    TerrainLoader();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~TerrainLoader() = default;
    
    // === File Operations ===
    
    /**
     * @brief Load terrain from file
     * @param file_path Path to terrain file
     * @param options Import options
     * @param progress_callback Optional progress callback
     * @return True if loading succeeded
     */
    bool LoadTerrain(const std::string& file_path, 
                    const TerrainImportOptions& options = TerrainImportOptions(),
                    TerrainLoadProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Save terrain to file
     * @param file_path Path to save terrain file
     * @param options Export options
     * @param progress_callback Optional progress callback
     * @return True if saving succeeded
     */
    bool SaveTerrain(const std::string& file_path,
                    const TerrainExportOptions& options = TerrainExportOptions(),
                    TerrainLoadProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Check if file format is supported
     * @param file_path File path to check
     * @return True if format is supported
     */
    bool IsFormatSupported(const std::string& file_path) const;
    
    /**
     * @brief Auto-detect terrain file format
     * @param file_path File path to analyze
     * @return Detected format
     */
    TerrainFileFormat DetectFileFormat(const std::string& file_path) const;
    
    // === Terrain Component Integration ===
    
    /**
     * @brief Set associated terrain renderer
     * @param terrain_renderer Pointer to terrain renderer component
     */
    void SetTerrainRenderer(TerrainRenderer* terrain_renderer);
    
    /**
     * @brief Get associated terrain renderer
     * @return Pointer to terrain renderer component
     */
    TerrainRenderer* GetTerrainRenderer() const { return m_terrain_renderer; }
    
    /**
     * @brief Set associated terrain collider
     * @param terrain_collider Pointer to terrain collider component
     */
    void SetTerrainCollider(TerrainCollider* terrain_collider);
    
    /**
     * @brief Get associated terrain collider
     * @return Pointer to terrain collider component
     */
    TerrainCollider* GetTerrainCollider() const { return m_terrain_collider; }
    
    /**
     * @brief Set terrain data directly
     * @param data Shared pointer to terrain data
     */
    void SetTerrainData(std::shared_ptr<TerrainData> data);
    
    /**
     * @brief Get terrain data
     * @return Shared pointer to terrain data
     */
    std::shared_ptr<TerrainData> GetTerrainData() const { return m_terrain_data; }
    
    // === Auto-loading Configuration ===
    
    /**
     * @brief Set terrain file path for auto-loading
     * @param file_path Path to terrain file
     */
    void SetTerrainFilePath(const std::string& file_path);
    
    /**
     * @brief Get terrain file path
     * @return Current terrain file path
     */
    const std::string& GetTerrainFilePath() const { return m_terrain_file_path; }
    
    /**
     * @brief Enable auto-loading on component ready
     * @param auto_load Auto-load enabled state
     */
    void SetAutoLoad(bool auto_load);
    
    /**
     * @brief Get auto-load state
     * @return True if auto-load is enabled
     */
    bool GetAutoLoad() const { return m_auto_load; }
    
    // === Streaming and Chunking ===
    
    /**
     * @brief Enable streaming mode for large terrains
     * @param streaming Streaming enabled state
     */
    void SetStreamingEnabled(bool streaming);
    
    /**
     * @brief Get streaming enabled state
     * @return True if streaming is enabled
     */
    bool IsStreamingEnabled() const { return m_streaming_enabled; }
    
    /**
     * @brief Set streaming distance for chunk loading
     * @param distance Distance in world units
     */
    void SetStreamingDistance(float distance);
    
    /**
     * @brief Get streaming distance
     * @return Streaming distance in world units
     */
    float GetStreamingDistance() const { return m_streaming_distance; }
    
    /**
     * @brief Set cache size for streaming chunks
     * @param cache_size Maximum number of chunks to keep in memory
     */
    void SetCacheSize(int cache_size);
    
    /**
     * @brief Get cache size
     * @return Maximum cached chunks
     */
    int GetCacheSize() const { return m_cache_size; }
    
    // === Utility Functions ===
    
    /**
     * @brief Get supported file extensions
     * @return Vector of supported file extensions
     */
    std::vector<std::string> GetSupportedExtensions() const;
    
    /**
     * @brief Get last error message
     * @return Last error message string
     */
    const std::string& GetLastError() const { return m_last_error; }
    
    /**
     * @brief Clear last error
     */
    void ClearLastError() { m_last_error.clear(); }
    
    // === Component Lifecycle ===
    
    /**
     * @brief Called when component is ready
     */
    virtual void OnReady() override;
    
    /**
     * @brief Called every frame
     * @param delta_time Time since last frame
     */
    virtual void OnUpdate(float delta_time) override;

protected:
    /**
     * @brief Initialize export variables
     */
    virtual void InitializeExportVariables() override;
    
    /**
     * @brief Update from export variables
     */
    void UpdateFromExportVariables();
    
    /**
     * @brief Update export variables
     */
    void UpdateExportVariables();

private:
    // File configuration
    std::string m_terrain_file_path;
    bool m_auto_load;
    
    // Streaming configuration
    bool m_streaming_enabled;
    float m_streaming_distance;
    int m_cache_size;
    
    // Component references
    TerrainRenderer* m_terrain_renderer;
    TerrainCollider* m_terrain_collider;
    std::shared_ptr<TerrainData> m_terrain_data;
    
    // Error handling
    std::string m_last_error;
    
    // Internal state
    bool m_is_loading;
    bool m_is_saving;
    float m_current_progress;
    
    // Format-specific loaders
    bool LoadLupineTerrainFormat(const std::string& file_path, const TerrainImportOptions& options, TerrainLoadProgressCallback callback);
    bool LoadHeightmapFormat(const std::string& file_path, const TerrainImportOptions& options, TerrainLoadProgressCallback callback);
    bool LoadImageFormat(const std::string& file_path, const TerrainImportOptions& options, TerrainLoadProgressCallback callback);
    
    // Format-specific savers
    bool SaveLupineTerrainFormat(const std::string& file_path, const TerrainExportOptions& options, TerrainLoadProgressCallback callback);
    bool SaveOBJFormat(const std::string& file_path, const TerrainExportOptions& options, TerrainLoadProgressCallback callback);
    bool SaveHeightmapFormat(const std::string& file_path, const TerrainExportOptions& options, TerrainLoadProgressCallback callback);
    
    // Helper methods
    void SetError(const std::string& error);
    void UpdateProgress(float progress, const std::string& status, TerrainLoadProgressCallback callback);
    std::string GetFileExtension(const std::string& file_path) const;
    bool FileExists(const std::string& file_path) const;
    void NotifyComponentsOfDataChange();
    
    // Format conversion helpers
    int FileFormatToInt(TerrainFileFormat format) const;
    TerrainFileFormat IntToFileFormat(int value) const;
};

} // namespace Lupine
