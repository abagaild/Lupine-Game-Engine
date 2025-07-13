#pragma once

#include "lupine/core/Node.h"
#include <glm/glm.hpp>

namespace Lupine {

/**
 * @brief 2D node with position, rotation, and scale
 * 
 * Node2D represents objects in 2D space with transform properties.
 * It uses 2D coordinates (x, y) and rotation in radians.
 */
class Node2D : public Node {
public:
    /**
     * @brief Constructor
     * @param name Name of the node
     */
    explicit Node2D(const std::string& name = "Node2D");
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Node2D() = default;
    
    /**
     * @brief Get local position
     * @return 2D position vector
     */
    const glm::vec2& GetPosition() const { return m_position; }
    
    /**
     * @brief Set local position
     * @param position New position
     */
    void SetPosition(const glm::vec2& position) { m_position = position; }
    
    /**
     * @brief Set local position
     * @param x X coordinate
     * @param y Y coordinate
     */
    void SetPosition(float x, float y) { m_position = glm::vec2(x, y); }
    
    /**
     * @brief Get local rotation in radians
     * @return Rotation in radians
     */
    float GetRotation() const { return m_rotation; }
    
    /**
     * @brief Set local rotation in radians
     * @param rotation Rotation in radians
     */
    void SetRotation(float rotation) { m_rotation = rotation; }
    
    /**
     * @brief Get local scale
     * @return 2D scale vector
     */
    const glm::vec2& GetScale() const { return m_scale; }
    
    /**
     * @brief Set local scale
     * @param scale New scale
     */
    void SetScale(const glm::vec2& scale) { m_scale = scale; }
    
    /**
     * @brief Set local scale
     * @param x X scale
     * @param y Y scale
     */
    void SetScale(float x, float y) { m_scale = glm::vec2(x, y); }
    
    /**
     * @brief Set uniform scale
     * @param scale Uniform scale value
     */
    void SetScale(float scale) { m_scale = glm::vec2(scale, scale); }
    
    /**
     * @brief Get global position (including parent transforms)
     * @return Global 2D position
     */
    glm::vec2 GetGlobalPosition() const;
    
    /**
     * @brief Get global rotation (including parent transforms)
     * @return Global rotation in radians
     */
    float GetGlobalRotation() const;
    
    /**
     * @brief Get global scale (including parent transforms)
     * @return Global 2D scale
     */
    glm::vec2 GetGlobalScale() const;
    
    /**
     * @brief Get local transform matrix
     * @return 3x3 transform matrix
     */
    glm::mat3 GetLocalTransform() const;
    
    /**
     * @brief Get global transform matrix
     * @return 3x3 transform matrix
     */
    glm::mat3 GetGlobalTransform() const;
    
    /**
     * @brief Translate by offset
     * @param offset Translation offset
     */
    void Translate(const glm::vec2& offset) { m_position += offset; }
    
    /**
     * @brief Rotate by angle
     * @param angle Rotation angle in radians
     */
    void Rotate(float angle) { m_rotation += angle; }
    
    /**
     * @brief Scale by factor
     * @param factor Scale factor
     */
    void ScaleBy(const glm::vec2& factor) { m_scale *= factor; }
    
    /**
     * @brief Scale by uniform factor
     * @param factor Uniform scale factor
     */
    void ScaleBy(float factor) { m_scale *= factor; }
    
    /**
     * @brief Get node type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Node2D"; }

protected:
    /**
     * @brief Copy type-specific properties to another node (override for Node2D)
     * @param target Target node to copy properties to
     */
    void CopyTypeSpecificProperties(Node* target) const override;

protected:
    glm::vec2 m_position;
    float m_rotation;
    glm::vec2 m_scale;
};

} // namespace Lupine
