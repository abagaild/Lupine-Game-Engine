#pragma once

#include "lupine/core/Component.h"
#include "lupine/physics/PhysicsManager.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace Lupine {

/**
 * @brief 3D Collision Mesh Component
 * 
 * Defines custom mesh collision shapes for 3D physics bodies.
 * Can be used with RigidBody3D, KinematicBody3D, or Area3D components.
 */
class CollisionMesh3D : public Component {
public:
    /**
     * @brief Mesh collision type
     */
    enum class MeshType {
        Convex,     // Convex hull (fast, suitable for dynamic bodies)
        Trimesh,    // Triangle mesh (accurate, only for static bodies)
        Simplified  // Simplified convex approximation
    };
    
    /**
     * @brief Constructor
     */
    CollisionMesh3D();
    
    /**
     * @brief Destructor
     */
    virtual ~CollisionMesh3D() = default;
    
    // === Mesh Properties ===
    
    /**
     * @brief Set mesh file path
     * @param path Path to mesh file (.obj, .fbx, etc.)
     */
    void SetMeshPath(const std::string& path);
    
    /**
     * @brief Get mesh file path
     * @return Current mesh path
     */
    const std::string& GetMeshPath() const { return m_mesh_path; }
    
    /**
     * @brief Set mesh collision type
     * @param type Collision mesh type
     */
    void SetMeshType(MeshType type);
    
    /**
     * @brief Get mesh collision type
     * @return Current mesh type
     */
    MeshType GetMeshType() const { return m_mesh_type; }
    
    /**
     * @brief Set mesh scale
     * @param scale Scale factor for the collision mesh
     */
    void SetScale(const glm::vec3& scale);
    
    /**
     * @brief Get mesh scale
     * @return Current mesh scale
     */
    const glm::vec3& GetScale() const { return m_scale; }
    
    /**
     * @brief Set mesh offset from node center
     * @param offset Local offset
     */
    void SetOffset(const glm::vec3& offset);
    
    /**
     * @brief Get mesh offset
     * @return Current offset
     */
    const glm::vec3& GetOffset() const { return m_offset; }
    
    /**
     * @brief Set whether shape is a trigger (no collision response)
     * @param trigger True for trigger, false for solid collision
     */
    void SetTrigger(bool trigger);
    
    /**
     * @brief Get whether shape is a trigger
     * @return True if trigger
     */
    bool IsTrigger() const { return m_is_trigger; }
    
    /**
     * @brief Set collision layer
     * @param layer Collision layer (0-31)
     */
    void SetCollisionLayer(int layer);
    
    /**
     * @brief Get collision layer
     * @return Current collision layer
     */
    int GetCollisionLayer() const { return m_collision_layer; }
    
    /**
     * @brief Set collision mask (which layers this shape collides with)
     * @param mask Collision mask bitfield
     */
    void SetCollisionMask(int mask);
    
    /**
     * @brief Get collision mask
     * @return Current collision mask
     */
    int GetCollisionMask() const { return m_collision_mask; }
    
    /**
     * @brief Check if shape collides with given layer
     * @param layer Layer to check
     * @return True if collision enabled
     */
    bool CollidesWithLayer(int layer) const;
    
    /**
     * @brief Check if mesh is loaded and valid
     * @return True if mesh is ready for physics
     */
    bool IsMeshLoaded() const { return m_mesh_loaded; }
    
    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "CollisionMesh3D"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "Physics"; }

    // Component lifecycle methods
    virtual void OnReady() override;
    virtual void OnUpdate(float delta_time) override;

protected:
    /**
     * @brief Initialize export variables
     */
    virtual void InitializeExportVariables() override;
    
    /**
     * @brief Update export variables from current values
     */
    void UpdateExportVariables();
    
    /**
     * @brief Update current values from export variables
     */
    void UpdateFromExportVariables();

private:
    std::string m_mesh_path;
    MeshType m_mesh_type;
    glm::vec3 m_scale;
    glm::vec3 m_offset;
    bool m_is_trigger;
    int m_collision_layer;
    int m_collision_mask;
    
    bool m_mesh_loaded;
    bool m_needs_update;
    
    /**
     * @brief Load collision mesh from file
     */
    void LoadMesh();
    
    /**
     * @brief Convert mesh type enum to int for export
     */
    int MeshTypeToInt(MeshType type) const;
    
    /**
     * @brief Convert int to mesh type enum from export
     */
    MeshType IntToMeshType(int value) const;
};

} // namespace Lupine
