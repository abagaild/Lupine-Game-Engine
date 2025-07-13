#pragma once

#include "lupine/core/Component.h"
#include "lupine/physics/PhysicsManager.h"
#include <glm/glm.hpp>

namespace Lupine {

class PhysicsBody2D;

/**
 * @brief 2D Rigid Body Component
 * 
 * Adds 2D physics simulation to a Node2D using Box2D.
 * Handles collision detection, forces, and physics material properties.
 */
class RigidBody2D : public Component {
public:
    /**
     * @brief Constructor
     */
    RigidBody2D();
    
    /**
     * @brief Destructor
     */
    virtual ~RigidBody2D();
    
    // === Physics Body Properties ===
    
    /**
     * @brief Set physics body type
     * @param type Body type (Static, Kinematic, Dynamic)
     */
    void SetBodyType(PhysicsBodyType type);
    
    /**
     * @brief Get physics body type
     * @return Current body type
     */
    PhysicsBodyType GetBodyType() const { return m_body_type; }
    
    /**
     * @brief Set collision shape
     * @param shape_type Type of collision shape
     * @param size Shape size (width/height for box, radius for circle)
     */
    void SetCollisionShape(CollisionShapeType shape_type, const glm::vec2& size);
    
    /**
     * @brief Get collision shape type
     * @return Current collision shape type
     */
    CollisionShapeType GetCollisionShapeType() const { return m_shape_type; }
    
    /**
     * @brief Get collision shape size
     * @return Current collision shape size
     */
    glm::vec2 GetCollisionShapeSize() const { return m_shape_size; }
    
    // === Physics Material ===
    
    /**
     * @brief Set physics material
     * @param material Material properties
     */
    void SetMaterial(const PhysicsMaterial& material);
    
    /**
     * @brief Get physics material
     * @return Current material properties
     */
    const PhysicsMaterial& GetMaterial() const { return m_material; }
    
    /**
     * @brief Set mass
     * @param mass Object mass
     */
    void SetMass(float mass);
    
    /**
     * @brief Get mass
     * @return Current mass
     */
    float GetMass() const;
    
    /**
     * @brief Set gravity scale
     * @param scale Gravity scale multiplier (1.0 = normal gravity, 0.0 = no gravity)
     */
    void SetGravityScale(float scale);
    
    /**
     * @brief Get gravity scale
     * @return Current gravity scale
     */
    float GetGravityScale() const;
    
    // === Velocity and Forces ===
    
    /**
     * @brief Set linear velocity
     * @param velocity Linear velocity vector
     */
    void SetLinearVelocity(const glm::vec2& velocity);
    
    /**
     * @brief Get linear velocity
     * @return Current linear velocity
     */
    glm::vec2 GetLinearVelocity() const;
    
    /**
     * @brief Set angular velocity
     * @param velocity Angular velocity (radians per second)
     */
    void SetAngularVelocity(float velocity);
    
    /**
     * @brief Get angular velocity
     * @return Current angular velocity
     */
    float GetAngularVelocity() const;
    
    /**
     * @brief Apply force at center of mass
     * @param force Force vector
     */
    void ApplyForce(const glm::vec2& force);
    
    /**
     * @brief Apply force at specific point
     * @param force Force vector
     * @param point Point of application (world coordinates)
     */
    void ApplyForceAtPoint(const glm::vec2& force, const glm::vec2& point);
    
    /**
     * @brief Apply impulse at center of mass
     * @param impulse Impulse vector
     */
    void ApplyImpulse(const glm::vec2& impulse);
    
    /**
     * @brief Apply impulse at specific point
     * @param impulse Impulse vector
     * @param point Point of application (world coordinates)
     */
    void ApplyImpulseAtPoint(const glm::vec2& impulse, const glm::vec2& point);
    
    /**
     * @brief Apply torque
     * @param torque Torque value
     */
    void ApplyTorque(float torque);
    
    // === Collision Detection ===
    
    /**
     * @brief Set collision callback
     * @param callback Function to call when collision occurs
     */
    void SetCollisionCallback(CollisionCallback callback);
    
    /**
     * @brief Enable/disable collision detection
     * @param enabled True to enable collision detection
     */
    void SetCollisionEnabled(bool enabled);
    
    /**
     * @brief Check if collision detection is enabled
     * @return True if collision detection is enabled
     */
    bool IsCollisionEnabled() const { return m_collision_enabled; }
    
    /**
     * @brief Set collision layer
     * @param layer Collision layer (0-15)
     */
    void SetCollisionLayer(int layer);
    
    /**
     * @brief Get collision layer
     * @return Current collision layer
     */
    int GetCollisionLayer() const { return m_collision_layer; }
    
    /**
     * @brief Set collision mask (which layers this body can collide with)
     * @param mask Collision mask bitfield
     */
    void SetCollisionMask(int mask);
    
    /**
     * @brief Get collision mask
     * @return Current collision mask
     */
    int GetCollisionMask() const { return m_collision_mask; }
    
    // === Component Lifecycle ===
    
    virtual void OnReady() override;
    virtual void OnUpdate(float delta_time) override;
    virtual void OnDestroy() override;
    
    /**
     * @brief Get the underlying physics body
     * @return Pointer to physics body (can be null)
     */
    PhysicsBody2D* GetPhysicsBody() const { return m_physics_body; }

    // === Component Interface ===

    /**
     * @brief Get component type name
     * @return Component type name
     */
    std::string GetTypeName() const override { return "RigidBody2D"; }

    /**
     * @brief Get component category
     * @return Component category
     */
    std::string GetCategory() const override { return "Physics"; }

    // === Debug and Utility Methods ===

    /**
     * @brief Get current velocity
     * @return Current linear velocity
     */
    glm::vec2 GetVelocity() const;

    /**
     * @brief Check if the body is sleeping
     * @return True if the body is sleeping
     */
    bool IsSleeping() const;

    /**
     * @brief Wake up the physics body
     */
    void WakeUp();

    /**
     * @brief Put the physics body to sleep
     */
    void Sleep();

protected:
    virtual void InitializeExportVariables() override;
    void UpdateExportVariables();
    void UpdateFromExportVariables();
    void RecreatePhysicsBody();

private:
    PhysicsBodyType m_body_type;
    CollisionShapeType m_shape_type;
    glm::vec2 m_shape_size;
    PhysicsMaterial m_material;
    float m_gravity_scale;

    bool m_collision_enabled;
    int m_collision_layer;
    int m_collision_mask;

    PhysicsBody2D* m_physics_body;
    CollisionCallback m_collision_callback;

    bool m_needs_recreation;
};

} // namespace Lupine
