#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace Lupine {

// Forward declarations
class TerrainData;
class TerrainChunk;
class Texture;

/**
 * @brief Terrain texture layer information
 */
struct TerrainTextureLayer {
    std::string texture_path;
    unsigned int texture_id = 0;
    float scale = 1.0f;
    float opacity = 1.0f;
    bool enabled = true;
    
    TerrainTextureLayer() = default;
    TerrainTextureLayer(const std::string& path, float tex_scale = 1.0f, float tex_opacity = 1.0f)
        : texture_path(path), scale(tex_scale), opacity(tex_opacity) {}
};

/**
 * @brief Terrain asset instance for scattered objects
 */
struct TerrainAssetInstance {
    std::string asset_path;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    float height_offset = 0.0f;
    bool visible = true;
    
    TerrainAssetInstance() = default;
    TerrainAssetInstance(const std::string& path, const glm::vec3& pos, 
                        const glm::vec3& rot = glm::vec3(0.0f), 
                        const glm::vec3& scl = glm::vec3(1.0f))
        : asset_path(path), position(pos), rotation(rot), scale(scl) {}
};

/**
 * @brief Render chunk for terrain rendering
 */
struct TerrainRenderChunk {
    TerrainChunk* terrain_chunk = nullptr;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    int vertex_count = 0;
    int index_count = 0;
    bool needs_update = true;

    ~TerrainRenderChunk();
};

/**
 * @brief Terrain rendering component for chunked terrain systems
 * 
 * TerrainRenderer handles rendering of height-based terrain with multiple texture layers
 * and scattered assets. Supports chunked rendering for large terrains with LOD.
 */
class TerrainRenderer : public Component {
public:
    /**
     * @brief Terrain rendering quality levels
     */
    enum class QualityLevel {
        Low,        // Reduced tessellation, basic lighting
        Medium,     // Standard tessellation, normal lighting
        High,       // High tessellation, advanced lighting
        Ultra       // Maximum tessellation, all effects
    };
    
    /**
     * @brief Constructor
     */
    TerrainRenderer();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~TerrainRenderer() = default;
    
    // === Terrain Data Management ===
    
    /**
     * @brief Set terrain data
     * @param data Shared pointer to terrain data
     */
    void SetTerrainData(std::shared_ptr<TerrainData> data);
    
    /**
     * @brief Get terrain data
     * @return Shared pointer to terrain data
     */
    std::shared_ptr<TerrainData> GetTerrainData() const { return m_terrain_data; }
    
    /**
     * @brief Create new terrain with specified dimensions
     * @param width Terrain width in world units
     * @param height Terrain height in world units
     * @param resolution Height map resolution (vertices per unit)
     */
    void CreateTerrain(float width, float height, float resolution = 1.0f);
    
    // === Chunk Management ===
    
    /**
     * @brief Set chunk size for terrain subdivision
     * @param size Chunk size in world units
     */
    void SetChunkSize(float size);
    
    /**
     * @brief Get chunk size
     * @return Chunk size in world units
     */
    float GetChunkSize() const { return m_chunk_size; }
    
    /**
     * @brief Get number of chunks
     * @return Number of terrain chunks
     */
    size_t GetChunkCount() const;
    
    /**
     * @brief Force regeneration of all chunks
     */
    void RegenerateChunks();
    
    // === Height Manipulation ===
    
    /**
     * @brief Modify terrain height at world position
     * @param world_pos World position
     * @param delta Height change amount
     * @param radius Brush radius
     * @param falloff Falloff curve (0.0 = linear, 1.0 = smooth)
     */
    void ModifyHeight(const glm::vec3& world_pos, float delta, float radius, float falloff = 0.5f);
    
    /**
     * @brief Flatten terrain height at world position
     * @param world_pos World position
     * @param target_height Target height to flatten to
     * @param radius Brush radius
     * @param strength Flattening strength (0.0 to 1.0)
     */
    void FlattenHeight(const glm::vec3& world_pos, float target_height, float radius, float strength = 1.0f);
    
    /**
     * @brief Smooth terrain height at world position
     * @param world_pos World position
     * @param radius Brush radius
     * @param strength Smoothing strength (0.0 to 1.0)
     */
    void SmoothHeight(const glm::vec3& world_pos, float radius, float strength = 1.0f);
    
    /**
     * @brief Get terrain height at world position
     * @param world_pos World position (x, z coordinates used)
     * @return Height at position
     */
    float GetHeightAtPosition(const glm::vec3& world_pos) const;
    
    /**
     * @brief Get terrain normal at world position
     * @param world_pos World position (x, z coordinates used)
     * @return Surface normal at position
     */
    glm::vec3 GetNormalAtPosition(const glm::vec3& world_pos) const;
    
