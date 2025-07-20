#pragma once

#include "lupine/core/Component.h"
#include "lupine/physics/PhysicsManager.h"
#include <glm/glm.hpp>
#include <vector>

namespace Lupine {

/**
 * @brief 2D Collision Shape Component
 * 
 * Defines collision shapes for 2D physics bodies.
 * Can be used with RigidBody2D, KinematicBody2D, or Area2D components.
 */
class CollisionShape2D : public Component {
public:
    /**
     * @brief Constructor
     */
    CollisionShape2D();
    
    /**
     * @brief Destructor
     */
    virtual ~CollisionShape2D() = default;
    
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
    
    /**
     * @brief Set shape offset from node center
     * @param offset Local offset
     */
    void SetOffset(const glm::vec2& offset);
    
    /**
     * @brief Get shape offset
     * @return Current offset
     */
    const glm::vec2& GetOffset() const { return m_offset; }
    
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
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "CollisionShape2D"; }

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
    CollisionShapeType m_shape_type;
    glm::vec2 m_size;
    glm::vec2 m_offset;
    bool m_is_trigger;
    int m_collision_layer;
    int m_collision_mask;
    
    bool m_needs_update;
    
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
