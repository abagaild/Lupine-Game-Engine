#include "lupine/components/Area2D.h"
#include "lupine/components/CollisionShape2D.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/physics/PhysicsManager.h"
#include <iostream>
#include <algorithm>

namespace Lupine {

Area2D::Area2D()
    : Component("Area2D")
    , m_shape_type(CollisionShapeType::Box)
    , m_size(1.0f, 1.0f)
    , m_collision_layer(0)
    , m_collision_mask(0xFFFF)
    , m_monitoring(true)
    , m_monitorable(true)
    , m_physics_body(nullptr)
    , m_needs_recreation(false) {
    
    InitializeExportVariables();
}

Area2D::~Area2D() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody2D(m_physics_body);
        m_physics_body = nullptr;
    }
}

void Area2D::SetShapeType(CollisionShapeType type) {
    if (m_shape_type != type) {
        m_shape_type = type;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void Area2D::SetSize(const glm::vec2& size) {
    if (m_size != size) {
        m_size = size;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void Area2D::SetCollisionLayer(int layer) {
    if (m_collision_layer != layer) {
        m_collision_layer = layer;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void Area2D::SetCollisionMask(int mask) {
    if (m_collision_mask != mask) {
        m_collision_mask = mask;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void Area2D::SetMonitoring(bool monitoring) {
    if (m_monitoring != monitoring) {
        m_monitoring = monitoring;
        UpdateExportVariables();
    }
}

void Area2D::SetMonitorable(bool monitorable) {
    if (m_monitorable != monitorable) {
        m_monitorable = monitorable;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void Area2D::SetOnBodyEntered(AreaCallback callback) {
    m_on_body_entered = callback;
}

void Area2D::SetOnBodyExited(AreaCallback callback) {
    m_on_body_exited = callback;
}

void Area2D::SetOnAreaEntered(AreaCallback callback) {
    m_on_area_entered = callback;
}

void Area2D::SetOnAreaExited(AreaCallback callback) {
    m_on_area_exited = callback;
}

std::vector<Node2D*> Area2D::GetOverlappingBodies() const {
    return m_overlapping_bodies;
}

std::vector<Node2D*> Area2D::GetOverlappingAreas() const {
    return m_overlapping_areas;
}

bool Area2D::HasOverlappingBody(Node2D* node) const {
    return std::find(m_overlapping_bodies.begin(), m_overlapping_bodies.end(), node) != m_overlapping_bodies.end();
}

bool Area2D::HasOverlappingArea(Node2D* node) const {
    return std::find(m_overlapping_areas.begin(), m_overlapping_areas.end(), node) != m_overlapping_areas.end();
}

void Area2D::OnReady() {
    UpdateFromExportVariables();
    RecreatePhysicsBody();
}

void Area2D::OnUpdate(float delta_time) {
    (void)delta_time;
    
    // Check if we need to recreate the physics body
    if (m_needs_recreation) {
        RecreatePhysicsBody();
        m_needs_recreation = false;
    }
    
    // Update from export variables if they changed
    UpdateFromExportVariables();
}

void Area2D::OnDestroy() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody2D(m_physics_body);
        m_physics_body = nullptr;
    }
}

void Area2D::InitializeExportVariables() {
    // Shape properties
    std::vector<std::string> shapeTypeOptions = {
        "Box", "Circle", "Capsule"
    };
    AddEnumExportVariable("shape_type", ShapeTypeToInt(m_shape_type), "Collision shape type", shapeTypeOptions);
    AddExportVariable("size", m_size, "Shape size (width/height for box, radius for circle)", ExportVariableType::Vec2);

    // Collision properties
    AddExportVariable("collision_layer", m_collision_layer, "Collision layer (0-31)", ExportVariableType::Int);
    AddExportVariable("collision_mask", m_collision_mask, "Collision mask (which layers to detect)", ExportVariableType::Int);

    // Area properties
    AddExportVariable("monitoring", m_monitoring, "Whether area is actively monitoring for overlaps", ExportVariableType::Bool);
    AddExportVariable("monitorable", m_monitorable, "Whether area can be detected by other areas", ExportVariableType::Bool);
}

void Area2D::UpdateExportVariables() {
    SetExportVariable("shape_type", ShapeTypeToInt(m_shape_type));
    SetExportVariable("size", m_size);
    SetExportVariable("collision_layer", m_collision_layer);
    SetExportVariable("collision_mask", m_collision_mask);
    SetExportVariable("monitoring", m_monitoring);
    SetExportVariable("monitorable", m_monitorable);
}

void Area2D::UpdateFromExportVariables() {
    // Get values from export variables
    CollisionShapeType new_shape_type = IntToShapeType(GetExportVariableValue<int>("shape_type", ShapeTypeToInt(m_shape_type)));
    glm::vec2 new_size = GetExportVariableValue<glm::vec2>("size", m_size);
    int new_collision_layer = GetExportVariableValue<int>("collision_layer", m_collision_layer);
    int new_collision_mask = GetExportVariableValue<int>("collision_mask", m_collision_mask);
    bool new_monitoring = GetExportVariableValue<bool>("monitoring", m_monitoring);
    bool new_monitorable = GetExportVariableValue<bool>("monitorable", m_monitorable);

    // Check if any values changed that require recreation
    bool needs_recreation = false;
    if (m_shape_type != new_shape_type) {
        m_shape_type = new_shape_type;
        needs_recreation = true;
    }
    if (m_size != new_size) {
        m_size = new_size;
        needs_recreation = true;
    }
    if (m_collision_layer != new_collision_layer) {
        m_collision_layer = new_collision_layer;
        needs_recreation = true;
    }
    if (m_collision_mask != new_collision_mask) {
        m_collision_mask = new_collision_mask;
        needs_recreation = true;
    }
    if (m_monitorable != new_monitorable) {
        m_monitorable = new_monitorable;
        needs_recreation = true;
    }

    // Update simple properties
    m_monitoring = new_monitoring;

    if (needs_recreation) {
        m_needs_recreation = true;
    }
}

void Area2D::RecreatePhysicsBody() {
    // Remove existing physics body
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody2D(m_physics_body);
        m_physics_body = nullptr;
    }

    // Get the owner node
    Node2D* node2d = dynamic_cast<Node2D*>(GetOwner());
    if (!node2d) {
        std::cerr << "Area2D can only be attached to Node2D!" << std::endl;
        return;
    }

    // Create physics body as a sensor (trigger)
    PhysicsMaterial material;
    material.density = 0.0f; // Areas have no mass

    // Check if there's a CollisionShape2D component and use its data
    CollisionShapeType shape_type = m_shape_type;
    glm::vec2 shape_size = m_size;
    glm::vec2 shape_offset(0.0f, 0.0f);

    if (auto* collision_shape = GetOwner()->GetComponent<CollisionShape2D>()) {
        shape_type = collision_shape->GetShapeType();
        shape_size = collision_shape->GetSize();
        shape_offset = collision_shape->GetOffset();
        std::cout << "Area2D using CollisionShape2D: type=" << (int)shape_type
                  << " size=(" << shape_size.x << "," << shape_size.y << ")"
                  << " offset=(" << shape_offset.x << "," << shape_offset.y << ")" << std::endl;
    }

    m_physics_body = PhysicsManager::CreatePhysicsBody2D(node2d, PhysicsBodyType::Static, shape_type, shape_size, material, shape_offset);

    if (m_physics_body) {
        // Set as sensor for area detection
        m_physics_body->SetIsSensor(true);

        // Set collision layer and mask
        m_physics_body->SetCollisionLayer(m_collision_layer);
        m_physics_body->SetCollisionMask(m_collision_mask);

        // Set up collision callback for area detection
        m_physics_body->SetCollisionCallback([this](Node* self, Node* other, const glm::vec3& contact_point, const glm::vec3& normal) {
            (void)self;
            (void)contact_point;

            if (auto* other_node2d = dynamic_cast<Node2D*>(other)) {
                // Check if this is an exit event (indicated by negative normal.x)
                bool is_enter = (normal.x >= 0.0f);
                OnCollision(other_node2d, is_enter);
            }
        });
    }
}

void Area2D::OnCollision(Node2D* other_node, bool entered) {
    if (!m_monitoring || !other_node) {
        return;
    }

    // TODO: Determine if other_node has a physics body or area
    // For now, assume it's a physics body

    if (entered) {
        // Add to overlapping bodies if not already present
        if (std::find(m_overlapping_bodies.begin(), m_overlapping_bodies.end(), other_node) == m_overlapping_bodies.end()) {
            m_overlapping_bodies.push_back(other_node);
            if (m_on_body_entered) {
                m_on_body_entered(other_node);
            }
        }
    } else {
        // Remove from overlapping bodies
        auto it = std::find(m_overlapping_bodies.begin(), m_overlapping_bodies.end(), other_node);
        if (it != m_overlapping_bodies.end()) {
            m_overlapping_bodies.erase(it);
            if (m_on_body_exited) {
                m_on_body_exited(other_node);
            }
        }
    }
}

int Area2D::ShapeTypeToInt(CollisionShapeType type) const {
    switch (type) {
        case CollisionShapeType::Box: return 0;
        case CollisionShapeType::Circle: return 1;
        case CollisionShapeType::Capsule: return 2;
        case CollisionShapeType::Mesh: return 3;
        case CollisionShapeType::Heightfield: return 4;
        default: return 0;
    }
}

CollisionShapeType Area2D::IntToShapeType(int value) const {
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
REGISTER_COMPONENT(Area2D, "Physics", "2D area for trigger detection")
