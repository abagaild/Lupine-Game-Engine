#include "lupine/components/CollisionShape2D.h"
#include "lupine/nodes/Node2D.h"
#include <iostream>

namespace Lupine {

CollisionShape2D::CollisionShape2D()
    : Component("CollisionShape2D")
    , m_shape_type(CollisionShapeType::Box)
    , m_size(1.0f, 1.0f)
    , m_offset(0.0f, 0.0f)
    , m_is_trigger(false)
    , m_collision_layer(0)
    , m_collision_mask(0xFFFF)
    , m_needs_update(false) {
    
    InitializeExportVariables();
}

void CollisionShape2D::SetShapeType(CollisionShapeType type) {
    if (m_shape_type != type) {
        m_shape_type = type;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionShape2D::SetSize(const glm::vec2& size) {
    if (m_size != size) {
        m_size = size;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionShape2D::SetOffset(const glm::vec2& offset) {
    if (m_offset != offset) {
        m_offset = offset;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionShape2D::SetTrigger(bool trigger) {
    if (m_is_trigger != trigger) {
        m_is_trigger = trigger;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionShape2D::SetCollisionLayer(int layer) {
    if (m_collision_layer != layer) {
        m_collision_layer = layer;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionShape2D::SetCollisionMask(int mask) {
    if (m_collision_mask != mask) {
        m_collision_mask = mask;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

bool CollisionShape2D::CollidesWithLayer(int layer) const {
    return (m_collision_mask & (1 << layer)) != 0;
}

void CollisionShape2D::OnReady() {
    UpdateFromExportVariables();
}

void CollisionShape2D::OnUpdate(float delta_time) {
    (void)delta_time;
    
    // Update from export variables if they changed
    UpdateFromExportVariables();
    
    // Note: This component defines collision shapes but doesn't create physics bodies itself.
    // Physics bodies (RigidBody2D, KinematicBody2D, Area2D) will use this component's data.
}

void CollisionShape2D::InitializeExportVariables() {
    // Shape properties
    // Create enum options for shape type
    std::vector<std::string> shapeTypeOptions = {
        "Box", "Circle", "Capsule"
    };

    AddEnumExportVariable("shape_type", ShapeTypeToInt(m_shape_type), "Collision shape type", shapeTypeOptions);
    AddExportVariable("size", m_size, "Shape size (width/height for box, radius for circle)", ExportVariableType::Vec2);
    AddExportVariable("offset", m_offset, "Shape offset from node center", ExportVariableType::Vec2);

    // Collision properties
    AddExportVariable("is_trigger", m_is_trigger, "Whether shape is a trigger (no collision response)", ExportVariableType::Bool);
    AddExportVariable("collision_layer", m_collision_layer, "Collision layer (0-31)", ExportVariableType::Int);
    AddExportVariable("collision_mask", m_collision_mask, "Collision mask (which layers to collide with)", ExportVariableType::Int);
}

void CollisionShape2D::UpdateExportVariables() {
    SetExportVariable("shape_type", ShapeTypeToInt(m_shape_type));
    SetExportVariable("size", m_size);
    SetExportVariable("offset", m_offset);
    SetExportVariable("is_trigger", m_is_trigger);
    SetExportVariable("collision_layer", m_collision_layer);
    SetExportVariable("collision_mask", m_collision_mask);
}

void CollisionShape2D::UpdateFromExportVariables() {
    // Get values from export variables
    CollisionShapeType new_shape_type = IntToShapeType(GetExportVariableValue<int>("shape_type", ShapeTypeToInt(m_shape_type)));
    glm::vec2 new_size = GetExportVariableValue<glm::vec2>("size", m_size);
    glm::vec2 new_offset = GetExportVariableValue<glm::vec2>("offset", m_offset);
    bool new_is_trigger = GetExportVariableValue<bool>("is_trigger", m_is_trigger);
    int new_collision_layer = GetExportVariableValue<int>("collision_layer", m_collision_layer);
    int new_collision_mask = GetExportVariableValue<int>("collision_mask", m_collision_mask);

    // Only log when size actually changes
    if (new_size != m_size) {
        std::cout << "CollisionShape2D::UpdateFromExportVariables: size changed from (" << m_size.x << "," << m_size.y << ") to (" << new_size.x << "," << new_size.y << ")" << std::endl;
    }
    
    // Check if any values changed
    bool changed = false;
    if (m_shape_type != new_shape_type) {
        m_shape_type = new_shape_type;
        changed = true;
    }
    if (m_size != new_size) {
        m_size = new_size;
        changed = true;
    }
    if (m_offset != new_offset) {
        m_offset = new_offset;
        changed = true;
    }
    if (m_is_trigger != new_is_trigger) {
        m_is_trigger = new_is_trigger;
        changed = true;
    }
    if (m_collision_layer != new_collision_layer) {
        m_collision_layer = new_collision_layer;
        changed = true;
    }
    if (m_collision_mask != new_collision_mask) {
        m_collision_mask = new_collision_mask;
        changed = true;
    }
    
    if (changed) {
        m_needs_update = true;
    }
}

int CollisionShape2D::ShapeTypeToInt(CollisionShapeType type) const {
    switch (type) {
        case CollisionShapeType::Box: return 0;
        case CollisionShapeType::Circle: return 1;
        case CollisionShapeType::Capsule: return 2;
        case CollisionShapeType::Mesh: return 3;
        case CollisionShapeType::Heightfield: return 4;
        default: return 0;
    }
}

CollisionShapeType CollisionShape2D::IntToShapeType(int value) const {
    switch (value) {
        case 0: return CollisionShapeType::Box;
        case 1: return CollisionShapeType::Circle;
        case 2: return CollisionShapeType::Capsule;
        case 3: return CollisionShapeType::Mesh;
        case 4: return CollisionShapeType::Heightfield;
        default: return CollisionShapeType::Box;
    }
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(CollisionShape2D, "Physics", "2D collision shape for physics bodies")
