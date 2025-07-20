#pragma once

#include "lupine/core/Component.h"
#include "lupine/physics/PhysicsManager.h"
#include <glm/glm.hpp>
#include <functional>

namespace Lupine {

class Node2D;

/**
 * @brief 2D Area Component
 * 
 * Defines trigger areas for 2D physics detection.
 * Areas can detect when other physics bodies enter, stay, or exit their bounds.
 */
class Area2D : public Component {
public:
    /**
     * @brief Area event callbacks
     */
    using AreaCallback = std::function<void(Node2D* other_node)>;
    
    /**
     * @brief Constructor
     */
    Area2D();
    
    /**
     * @brief Destructor
     */
    virtual ~Area2D();
    
    // === Area Properties ===
    
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
     * @brief Set collision mask (which layers this area detects)
     * @param mask Collision mask bitfield
     */
    void SetCollisionMask(int mask);
    
    /**
     * @brief Get collision mask
     * @return Current collision mask
     */
    int GetCollisionMask() const { return m_collision_mask; }
    
    /**
     * @brief Set whether area is monitoring (actively detecting)
     * @param monitoring True to enable monitoring
     */
    void SetMonitoring(bool monitoring);
    
    /**
     * @brief Get whether area is monitoring
     * @return True if monitoring
     */
    bool IsMonitoring() const { return m_monitoring; }
    
    /**
     * @brief Set whether area is monitorable (can be detected by other areas)
     * @param monitorable True to be monitorable
     */
    void SetMonitorable(bool monitorable);
    
    /**
     * @brief Get whether area is monitorable
     * @return True if monitorable
     */
    bool IsMonitorable() const { return m_monitorable; }
    
    // === Callbacks ===
    
    /**
     * @brief Set callback for when a body enters the area
     * @param callback Function to call on body enter
     */
    void SetOnBodyEntered(AreaCallback callback);
    
    /**
     * @brief Set callback for when a body exits the area
     * @param callback Function to call on body exit
     */
    void SetOnBodyExited(AreaCallback callback);
    
    /**
     * @brief Set callback for when an area enters this area
     * @param callback Function to call on area enter
     */
    void SetOnAreaEntered(AreaCallback callback);
    
    /**
     * @brief Set callback for when an area exits this area
     * @param callback Function to call on area exit
     */
    void SetOnAreaExited(AreaCallback callback);
    
    // === Queries ===
    
    /**
     * @brief Get all bodies currently in the area
     * @return Vector of nodes with physics bodies in the area
     */
    std::vector<Node2D*> GetOverlappingBodies() const;
    
    /**
     * @brief Get all areas currently overlapping this area
     * @return Vector of nodes with areas overlapping this area
     */
    std::vector<Node2D*> GetOverlappingAreas() const;
    
    /**
     * @brief Check if a specific body is in the area
     * @param node Node to check
     * @return True if node is in the area
     */
    bool HasOverlappingBody(Node2D* node) const;
    
    /**
     * @brief Check if a specific area is overlapping this area
     * @param node Node to check
     * @return True if area is overlapping
     */
    bool HasOverlappingArea(Node2D* node) const;
    
    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Area2D"; }

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
    int m_collision_layer;
    int m_collision_mask;
    bool m_monitoring;
    bool m_monitorable;
    
    // Callbacks
    AreaCallback m_on_body_entered;
    AreaCallback m_on_body_exited;
    AreaCallback m_on_area_entered;
    AreaCallback m_on_area_exited;
    
    // Physics body for area detection
    PhysicsBody2D* m_physics_body;
    bool m_needs_recreation;
    
    // Tracking overlapping objects
    std::vector<Node2D*> m_overlapping_bodies;
    std::vector<Node2D*> m_overlapping_areas;
    
    /**
     * @brief Recreate the physics body
     */
    void RecreatePhysicsBody();
    
    /**
     * @brief Handle collision events
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
