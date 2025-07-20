#include "lupine/nodes/Node3D.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Lupine {

Node3D::Node3D(const std::string& name)
    : Node(name)
    , m_position(0.0f, 0.0f, 0.0f)
    , m_rotation(1.0f, 0.0f, 0.0f, 0.0f) // Identity quaternion
    , m_scale(1.0f, 1.0f, 1.0f) {
}

glm::vec3 Node3D::GetGlobalPosition() const {
    if (!m_parent || !m_parent->IsValidNode()) {
        return m_position;
    }

    // Check if parent is also a Node3D using safe casting
    if (auto* parent3d = m_parent->SafeCast<const Node3D>()) {
        try {
            glm::mat4 parent_transform = parent3d->GetGlobalTransform();
            glm::vec4 global_pos = parent_transform * glm::vec4(m_position, 1.0f);
            return glm::vec3(global_pos);
        } catch (...) {
            // If parent transform calculation fails, return local position
            return m_position;
        }
    }

    // If parent is not Node3D, just return local position
    return m_position;
}

glm::quat Node3D::GetGlobalRotation() const {
    if (!m_parent) {
        return m_rotation;
    }
    
    // Check if parent is also a Node3D
    if (auto* parent3d = dynamic_cast<const Node3D*>(m_parent)) {
        return parent3d->GetGlobalRotation() * m_rotation;
    }
    
    // If parent is not Node3D, just return local rotation
    return m_rotation;
}

glm::vec3 Node3D::GetGlobalScale() const {
    if (!m_parent) {
        return m_scale;
    }
    
    // Check if parent is also a Node3D
    if (auto* parent3d = dynamic_cast<const Node3D*>(m_parent)) {
        glm::vec3 parent_scale = parent3d->GetGlobalScale();
        return glm::vec3(parent_scale.x * m_scale.x, parent_scale.y * m_scale.y, parent_scale.z * m_scale.z);
    }
    
    // If parent is not Node3D, just return local scale
    return m_scale;
}

glm::mat4 Node3D::GetLocalTransform() const {
    // Create transformation matrix: T * R * S
    // Apply transformations in reverse order: Scale, then Rotation, then Translation
    glm::mat4 transform = glm::mat4(1.0f);

    // Translation
    transform = glm::translate(transform, m_position);

    // Rotation
    transform = transform * glm::mat4_cast(m_rotation);

    // Scale
    transform = glm::scale(transform, m_scale);

    return transform;
}

glm::mat4 Node3D::GetGlobalTransform() const {
    if (!m_parent || !m_parent->IsValidNode()) {
        return GetLocalTransform();
    }

    // Check if parent is also a Node3D using safe casting
    if (auto* parent3d = m_parent->SafeCast<const Node3D>()) {
        try {
            return parent3d->GetGlobalTransform() * GetLocalTransform();
        } catch (...) {
            // If parent transform calculation fails, return local transform
            return GetLocalTransform();
        }
    }

    // If parent is not Node3D, just return local transform
    return GetLocalTransform();
}

glm::vec3 Node3D::GetForward() const {
    return m_rotation * glm::vec3(0.0f, 0.0f, -1.0f);
}

glm::vec3 Node3D::GetRight() const {
    return m_rotation * glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3 Node3D::GetUp() const {
    return m_rotation * glm::vec3(0.0f, 1.0f, 0.0f);
}

void Node3D::CopyTypeSpecificProperties(Node* target) const {
    // Call base class implementation first
    Node::CopyTypeSpecificProperties(target);

    // Copy Node3D-specific properties
    if (auto* target3d = dynamic_cast<Node3D*>(target)) {
        target3d->SetPosition(m_position);
        target3d->SetRotation(m_rotation);
        target3d->SetScale(m_scale);
    }
}

} // namespace Lupine
