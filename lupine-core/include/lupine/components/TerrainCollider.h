#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace Lupine {

// Forward declarations
class TerrainData;
class TerrainRenderer;

/**
 * @brief Terrain collision shape types
 */
enum class TerrainCollisionType {
    None,           // No collision
    Heightfield,    // Heightfield collision (most efficient for terrain)
    Trimesh,        // Triangle mesh collision (accurate but slower)
    ConvexHull,     // Convex hull approximation (fast but less accurate)
    Simplified      // Simplified mesh with reduced detail
};

/**
 * @brief Terrain collision component for physics interaction
 * 
 * TerrainCollider provides collision detection for terrain surfaces.
 * It can generate collision meshes from terrain height data and update
 * them when the terrain is modified.
 */
class TerrainCollider : public Component {
public:
    /**
     * @brief Constructor
     */
    TerrainCollider();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~TerrainCollider() = default;
    
    // === Collision Configuration ===
    
    /**
     * @brief Set collision type
     * @param type Collision shape type
     */
    void SetCollisionType(TerrainCollisionType type);
    
    /**
     * @brief Get collision type
     * @return Current collision type
     */
    TerrainCollisionType GetCollisionType() const { return m_collision_type; }
    
    /**
     * @brief Enable or disable collision
     * @param enabled Collision enabled state
     */
    void SetCollisionEnabled(bool enabled);
    
    /**
     * @brief Get collision enabled state
     * @return True if collision is enabled
     */
    bool IsCollisionEnabled() const { return m_collision_enabled; }
    
    /**
     * @brief Set collision layer
     * @param layer Collision layer bitmask
     */
    void SetCollisionLayer(unsigned int layer);
    
    /**
     * @brief Get collision layer
     * @return Collision layer bitmask
     */
    unsigned int GetCollisionLayer() const { return m_collision_layer; }
    
    /**
     * @brief Set collision mask
     * @param mask Collision mask bitmask
     */
    void SetCollisionMask(unsigned int mask);
    
    /**
     * @brief Get collision mask
     * @return Collision mask bitmask
     */
    unsigned int GetCollisionMask() const { return m_collision_mask; }
    
    // === Collision Properties ===
    
    /**
     * @brief Set collision margin
     * @param margin Collision margin for physics stability
     */
    void SetCollisionMargin(float margin);
    
    /**
     * @brief Get collision margin
     * @return Collision margin
     */
    float GetCollisionMargin() const { return m_collision_margin; }
    
    /**
     * @brief Set friction coefficient
     * @param friction Friction value (0.0 to 1.0)
     */
    void SetFriction(float friction);
    
    /**
     * @brief Get friction coefficient
     * @return Friction value
     */
    float GetFriction() const { return m_friction; }
    
    /**
     * @brief Set restitution (bounciness)
     * @param restitution Restitution value (0.0 to 1.0)
     */
    void SetRestitution(float restitution);
    
    /**
     * @brief Get restitution
     * @return Restitution value
     */
    float GetRestitution() const { return m_restitution; }
    
    // === Mesh Generation ===
    
    /**
     * @brief Set mesh simplification level for performance
     * @param level Simplification level (0.0 = no simplification, 1.0 = maximum)
     */
    void SetSimplificationLevel(float level);
    
    /**
     * @brief Get mesh simplification level
     * @return Simplification level
     */
    float GetSimplificationLevel() const { return m_simplification_level; }
    
    /**
     * @brief Set collision mesh resolution
     * @param resolution Vertices per world unit for collision mesh
     */
    void SetCollisionResolution(float resolution);
    
    /**
     * @brief Get collision mesh resolution
     * @return Collision mesh resolution
     */
    float GetCollisionResolution() const { return m_collision_resolution; }
    
    /**
     * @brief Force regeneration of collision mesh
     */
    void RegenerateCollisionMesh();
    
    /**
     * @brief Update collision mesh for a specific region
     * @param min_bounds Minimum bounds of region to update
     * @param max_bounds Maximum bounds of region to update
     */
    void UpdateCollisionRegion(const glm::vec3& min_bounds, const glm::vec3& max_bounds);
    
    // === Terrain Integration ===
    
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
     * @brief Set terrain data directly
     * @param data Shared pointer to terrain data
     */
    void SetTerrainData(std::shared_ptr<TerrainData> data);
    
    /**
     * @brief Get terrain data
     * @return Shared pointer to terrain data
     */
    std::shared_ptr<TerrainData> GetTerrainData() const { return m_terrain_data; }
    
    // === Collision Queries ===
    
    /**
     * @brief Raycast against terrain
     * @param ray_origin Ray origin in world space
     * @param ray_direction Ray direction (normalized)
     * @param max_distance Maximum ray distance
     * @param hit_point Output hit point
     * @param hit_normal Output hit normal
     * @return True if ray hit terrain
     */
    bool Raycast(const glm::vec3& ray_origin, const glm::vec3& ray_direction, 
                float max_distance, glm::vec3& hit_point, glm::vec3& hit_normal) const;
    
    /**
     * @brief Check if point is inside terrain bounds
     * @param point World space point
     * @return True if point is within terrain bounds
     */
    bool IsPointInBounds(const glm::vec3& point) const;
    
    /**
     * @brief Get terrain height at world position (collision-based)
     * @param world_pos World position (x, z coordinates used)
     * @return Height at position, or NaN if outside bounds
     */
    float GetHeightAtPosition(const glm::vec3& world_pos) const;
    
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
    
    /**
     * @brief Called during physics processing
     * @param delta_time Time since last physics update
     */
    virtual void OnPhysicsProcess(float delta_time) override;

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
    // Collision configuration
    TerrainCollisionType m_collision_type;
    bool m_collision_enabled;
    unsigned int m_collision_layer;
    unsigned int m_collision_mask;
    
    // Collision properties
    float m_collision_margin;
    float m_friction;
    float m_restitution;
    
    // Mesh generation settings
    float m_simplification_level;
    float m_collision_resolution;
    
    // Terrain data
    std::shared_ptr<TerrainData> m_terrain_data;
    TerrainRenderer* m_terrain_renderer;
    
    // Physics data (platform-specific handles)
    void* m_collision_shape;       // Physics engine collision shape
    void* m_rigid_body;           // Physics engine rigid body
    
    // Internal state
    bool m_collision_mesh_dirty;
    bool m_needs_physics_update;
    glm::vec3 m_terrain_bounds_min;
    glm::vec3 m_terrain_bounds_max;
    
    // Helper methods
    void CreateCollisionShape();
    void UpdateCollisionShape();
    void DestroyCollisionShape();
    void GenerateHeightfieldCollision();
    void GenerateTrimeshCollision();
    void GenerateConvexHullCollision();
    void GenerateSimplifiedCollision();
    void UpdatePhysicsBody();
    std::vector<glm::vec3> GenerateCollisionVertices() const;
    std::vector<unsigned int> GenerateCollisionIndices() const;
    
    // Collision type conversion helpers
    int CollisionTypeToInt(TerrainCollisionType type) const;
    TerrainCollisionType IntToCollisionType(int value) const;
};

} // namespace Lupine
