#pragma once

#include "lupine/core/Component.h"
#include "lupine/physics/PhysicsManager.h"
#include <glm/glm.hpp>

namespace Lupine {

/**
 * @brief 3D Kinematic Body Component
 *
 * Kinematic bodies are physics bodies that can be moved programmatically
 * but are not affected by forces or gravity. They can push dynamic bodies
 * and detect collisions, making them ideal for player controllers and
 * moving platforms.
 */
class KinematicBody3D : public Component {
public:
    /**
     * @brief Constructor
     */
    KinematicBody3D();

    /**
     * @brief Destructor
     */
    virtual ~KinematicBody3D();

    // === Shape Properties ===

    /**
     * @brief Set collision shape type
     * @param type Shape type (Box, Sphere, etc.)
     */
    void SetShapeType(CollisionShapeType type);

    /**
     * @brief Get collision shape type
     * @return Current shape type
     */
    CollisionShapeType GetShapeType() const { return m_shape_type; }

    /**
     * @brief Set shape size
     * @param size Shape size (width/height/depth for box, radius for sphere)
     */
    void SetSize(const glm::vec3& size);

    /**
     * @brief Get shape size
     * @return Current shape size
     */
    const glm::vec3& GetSize() const { return m_size; }

    // === Movement ===

    /**
     * @brief Move the body by a given velocity
     * @param velocity Movement velocity
     * @param delta_time Time step
     * @return True if movement was successful
     */
    bool MoveAndSlide(const glm::vec3& velocity, float delta_time);

    /**
     * @brief Move the body and detect collisions
     * @param velocity Movement velocity
     * @param delta_time Time step
     * @return Collision information
     */
    bool MoveAndCollide(const glm::vec3& velocity, float delta_time);

    /**
     * @brief Set velocity directly
     * @param velocity New velocity
     */
    void SetVelocity(const glm::vec3& velocity);

    /**
     * @brief Get current velocity
     * @return Current velocity
     */
    const glm::vec3& GetVelocity() const { return m_velocity; }

    // === Collision Detection ===

    /**
     * @brief Check if the body is on the floor
     * @return True if on floor
     */
    bool IsOnFloor() const { return m_on_floor; }

    /**
     * @brief Check if the body is on a wall
     * @return True if on wall
     */
    bool IsOnWall() const { return m_on_wall; }

    /**
     * @brief Check if the body is on the ceiling
     * @return True if on ceiling
     */
    bool IsOnCeiling() const { return m_on_ceiling; }

    /**
     * @brief Get the last collision normal
     * @return Normal vector of last collision
     */
    const glm::vec3& GetLastCollisionNormal() const { return m_last_collision_normal; }

    // === Collision Callbacks ===

    /**
     * @brief Set collision callback function
     * @param callback Function to call when collision occurs
     */
    void SetCollisionCallback(CollisionCallback callback);

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "KinematicBody3D"; }

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
     * @brief Update from export variables
     */
    void UpdateFromExportVariables();

    /**
     * @brief Perform swept AABB collision test as fallback
     * @param start_pos Starting position
     * @param end_pos Ending position
     * @param movement Movement vector
     * @param hit_body Output: body that was hit
     * @param hit_point Output: collision point
     * @param hit_normal Output: collision normal
     * @param hit_fraction Output: fraction of movement before collision
     * @return True if collision detected
     */
    bool PerformSweptAABBTest(const glm::vec3& start_pos, const glm::vec3& end_pos, const glm::vec3& movement,
                             PhysicsBody3D*& hit_body, glm::vec3& hit_point, glm::vec3& hit_normal, float& hit_fraction);

    /**
     * @brief Perform swept AABB collision between moving and static AABB
     * @param moving_pos Position of moving AABB
     * @param moving_size Size of moving AABB
     * @param static_min Minimum corner of static AABB
     * @param static_max Maximum corner of static AABB
     * @param movement Movement vector
     * @return Time of collision (0-1) or -1 if no collision
     */
    float PerformSweptAABBCollision(const glm::vec3& moving_pos, const glm::vec3& moving_size,
                                   const glm::vec3& static_min, const glm::vec3& static_max, const glm::vec3& movement);

private:
    CollisionShapeType m_shape_type;
    glm::vec3 m_size;
    PhysicsMaterial m_material;
    int m_collision_layer;
    int m_collision_mask;

    glm::vec3 m_velocity;
    glm::vec3 m_last_collision_normal;
    bool m_on_floor;
    bool m_on_wall;
    bool m_on_ceiling;

    PhysicsBody3D* m_physics_body;
    CollisionCallback m_collision_callback;
    bool m_needs_recreation;

    /**
     * @brief Recreate the physics body
     */
    void RecreatePhysicsBody();

    /**
     * @brief Handle collision detection
     */
    void OnCollision(Node3D* other_node, bool entered);

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
