#pragma once

#include "lupine/core/Node.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Lupine {

/**
 * @brief 3D node with position, rotation, and scale
 * 
 * Node3D represents objects in 3D space with transform properties.
 * It uses 3D coordinates (x, y, z) and quaternion rotation.
 */
class Node3D : public Node {
public:
    /**
     * @brief Constructor
     * @param name Name of the node
     */
    explicit Node3D(const std::string& name = "Node3D");
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Node3D() = default;
    
    /**
     * @brief Get local position
     * @return 3D position vector
     */
    const glm::vec3& GetPosition() const { return m_position; }
    
    /**
     * @brief Set local position
     * @param position New position
     */
    void SetPosition(const glm::vec3& position) { m_position = position; }
    
    /**
     * @brief Set local position
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     */
    void SetPosition(float x, float y, float z) { m_position = glm::vec3(x, y, z); }
    
    /**
     * @brief Get local rotation quaternion
     * @return Rotation quaternion
     */
    const glm::quat& GetRotation() const { return m_rotation; }
    
    /**
     * @brief Set local rotation quaternion
     * @param rotation Rotation quaternion
     */
    void SetRotation(const glm::quat& rotation) { m_rotation = rotation; }
    
    /**
     * @brief Set local rotation from Euler angles (in radians)
     * @param x X rotation (pitch)
     * @param y Y rotation (yaw)
     * @param z Z rotation (roll)
     */
    void SetRotation(float x, float y, float z) { m_rotation = glm::quat(glm::vec3(x, y, z)); }
    
    /**
     * @brief Get local scale
     * @return 3D scale vector
     */
    const glm::vec3& GetScale() const { return m_scale; }
    
    /**
     * @brief Set local scale
     * @param scale New scale
     */
    void SetScale(const glm::vec3& scale) { m_scale = scale; }
    
    /**
     * @brief Set local scale
     * @param x X scale
     * @param y Y scale
     * @param z Z scale
     */
    void SetScale(float x, float y, float z) { m_scale = glm::vec3(x, y, z); }
    
    /**
     * @brief Set uniform scale
     * @param scale Uniform scale value
     */
    void SetScale(float scale) { m_scale = glm::vec3(scale, scale, scale); }
    
    /**
     * @brief Get global position (including parent transforms)
     * @return Global 3D position
     */
    glm::vec3 GetGlobalPosition() const;
    
    /**
     * @brief Get global rotation (including parent transforms)
     * @return Global rotation quaternion
     */
    glm::quat GetGlobalRotation() const;
    
    /**
     * @brief Get global scale (including parent transforms)
     * @return Global 3D scale
     */
    glm::vec3 GetGlobalScale() const;
    
    /**
     * @brief Get local transform matrix
     * @return 4x4 transform matrix
     */
    glm::mat4 GetLocalTransform() const;
    
    /**
     * @brief Get global transform matrix
     * @return 4x4 transform matrix
     */
    glm::mat4 GetGlobalTransform() const;
    
    /**
     * @brief Translate by offset
     * @param offset Translation offset
     */
    void Translate(const glm::vec3& offset) { m_position += offset; }
    
    /**
     * @brief Rotate by quaternion
     * @param rotation Rotation quaternion
     */
    void Rotate(const glm::quat& rotation) { m_rotation = rotation * m_rotation; }
    
    /**
     * @brief Rotate around axis
     * @param angle Rotation angle in radians
     * @param axis Rotation axis
     */
    void Rotate(float angle, const glm::vec3& axis) { 
        m_rotation = glm::angleAxis(angle, axis) * m_rotation; 
    }
    
    /**
     * @brief Scale by factor
     * @param factor Scale factor
     */
    void ScaleBy(const glm::vec3& factor) { m_scale *= factor; }
    
    /**
     * @brief Scale by uniform factor
     * @param factor Uniform scale factor
     */
    void ScaleBy(float factor) { m_scale *= factor; }
    
    /**
     * @brief Get forward vector (local -Z axis)
     * @return Forward vector
     */
    glm::vec3 GetForward() const;
    
    /**
     * @brief Get right vector (local X axis)
     * @return Right vector
     */
    glm::vec3 GetRight() const;
    
    /**
     * @brief Get up vector (local Y axis)
     * @return Up vector
     */
    glm::vec3 GetUp() const;
    
    /**
     * @brief Get node type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Node3D"; }

protected:
    /**
     * @brief Copy type-specific properties to another node (override for Node3D)
     * @param target Target node to copy properties to
     */
    void CopyTypeSpecificProperties(Node* target) const override;

    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_scale;
};

} // namespace Lupine
