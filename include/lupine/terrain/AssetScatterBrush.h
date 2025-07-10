#pragma once

#include "lupine/terrain/TerrainBrush.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <random>

namespace Lupine {

// Forward declarations
class TerrainData;
class TerrainChunk;
class Node3D;

/**
 * @brief Surface snapping modes for asset placement
 */
enum class SurfaceSnappingMode {
    TerrainNormals,     // Snap to terrain surface normals
    TerrainFlat,        // Snap to terrain but keep assets upright
    MeshSurface,        // Snap to arbitrary mesh surfaces
    WorldUp,            // Always align with world up vector
    Custom              // Custom alignment vector
};

/**
 * @brief Asset scatter distribution patterns
 */
enum class ScatterPattern {
    Random,             // Random distribution
    Grid,               // Grid-based distribution
    Poisson,            // Poisson disk distribution (more natural)
    Cluster,            // Clustered distribution
    Custom              // Custom pattern from texture
};

/**
 * @brief Asset scatter parameters
 */
struct AssetScatterParams {
    SurfaceSnappingMode snapping_mode = SurfaceSnappingMode::TerrainNormals;
    ScatterPattern pattern = ScatterPattern::Random;
    float density = 1.0f;                       // Assets per unit area
    float min_distance = 0.5f;                  // Minimum distance between assets
    float max_distance = 10.0f;                 // Maximum distance from brush center
    
    // Scale randomization
    glm::vec2 scale_range = glm::vec2(0.8f, 1.2f);
    bool uniform_scale = true;
    
    // Rotation randomization
    glm::vec3 rotation_range = glm::vec3(0.0f, 360.0f, 0.0f); // Min/max rotation in degrees
    bool align_to_surface = true;
    
    // Height offset
    glm::vec2 height_offset_range = glm::vec2(-0.2f, 0.2f);
    bool follow_terrain_slope = true;
    
    // Filtering
    float min_slope_angle = 0.0f;              // Minimum slope for placement
    float max_slope_angle = 45.0f;             // Maximum slope for placement
    bool avoid_water = true;                   // Avoid placing in water
    float water_level = 0.0f;                  // Water level height
    
    // LOD settings
    bool use_lod = true;
    float lod_distance_1 = 50.0f;              // Distance for LOD level 1
    float lod_distance_2 = 100.0f;             // Distance for LOD level 2
    float cull_distance = 200.0f;              // Distance to cull assets
    
    AssetScatterParams() = default;
};

/**
 * @brief Asset information for scattering
 */
struct ScatterAssetInfo {
    std::string asset_path;
    std::string lod1_path;                      // LOD level 1 mesh
    std::string lod2_path;                      // LOD level 2 mesh
    float weight = 1.0f;                        // Relative probability of selection
    glm::vec3 pivot_offset = glm::vec3(0.0f);   // Pivot point offset
    float collision_radius = 1.0f;              // Collision radius for spacing
    bool enabled = true;
    
    ScatterAssetInfo() = default;
    explicit ScatterAssetInfo(const std::string& path) : asset_path(path) {}
};

/**
 * @brief Asset scatter stroke data for undo/redo
 */
struct AssetScatterStroke {
    std::vector<glm::vec3> positions;           // Stroke positions
    BrushSettings brush_settings;               // Brush settings used
    AssetScatterParams scatter_params;          // Scatter parameters
    std::vector<ScatterAssetInfo> asset_palette; // Assets used in stroke
    std::vector<uint32_t> created_asset_ids;    // IDs of assets created in this stroke
    
    AssetScatterStroke() = default;
};

/**
 * @brief Asset scatter brush system for terrain asset placement
 */
class AssetScatterBrush {
public:
    /**
     * @brief Constructor
     */
    AssetScatterBrush();
    
    /**
     * @brief Destructor
     */
    ~AssetScatterBrush() = default;
    
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
     * @brief Set scatter parameters
     * @param params Scatter parameters
     */
    void SetScatterParams(const AssetScatterParams& params);
    
