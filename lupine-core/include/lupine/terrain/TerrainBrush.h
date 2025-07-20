#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace Lupine {

// Forward declarations
class TerrainData;
class TerrainChunk;

/**
 * @brief Brush shape types for terrain painting
 */
enum class TerrainBrushShape {
    Circle,         // Circular brush
    Square,         // Square brush
    Diamond,        // Diamond-shaped brush
    Custom          // Custom brush from texture/heightmap
};

/**
 * @brief Brush falloff types
 */
enum class BrushFalloff {
    Linear,         // Linear falloff
    Smooth,         // Smooth (cosine) falloff
    Sharp,          // Sharp (exponential) falloff
    Constant,       // No falloff (constant strength)
    Custom          // Custom falloff curve
};

/**
 * @brief Height painting operation types
 */
enum class HeightOperation {
    Raise,          // Raise terrain height
    Lower,          // Lower terrain height
    Flatten,        // Flatten to target height
    Smooth,         // Smooth height variations
    Noise,          // Add noise to height
    Set             // Set absolute height
};

/**
 * @brief Terrain brush configuration
 */
struct BrushSettings {
    TerrainBrushShape shape = TerrainBrushShape::Circle;
    BrushFalloff falloff = BrushFalloff::Smooth;
    float size = 5.0f;                  // Brush radius in world units
    float strength = 1.0f;              // Brush strength (0.0 to 1.0)
    float falloff_curve = 0.5f;         // Falloff curve parameter (0.0 to 1.0)
    float spacing = 0.25f;              // Brush spacing for continuous painting
    bool pressure_sensitive = false;    // Enable pressure sensitivity
    std::string custom_brush_path;      // Path to custom brush texture
    
    BrushSettings() = default;
};

/**
 * @brief Height operation parameters
 */
struct HeightOperationParams {
    HeightOperation operation = HeightOperation::Raise;
    float target_height = 0.0f;        // Target height for flatten/set operations
    float noise_scale = 1.0f;          // Noise scale for noise operation
    float noise_frequency = 0.1f;      // Noise frequency for noise operation
    bool respect_max_slope = false;    // Limit modifications based on slope
    float max_slope_angle = 45.0f;     // Maximum slope angle in degrees
    
    HeightOperationParams() = default;
};

/**
 * @brief Brush stroke data for undo/redo
 */
struct BrushStroke {
    std::vector<glm::vec3> positions;   // Stroke positions
    BrushSettings brush_settings;       // Brush settings used
    HeightOperationParams operation_params; // Operation parameters
    std::vector<std::pair<glm::ivec2, float>> original_heights; // Original height data for undo
    
    BrushStroke() = default;
};

/**
 * @brief Terrain brush system for height painting
 */
class TerrainBrush {
public:
    /**
     * @brief Constructor
     */
    TerrainBrush();
    
    /**
     * @brief Destructor
     */
    ~TerrainBrush() = default;
    
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
     * @brief Set height operation parameters
     * @param params Operation parameters
     */
    void SetOperationParams(const HeightOperationParams& params);
    
    /**
     * @brief Get height operation parameters
     * @return Current operation parameters
     */
    const HeightOperationParams& GetOperationParams() const { return m_operation_params; }
    
    // === Brush Operations ===
    
    /**
     * @brief Start a new brush stroke
     * @param world_pos Starting position
     * @param terrain_data Terrain data to modify
     */
    void StartStroke(const glm::vec3& world_pos, TerrainData* terrain_data);
    
    /**
     * @brief Continue brush stroke
     * @param world_pos Current position
     * @param delta_time Time since last update
     */
    void ContinueStroke(const glm::vec3& world_pos, float delta_time);
    
    /**
     * @brief End brush stroke
     */
    void EndStroke();
    
    /**
     * @brief Apply single brush dab at position
     * @param world_pos World position
     * @param terrain_data Terrain data to modify
     * @param strength_multiplier Additional strength multiplier
     */
    void ApplyBrushDab(const glm::vec3& world_pos, TerrainData* terrain_data, float strength_multiplier = 1.0f);
    
    // === Brush Preview ===
    
    /**
     * @brief Generate brush preview mesh
     * @param world_pos Preview position
     * @param terrain_data Terrain data for height sampling
     * @param vertices Output vertices
     * @param indices Output indices
     */
    void GenerateBrushPreview(const glm::vec3& world_pos, const TerrainData* terrain_data,
                             std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices) const;
    
    /**
     * @brief Get brush affected area bounds
     * @param world_pos Brush center position
     * @return Bounds (min, max)
     */
    std::pair<glm::vec3, glm::vec3> GetBrushBounds(const glm::vec3& world_pos) const;
    
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
    const BrushStroke& GetCurrentStroke() const { return m_current_stroke; }
    
    /**
     * @brief Set stroke callback for real-time updates
     * @param callback Callback function
     */
    void SetStrokeCallback(std::function<void(const BrushStroke&)> callback);

private:
    // Brush configuration
    BrushSettings m_brush_settings;
    HeightOperationParams m_operation_params;
    
    // Stroke state
    bool m_stroke_active;
    BrushStroke m_current_stroke;
    TerrainData* m_current_terrain;
    glm::vec3 m_last_position;
    float m_accumulated_distance;
    
    // Callbacks
    std::function<void(const BrushStroke&)> m_stroke_callback;
    
    // Brush weight calculation
    float CalculateBrushWeight(const glm::vec2& offset, float brush_radius) const;
    float CalculateFalloffWeight(float distance, float radius, BrushFalloff falloff, float curve) const;
    bool IsPointInBrushShape(const glm::vec2& offset, float radius, TerrainBrushShape shape) const;
    
    // Height operations
    void ApplyHeightOperation(TerrainChunk* chunk, int local_x, int local_z, 
                             float original_height, float brush_weight, float delta_time);
    float CalculateTargetHeight(HeightOperation operation, float current_height, 
                               float target_height, float brush_weight, float delta_time);
    
    // Noise generation
    float GenerateNoise(const glm::vec2& position, float scale, float frequency) const;
    
    // Slope calculation
    float CalculateSlope(const TerrainData* terrain_data, const glm::vec3& world_pos) const;
    bool ShouldRespectSlope(const TerrainData* terrain_data, const glm::vec3& world_pos) const;
    
    // Custom brush loading
    bool LoadCustomBrush(const std::string& brush_path);
    std::vector<float> m_custom_brush_data;
    glm::ivec2 m_custom_brush_size;
    
    // Utility methods
    std::vector<TerrainChunk*> GetAffectedChunks(const glm::vec3& world_pos, TerrainData* terrain_data) const;
    void RecordOriginalHeight(const glm::ivec2& chunk_coords, int local_x, int local_z, float height);
};

/**
 * @brief Height painting undo/redo system
 */
class HeightPaintingHistory {
public:
    /**
     * @brief Constructor
     * @param max_history_size Maximum number of operations to keep
     */
    explicit HeightPaintingHistory(size_t max_history_size = 50);
    
    /**
     * @brief Destructor
     */
    ~HeightPaintingHistory() = default;
    
    /**
     * @brief Record a brush stroke for undo
     * @param stroke Brush stroke data
     */
    void RecordStroke(const BrushStroke& stroke);
    
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
    std::vector<BrushStroke> m_history;
    size_t m_current_index;
    size_t m_max_history_size;
    
    void TrimHistory();
    void ApplyStroke(const BrushStroke& stroke, TerrainData* terrain_data, bool reverse = false);
};

} // namespace Lupine
