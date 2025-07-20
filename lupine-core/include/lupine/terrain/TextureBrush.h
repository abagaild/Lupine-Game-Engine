#pragma once

#include "lupine/terrain/TerrainBrush.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

namespace Lupine {

// Forward declarations
class TerrainData;
class TerrainChunk;

/**
 * @brief Texture blend modes for painting
 */
enum class TextureBlendMode {
    Replace,        // Replace existing texture weights
    Add,            // Add to existing texture weights
    Subtract,       // Subtract from existing texture weights
    Multiply,       // Multiply existing texture weights
    Overlay,        // Overlay blend mode
    SoftLight,      // Soft light blend mode
    HardLight       // Hard light blend mode
};

/**
 * @brief Texture painting operation parameters
 */
struct TexturePaintParams {
    int target_layer = 0;                       // Target texture layer index
    TextureBlendMode blend_mode = TextureBlendMode::Replace;
    float opacity = 1.0f;                       // Paint opacity (0.0 to 1.0)
    bool normalize_weights = true;              // Normalize all layer weights after painting
    bool respect_existing_weights = false;     // Consider existing weights when painting
    float flow_rate = 1.0f;                    // Flow rate for continuous painting
    
    TexturePaintParams() = default;
};

/**
 * @brief Texture layer information for painting
 */
struct TextureLayerInfo {
    std::string texture_path;
    std::string normal_map_path;
    std::string roughness_map_path;
    std::string metallic_map_path;
    float tiling_scale = 1.0f;
    float opacity = 1.0f;
    bool enabled = true;
    glm::vec4 color_tint = glm::vec4(1.0f);
    
    TextureLayerInfo() = default;
    explicit TextureLayerInfo(const std::string& path) : texture_path(path) {}
};

/**
 * @brief Texture painting stroke data for undo/redo
 */
struct TexturePaintStroke {
    std::vector<glm::vec3> positions;           // Stroke positions
    BrushSettings brush_settings;               // Brush settings used
    TexturePaintParams paint_params;            // Paint parameters
    std::vector<std::pair<glm::ivec2, std::vector<float>>> original_weights; // Original texture weights for undo
    
    TexturePaintStroke() = default;
};

/**
 * @brief Texture brush system for terrain texture painting
 */
class TextureBrush {
public:
    /**
     * @brief Constructor
     */
    TextureBrush();
    
    /**
     * @brief Destructor
     */
    ~TextureBrush() = default;
    
    // === Brush Configuration ===
    
    /**
     * @brief Set brush settings
     * @param settings Brush settings
     */
    void SetBrushSettings(const BrushSettings& settings);
    
    /**
     * @brief Get brush settings
     * @return Current brush settings
     */
    const BrushSettings& GetBrushSettings() const { return m_brush_settings; }
    
    /**
     * @brief Set texture paint parameters
     * @param params Paint parameters
     */
    void SetPaintParams(const TexturePaintParams& params);
    
    /**
     * @brief Get texture paint parameters
     * @return Current paint parameters
     */
    const TexturePaintParams& GetPaintParams() const { return m_paint_params; }
    
    // === Texture Layer Management ===
    
    /**
     * @brief Add texture layer
     * @param layer_info Texture layer information
     * @return Layer index
     */
    int AddTextureLayer(const TextureLayerInfo& layer_info);
    
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
     * @brief Get texture layer info
     * @param layer_index Layer index
     * @return Texture layer information
     */
    const TextureLayerInfo& GetTextureLayer(int layer_index) const;
    
    /**
     * @brief Update texture layer
     * @param layer_index Layer index
     * @param layer_info New layer information
     */
    void UpdateTextureLayer(int layer_index, const TextureLayerInfo& layer_info);
    
    // === Painting Operations ===
    
    /**
     * @brief Start a new texture paint stroke
     * @param world_pos Starting position
     * @param terrain_data Terrain data to modify
     */
    void StartStroke(const glm::vec3& world_pos, TerrainData* terrain_data);
    
    /**
     * @brief Continue texture paint stroke
     * @param world_pos Current position
     * @param delta_time Time since last update
     */
    void ContinueStroke(const glm::vec3& world_pos, float delta_time);
    
    /**
     * @brief End texture paint stroke
     */
    void EndStroke();
    
    /**
     * @brief Apply single texture paint dab at position
     * @param world_pos World position
     * @param terrain_data Terrain data to modify
     * @param strength_multiplier Additional strength multiplier
     */
    void ApplyTextureDab(const glm::vec3& world_pos, TerrainData* terrain_data, float strength_multiplier = 1.0f);
    