    /**
     * @brief Get scatter parameters
     * @return Current scatter parameters
     */
    const AssetScatterParams& GetScatterParams() const { return m_scatter_params; }
    
    // === Asset Palette Management ===
    
    /**
     * @brief Add asset to scatter palette
     * @param asset_info Asset information
     * @return Asset index in palette
     */
    int AddAsset(const ScatterAssetInfo& asset_info);
    
    /**
     * @brief Remove asset from palette
     * @param asset_index Asset index to remove
     */
    void RemoveAsset(int asset_index);
    
    /**
     * @brief Get asset count in palette
     * @return Number of assets in palette
     */
    size_t GetAssetCount() const { return m_asset_palette.size(); }
    
    /**
     * @brief Get asset info
     * @param asset_index Asset index
     * @return Asset information
     */
    const ScatterAssetInfo& GetAsset(int asset_index) const;
    
    /**
     * @brief Update asset info
     * @param asset_index Asset index
     * @param asset_info New asset information
     */
    void UpdateAsset(int asset_index, const ScatterAssetInfo& asset_info);
    
    /**
     * @brief Clear all assets from palette
     */
    void ClearAssets() { m_asset_palette.clear(); }
    
    // === Scattering Operations ===
    
    /**
     * @brief Start a new asset scatter stroke
     * @param world_pos Starting position
     * @param terrain_data Terrain data for surface snapping
     * @param target_node Target node to add assets to (optional)
     */
    void StartStroke(const glm::vec3& world_pos, TerrainData* terrain_data, Node3D* target_node = nullptr);
    
    /**
     * @brief Continue asset scatter stroke
     * @param world_pos Current position
     * @param delta_time Time since last update
     */
    void ContinueStroke(const glm::vec3& world_pos, float delta_time);
    
    /**
     * @brief End asset scatter stroke
     */
    void EndStroke();
    
    /**
     * @brief Apply single scatter dab at position
     * @param world_pos World position
     * @param terrain_data Terrain data for surface snapping
     * @param target_node Target node to add assets to
     * @param strength_multiplier Additional strength multiplier
     */
    void ApplyScatterDab(const glm::vec3& world_pos, TerrainData* terrain_data, 
                        Node3D* target_node, float strength_multiplier = 1.0f);
    
    /**
     * @brief Erase assets in brush area
     * @param world_pos World position
     * @param terrain_data Terrain data
     * @param target_node Node containing assets to erase
     * @param radius Erase radius
     */
    void EraseAssets(const glm::vec3& world_pos, TerrainData* terrain_data, 
                    Node3D* target_node, float radius);
    
    // === Surface Snapping ===
    
    /**
     * @brief Set surface snapping target mesh
     * @param target_mesh Target mesh for surface snapping
     */
    void SetSurfaceSnappingTarget(Node3D* target_mesh);
    
    /**
     * @brief Get surface snapping target
     * @return Target mesh for surface snapping
     */
    Node3D* GetSurfaceSnappingTarget() const { return m_surface_target; }
    
    /**
     * @brief Calculate surface position and normal
     * @param world_pos Input world position
     * @param terrain_data Terrain data
     * @param surface_pos Output surface position
     * @param surface_normal Output surface normal
     * @return True if valid surface found
     */
    bool CalculateSurfaceSnapping(const glm::vec3& world_pos, TerrainData* terrain_data,
                                 glm::vec3& surface_pos, glm::vec3& surface_normal) const;
    
    // === Stroke Management ===
    
    /**
     * @brief Check if currently scattering
     * @return True if stroke is active
     */
    bool IsStrokeActive() const { return m_stroke_active; }
    
    /**
     * @brief Get current stroke data
     * @return Current stroke
     */
    const AssetScatterStroke& GetCurrentStroke() const { return m_current_stroke; }
    
