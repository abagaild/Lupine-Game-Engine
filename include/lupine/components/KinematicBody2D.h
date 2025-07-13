#pragma once

#include "lupine/core/Component.h"
#include "lupine/physics/PhysicsManager.h"
#include <glm/glm.hpp>

namespace Lupine {

/**
 * @brief 2D Kinematic Body Component
 * 
 * Kinematic bodies are physics bodies that can be moved programmatically
 * but are not affected by forces or gravity. They can push dynamic bodies
 * and detect collisions, making them ideal for player controllers and
 * moving platforms.
 */
class KinematicBody2D : public Component {
public:
    /**
     * @brief Constructor
     */
    KinematicBody2D();
    
    /**
     * @brief Destructor
     */
    virtual ~KinematicBody2D();
    
    // === Shape Properties ===
    
    /**
     * @brief Set collision shape type
     * @param type Shape type (Box, Circle, etc.)
     */
    void SetShapeType(CollisionShapeType type);
    
    /**
     * @brief Get collision shape type
     * @return Current shape type
     */
    CollisionShapeType GetShapeType() const { return m_shape_type; }
    
    /**
     * @brief Set shape size
     * @param size Shape size (width/height for box, radius for circle)
     */
    void SetSize(const glm::vec2& size);
    
    /**
     * @brief Get shape size
     * @return Current shape size
     */
    const glm::vec2& GetSize() const { return m_size; }
    
    // === Movement ===
    
    /**
     * @brief Move the body by a given velocity
     * @param velocity Movement velocity
     * @param delta_time Time step
     * @return True if movement was successful
     */
    bool MoveAndSlide(const glm::vec2& velocity, float delta_time);
    
    /**
     * @brief Move the body and detect collisions
     * @param velocity Movement velocity
     * @param delta_time Time step
     * @return Collision information
     */
    bool MoveAndCollide(const glm::vec2& velocity, float delta_time);
    
    /**
     * @brief Set the body's velocity
     * @param velocity New velocity
     */
    void SetVelocity(const glm::vec2& velocity);
    
    /**
     * @brief Get the body's current velocity
     * @return Current velocity
     */
    const glm::vec2& GetVelocity() const { return m_velocity; }
    
    // === Collision Properties ===
    
    /**
     * @brief Set physics material
     * @param material Material properties
     */
    void SetMaterial(const PhysicsMaterial& material);
    
    /**
     * @brief Get physics material
     * @return Current material
     */
    const PhysicsMaterial& GetMaterial() const { return m_material; }
    
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
     * @brief Set collision mask (which layers this body collides with)
     * @param mask Collision mask bitfield
     */
    void SetCollisionMask(int mask);
    
    /**
     * @brief Get collision mask
     * @return Current collision mask
     */
    int GetCollisionMask() const { return m_collision_mask; }
    
    /**
     * @brief Set collision callback
     * @param callback Function to call on collision
     */
    void SetCollisionCallback(CollisionCallback callback);
    
    // === Queries ===
    
    /**
     * @brief Check if body is on floor
     * @param floor_normal Expected floor normal (default: up)
     * @return True if on floor
     */
    bool IsOnFloor(const glm::vec2& floor_normal = glm::vec2(0.0f, -1.0f)) const;
    
    /**
     * @brief Check if body is on wall
     * @param wall_normal Expected wall normal
     * @return True if on wall
     */
    bool IsOnWall(const glm::vec2& wall_normal) const;
    
    /**
     * @brief Check if body is on ceiling
     * @param ceiling_normal Expected ceiling normal (default: down)
     * @return True if on ceiling
     */
    bool IsOnCeiling(const glm::vec2& ceiling_normal = glm::vec2(0.0f, 1.0f)) const;
    
    /**
     * @brief Get the last collision normal
     * @return Normal vector of last collision
     */
    const glm::vec2& GetLastCollisionNormal() const { return m_last_collision_normal; }
    
    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "KinematicBody2D"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "Physics"; }

    // Component lifecycle methods
    virtual void OnReady() override;
    virtual void OnUpdate(float delta_time) override;
    virtual void OnDestroy() override;

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
    CollisionShapeType m_shape_type;
    glm::vec2 m_size;
    PhysicsMaterial m_material;
    int m_collision_layer;
    int m_collision_mask;
    
    glm::vec2 m_velocity;
    glm::vec2 m_last_collision_normal;
    bool m_on_floor;
    bool m_on_wall;
    bool m_on_ceiling;
    
    PhysicsBody2D* m_physics_body;
    CollisionCallback m_collision_callback;
    bool m_needs_recreation;
    
    /**
     * @brief Recreate the physics body
     */
    void RecreatePhysicsBody();
    
    /**
     * @brief Handle collision detection
     */
    void OnCollision(Node2D* other_node, bool entered);
    
    /**
     * @brief Convert shape type enum to int for export
     */
    int ShapeTypeToInt(CollisionShapeType type) const;
    
    /**
     * @brief Convert int to shape type enum from export
     */
    CollisionShapeType IntToShapeType(int value) const;
};

} // namespace Lupine