    // === Texture Sampling ===
    
    /**
     * @brief Get texture weights at world position
     * @param world_pos World position
     * @param terrain_data Terrain data
     * @return Vector of texture weights
     */
    std::vector<float> GetTextureWeightsAt(const glm::vec3& world_pos, const TerrainData* terrain_data) const;
    
    /**
     * @brief Get dominant texture layer at position
     * @param world_pos World position
     * @param terrain_data Terrain data
     * @return Dominant texture layer index
     */
    int GetDominantTextureAt(const glm::vec3& world_pos, const TerrainData* terrain_data) const;
    
    // === Stroke Management ===
    
    /**
     * @brief Check if currently painting
     * @return True if stroke is active
     */
    bool IsStrokeActive() const { return m_stroke_active; }
    
    /**
     * @brief Get current stroke data
     * @return Current stroke
     */
    const TexturePaintStroke& GetCurrentStroke() const { return m_current_stroke; }
    
    /**
     * @brief Set stroke callback for real-time updates
     * @param callback Callback function
     */
    void SetStrokeCallback(std::function<void(const TexturePaintStroke&)> callback);

private:
    // Brush configuration
    BrushSettings m_brush_settings;
    TexturePaintParams m_paint_params;
    
    // Texture layers
    std::vector<TextureLayerInfo> m_texture_layers;
    
    // Stroke state
    bool m_stroke_active;
    TexturePaintStroke m_current_stroke;
    TerrainData* m_current_terrain;
    glm::vec3 m_last_position;
    float m_accumulated_distance;
    float m_accumulated_flow;
    
    // Callbacks
    std::function<void(const TexturePaintStroke&)> m_stroke_callback;
    
    // Painting operations
    void ApplyTexturePaint(TerrainChunk* chunk, int local_x, int local_z, 
                          const std::vector<float>& original_weights, float brush_weight, float delta_time);
    std::vector<float> CalculateNewWeights(const std::vector<float>& original_weights, 
                                          float brush_weight, float delta_time) const;
    void NormalizeWeights(std::vector<float>& weights) const;
    float ApplyBlendMode(float original_weight, float paint_weight, TextureBlendMode blend_mode) const;
    
    // Utility methods
    std::vector<TerrainChunk*> GetAffectedChunks(const glm::vec3& world_pos, TerrainData* terrain_data) const;
    void RecordOriginalWeights(const glm::ivec2& chunk_coords, int local_x, int local_z, 
                              const std::vector<float>& weights);
    float CalculateBrushWeight(const glm::vec2& offset, float brush_radius) const;
};

/**
 * @brief Texture painting undo/redo system
 */
class TexturePaintingHistory {
public:
    /**
     * @brief Constructor
     * @param max_history_size Maximum number of operations to keep
     */
    explicit TexturePaintingHistory(size_t max_history_size = 50);
    
    /**
     * @brief Destructor
     */
    ~TexturePaintingHistory() = default;
    
    /**
     * @brief Record a texture paint stroke for undo
     * @param stroke Texture paint stroke data
     */
    void RecordStroke(const TexturePaintStroke& stroke);
    
    /**
     * @brief Undo last operation
     * @param terrain_data Terrain data to modify
     * @return True if undo was performed
     */
    bool Undo(TerrainData* terrain_data);
    
    /**
     * @brief Redo last undone operation
     * @param terrain_data Terrain data to modify
     * @return True if redo was performed
     */
    bool Redo(TerrainData* terrain_data);
    
    /**
     * @brief Check if undo is available
     * @return True if undo is possible
     */
    bool CanUndo() const { return m_current_index > 0; }
    
    /**
     * @brief Check if redo is available
     * @return True if redo is possible
     */
    bool CanRedo() const { return m_current_index < m_history.size(); }
    
    /**
     * @brief Clear all history
     */
    void Clear();
    
    /**
     * @brief Get history size
     * @return Number of operations in history
     */
    size_t GetHistorySize() const { return m_history.size(); }

private:
    std::vector<TexturePaintStroke> m_history;
    size_t m_current_index;
    size_t m_max_history_size;
    
    void TrimHistory();
    void ApplyStroke(const TexturePaintStroke& stroke, TerrainData* terrain_data, bool reverse = false);
};

} // namespace Lupine