    /**
     * @brief Set stroke callback for real-time updates
     * @param callback Callback function
     */
    void SetStrokeCallback(std::function<void(const AssetScatterStroke&)> callback);

private:
    // Brush configuration
    BrushSettings m_brush_settings;
    AssetScatterParams m_scatter_params;
    
    // Asset palette
    std::vector<ScatterAssetInfo> m_asset_palette;
    
    // Surface snapping
    Node3D* m_surface_target;
    
    // Stroke state
    bool m_stroke_active;
    AssetScatterStroke m_current_stroke;
    TerrainData* m_current_terrain;
    Node3D* m_current_target_node;
    glm::vec3 m_last_position;
    float m_accumulated_distance;
    
    // Random number generation
    mutable std::mt19937 m_random_generator;
    mutable std::uniform_real_distribution<float> m_uniform_dist;
    
    // Callbacks
    std::function<void(const AssetScatterStroke&)> m_stroke_callback;
    
    // Asset placement
    void PlaceAssetsInArea(const glm::vec3& center_pos, float radius, TerrainData* terrain_data, 
                          Node3D* target_node, float strength_multiplier);
    std::vector<glm::vec3> GenerateScatterPositions(const glm::vec3& center_pos, float radius, 
                                                    int target_count) const;
    bool IsValidPlacementPosition(const glm::vec3& world_pos, TerrainData* terrain_data,
                                 const std::vector<glm::vec3>& existing_positions, 
                                 float min_distance) const;
    
    // Asset selection and creation
    const ScatterAssetInfo& SelectRandomAsset() const;
    Node3D* CreateAssetInstance(const ScatterAssetInfo& asset_info, const glm::vec3& position,
                               const glm::vec3& rotation, const glm::vec3& scale, 
                               Node3D* parent_node);
    
    // Transformation calculation
    glm::vec3 CalculateAssetRotation(const glm::vec3& surface_normal, const ScatterAssetInfo& asset_info) const;
    glm::vec3 CalculateAssetScale(const ScatterAssetInfo& asset_info) const;
    
    // Utility methods
    float CalculateBrushWeight(const glm::vec2& offset, float brush_radius) const;
    bool CheckSlope(const glm::vec3& surface_normal) const;
    bool CheckWaterLevel(const glm::vec3& position) const;
    uint32_t GenerateAssetId();
    
    // Pattern generation
    std::vector<glm::vec3> GenerateRandomPattern(const glm::vec3& center, float radius, int count) const;
    std::vector<glm::vec3> GenerateGridPattern(const glm::vec3& center, float radius, int count) const;
    std::vector<glm::vec3> GeneratePoissonPattern(const glm::vec3& center, float radius, int count) const;
    std::vector<glm::vec3> GenerateClusterPattern(const glm::vec3& center, float radius, int count) const;
};

/**
 * @brief Asset scatter undo/redo system
 */
class AssetScatterHistory {
public:
    /**
     * @brief Constructor
     * @param max_history_size Maximum number of operations to keep
     */
    explicit AssetScatterHistory(size_t max_history_size = 50);
    
    /**
     * @brief Destructor
     */
    ~AssetScatterHistory() = default;
    
    /**
     * @brief Record an asset scatter stroke for undo
     * @param stroke Asset scatter stroke data
     */
    void RecordStroke(const AssetScatterStroke& stroke);
    
    /**
     * @brief Undo last operation
     * @param target_node Node containing scattered assets
     * @return True if undo was performed
     */
    bool Undo(Node3D* target_node);
    
    /**
     * @brief Redo last undone operation
     * @param target_node Node containing scattered assets
     * @return True if redo was performed
     */
    bool Redo(Node3D* target_node);
    
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
    std::vector<AssetScatterStroke> m_history;
    size_t m_current_index;
    size_t m_max_history_size;
    
    void TrimHistory();
    void ApplyStroke(const AssetScatterStroke& stroke, Node3D* target_node, bool reverse = false);
};

} // namespace Lupine
