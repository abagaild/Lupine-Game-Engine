#include "lupine/nodes/Node2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace Lupine {

Node2D::Node2D(const std::string& name)
    : Node(name)
    , m_position(0.0f, 0.0f)
    , m_rotation(0.0f)
    , m_scale(1.0f, 1.0f) {
}

glm::vec2 Node2D::GetGlobalPosition() const {
    if (!m_parent) {
        return m_position;
    }
    
    // Check if parent is also a Node2D
    if (auto* parent2d = dynamic_cast<const Node2D*>(m_parent)) {
        glm::mat3 parent_transform = parent2d->GetGlobalTransform();
        glm::vec3 global_pos = parent_transform * glm::vec3(m_position, 1.0f);
        return glm::vec2(global_pos.x, global_pos.y);
    }
    
    // If parent is not Node2D, just return local position
    return m_position;
}

float Node2D::GetGlobalRotation() const {
    if (!m_parent) {
        return m_rotation;
    }
    
    // Check if parent is also a Node2D
    if (auto* parent2d = dynamic_cast<const Node2D*>(m_parent)) {
        return parent2d->GetGlobalRotation() + m_rotation;
    }
    
    // If parent is not Node2D, just return local rotation
    return m_rotation;
}

glm::vec2 Node2D::GetGlobalScale() const {
    if (!m_parent) {
        return m_scale;
    }
    
    // Check if parent is also a Node2D
    if (auto* parent2d = dynamic_cast<const Node2D*>(m_parent)) {
        glm::vec2 parent_scale = parent2d->GetGlobalScale();
        return glm::vec2(parent_scale.x * m_scale.x, parent_scale.y * m_scale.y);
    }
    
    // If parent is not Node2D, just return local scale
    return m_scale;
}

glm::mat3 Node2D::GetLocalTransform() const {
    // Create transformation matrix: T * R * S
    // Apply transformations in correct order: Translation, then Rotation, then Scale
    glm::mat3 transform(1.0f);

    // Translation
    transform[2][0] = m_position.x;
    transform[2][1] = m_position.y;

    // Rotation
    float cos_r = std::cos(m_rotation);
    float sin_r = std::sin(m_rotation);

    glm::mat3 rotation(1.0f);
    rotation[0][0] = cos_r;
    rotation[0][1] = sin_r;
    rotation[1][0] = -sin_r;
    rotation[1][1] = cos_r;

    transform = transform * rotation;

    // Scale
    glm::mat3 scale(1.0f);
    scale[0][0] = m_scale.x;
    scale[1][1] = m_scale.y;

    transform = transform * scale;

    return transform;
}

glm::mat3 Node2D::GetGlobalTransform() const {
    if (!m_parent || !m_parent->IsValidNode()) {
        return GetLocalTransform();
    }

    // Check if parent is also a Node2D using safe casting
    if (auto* parent2d = m_parent->SafeCast<const Node2D>()) {
        try {
            return parent2d->GetGlobalTransform() * GetLocalTransform();
        } catch (...) {
            // If parent transform calculation fails, return local transform
            return GetLocalTransform();
        }
    }

    // If parent is not Node2D, just return local transform
    return GetLocalTransform();
}

void Node2D::CopyTypeSpecificProperties(Node* target) const {
    // Call base class implementation first
    Node::CopyTypeSpecificProperties(target);

    // Copy Node2D-specific properties using safe casting
    if (target && target->IsValidNode()) {
        if (auto* target2d = target->SafeCast<Node2D>()) {
        target2d->SetPosition(m_position);
        target2d->SetRotation(m_rotation);
        target2d->SetScale(m_scale);
    }
 }
}

} // namespace Lupine
