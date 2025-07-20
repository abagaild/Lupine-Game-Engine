#pragma once

#include "lupine/core/Component.h"
#include "lupine/physics/PhysicsManager.h"
#include <glm/glm.hpp>
#include <vector>

namespace Lupine {

/**
 * @brief 2D Collision Polygon Component
 * 
 * Defines custom polygon collision shapes for 2D physics bodies.
 * Can be used with RigidBody2D, KinematicBody2D, or Area2D components.
 */
class CollisionPolygon2D : public Component {
public:
    /**
     * @brief Constructor
     */
    CollisionPolygon2D();
    
    /**
     * @brief Destructor
     */
    virtual ~CollisionPolygon2D() = default;
    
    // === Polygon Properties ===
    
    /**
     * @brief Set polygon vertices
     * @param vertices Array of vertices in local coordinates
     */
    void SetVertices(const std::vector<glm::vec2>& vertices);
    
    /**
     * @brief Get polygon vertices
     * @return Current vertices
     */
    const std::vector<glm::vec2>& GetVertices() const { return m_vertices; }
    
    /**
     * @brief Add a vertex to the polygon
     * @param vertex Vertex position in local coordinates
     */
    void AddVertex(const glm::vec2& vertex);
    
    /**
     * @brief Remove vertex at index
     * @param index Vertex index to remove
     */
    void RemoveVertex(size_t index);
    
    /**
     * @brief Clear all vertices
     */
    void ClearVertices();
    
    /**
     * @brief Get vertex count
     * @return Number of vertices
     */
    size_t GetVertexCount() const { return m_vertices.size(); }
    
    /**
     * @brief Set whether polygon is convex (for optimization)
     * @param convex True if polygon is convex
     */
    void SetConvex(bool convex);
    
    /**
     * @brief Get whether polygon is convex
     * @return True if convex
     */
    bool IsConvex() const { return m_is_convex; }
    
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
     * @brief Validate polygon (check for valid shape)
     * @return True if polygon is valid for physics
     */
    bool IsValidPolygon() const;
    
    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "CollisionPolygon2D"; }

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
    std::vector<glm::vec2> m_vertices;
    bool m_is_convex;
    bool m_is_trigger;
    int m_collision_layer;
    int m_collision_mask;
    
    bool m_needs_update;
    
    /**
     * @brief Convert vertices to string for export
     */
    std::string VerticesToString() const;
    
    /**
     * @brief Parse vertices from string for import
     */
    void StringToVertices(const std::string& str);
    
    /**
     * @brief Check if polygon is actually convex
     */
    bool CheckConvexity() const;
};

} // namespace Lupine