    // === Texture Management ===
    
    /**
     * @brief Add texture layer
     * @param texture_path Path to texture file
     * @param scale Texture scale
     * @param opacity Initial opacity
     * @return Layer index
     */
    int AddTextureLayer(const std::string& texture_path, float scale = 1.0f, float opacity = 1.0f);
    
    /**
     * @brief Remove texture layer
     * @param layer_index Layer index to remove
     */
    void RemoveTextureLayer(int layer_index);
    
    /**
     * @brief Get texture layer count
     * @return Number of texture layers
     */
    size_t GetTextureLayerCount() const { return m_texture_layers.size(); }
    
    /**
     * @brief Get texture layer
     * @param layer_index Layer index
     * @return Texture layer reference
     */
    const TerrainTextureLayer& GetTextureLayer(int layer_index) const;
    
    /**
     * @brief Paint texture at world position
     * @param world_pos World position
     * @param layer_index Texture layer to paint
     * @param opacity Paint opacity (0.0 to 1.0)
     * @param radius Brush radius
     * @param falloff Falloff curve
     */
    void PaintTexture(const glm::vec3& world_pos, int layer_index, float opacity, float radius, float falloff = 0.5f);
    
    // === Asset Scattering ===
    
    /**
     * @brief Scatter assets at world position
     * @param world_pos World position
     * @param asset_paths List of asset paths to scatter
     * @param density Scattering density
     * @param radius Scattering radius
     * @param scale_variance Scale randomization (0.0 to 1.0)
     * @param rotation_variance Rotation randomization (0.0 to 1.0)
     * @param height_offset_range Height offset randomization range
     */
    void ScatterAssets(const glm::vec3& world_pos, const std::vector<std::string>& asset_paths,
                      float density, float radius, float scale_variance = 0.2f, 
                      float rotation_variance = 1.0f, const glm::vec2& height_offset_range = glm::vec2(-0.5f, 0.5f));
    
    /**
     * @brief Remove assets in radius
     * @param world_pos World position
     * @param radius Removal radius
     */
    void RemoveAssets(const glm::vec3& world_pos, float radius);
    
    /**
     * @brief Get asset instances
     * @return Vector of asset instances
     */
    const std::vector<TerrainAssetInstance>& GetAssetInstances() const { return m_asset_instances; }

    /**
     * @brief Update all dirty terrain chunks
     */
    void UpdateAllDirtyChunks();

    // === Rendering Properties ===
    
    /**
     * @brief Set rendering quality level
     * @param quality Quality level
     */
    void SetQualityLevel(QualityLevel quality);
    
    /**
     * @brief Get rendering quality level
     * @return Current quality level
     */
    QualityLevel GetQualityLevel() const { return m_quality_level; }
    
    /**
     * @brief Set wireframe rendering
     * @param wireframe Enable wireframe rendering
     */
    void SetWireframe(bool wireframe);
    
    /**
     * @brief Get wireframe rendering state
     * @return True if wireframe rendering is enabled
     */
    bool GetWireframe() const { return m_wireframe; }
    
    /**
     * @brief Set terrain color modulation
     * @param color Color to modulate terrain with
     */
    void SetColor(const glm::vec4& color);
    
    /**
     * @brief Get terrain color modulation
     * @return Current color modulation
     */
    const glm::vec4& GetColor() const { return m_color; }
    
    // === Component Lifecycle ===
    
    /**
     * @brief Called when component is ready
     */
    virtual void OnReady() override;
    
    /**
     * @brief Called every frame for rendering
     * @param delta_time Time since last frame
     */
    virtual void OnUpdate(float delta_time) override;
    
    /**
     * @brief Render the terrain
     */
    void Render();

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
    // Terrain data
    std::shared_ptr<TerrainData> m_terrain_data;
    std::vector<std::unique_ptr<TerrainRenderChunk>> m_chunks;
    float m_chunk_size;
    
    // Texture layers
    std::vector<TerrainTextureLayer> m_texture_layers;
    
    // Asset instances
    std::vector<TerrainAssetInstance> m_asset_instances;
    
    // Rendering properties
    QualityLevel m_quality_level;
    bool m_wireframe;
    glm::vec4 m_color;
    bool m_casts_shadows;
    bool m_receives_shadows;
    
    // Internal state
    bool m_needs_regeneration;
    bool m_chunks_dirty;
    
    // Helper methods
    void LoadTextures();
    void GenerateChunks();
    void UpdateChunk(TerrainChunk* chunk);
    glm::vec2 WorldToTerrainCoords(const glm::vec3& world_pos) const;
    glm::vec3 TerrainToWorldCoords(const glm::vec2& terrain_coords) const;
    float CalculateBrushWeight(float distance, float radius, float falloff) const;
};

} // namespace Lupine
