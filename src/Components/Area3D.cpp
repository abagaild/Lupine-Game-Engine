#include "lupine/components/Area3D.h"
#include "lupine/components/CollisionMesh3D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/physics/PhysicsManager.h"
#include <iostream>
#include <algorithm>

namespace Lupine {

Area3D::Area3D()
    : Component("Area3D")
    , m_shape_type(CollisionShapeType::Box)
    , m_size(1.0f, 1.0f, 1.0f)
    , m_collision_layer(0)
    , m_collision_mask(0xFFFF)
    , m_monitoring(true)
    , m_monitorable(true)
    , m_physics_body(nullptr)
    , m_needs_recreation(false) {

    InitializeExportVariables();
}

Area3D::~Area3D() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody3D(m_physics_body);
        m_physics_body = nullptr;
    }
}

void Area3D::SetShapeType(CollisionShapeType type) {
    if (m_shape_type != type) {
        m_shape_type = type;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void Area3D::SetSize(const glm::vec3& size) {
    if (m_size != size) {
        m_size = size;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void Area3D::SetCollisionLayer(int layer) {
    if (m_collision_layer != layer) {
        m_collision_layer = layer;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void Area3D::SetCollisionMask(int mask) {
    if (m_collision_mask != mask) {
        m_collision_mask = mask;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void Area3D::SetMonitoring(bool monitoring) {
    if (m_monitoring != monitoring) {
        m_monitoring = monitoring;
        UpdateExportVariables();
    }
}

void Area3D::SetMonitorable(bool monitorable) {
    if (m_monitorable != monitorable) {
        m_monitorable = monitorable;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void Area3D::SetOnBodyEntered(AreaCallback callback) {
    m_on_body_entered = callback;
}

void Area3D::SetOnBodyExited(AreaCallback callback) {
    m_on_body_exited = callback;
}

void Area3D::SetOnAreaEntered(AreaCallback callback) {
    m_on_area_entered = callback;
}

void Area3D::SetOnAreaExited(AreaCallback callback) {
    m_on_area_exited = callback;
}

std::vector<Node3D*> Area3D::GetOverlappingBodies() const {
    return m_overlapping_bodies;
}

std::vector<Node3D*> Area3D::GetOverlappingAreas() const {
    return m_overlapping_areas;
}

bool Area3D::HasOverlappingBody(Node3D* node) const {
    return std::find(m_overlapping_bodies.begin(), m_overlapping_bodies.end(), node) != m_overlapping_bodies.end();
}

bool Area3D::HasOverlappingArea(Node3D* node) const {
    return std::find(m_overlapping_areas.begin(), m_overlapping_areas.end(), node) != m_overlapping_areas.end();
}

void Area3D::OnReady() {
    UpdateFromExportVariables();
    RecreatePhysicsBody();
}

void Area3D::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if we need to recreate the physics body
    if (m_needs_recreation) {
        RecreatePhysicsBody();
        m_needs_recreation = false;
    }

    // Update from export variables if they changed
    UpdateFromExportVariables();
}

void Area3D::OnDestroy() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody3D(m_physics_body);
        m_physics_body = nullptr;
    }
}

void Area3D::InitializeExportVariables() {
    // Shape properties
    std::vector<std::string> shapeTypeOptions = {
        "Box", "Sphere", "Capsule", "Cylinder"
    };
    AddEnumExportVariable("shape_type", ShapeTypeToInt(m_shape_type), "Collision shape type", shapeTypeOptions);
    AddExportVariable("size", m_size, "Shape size (width/height/depth for box, radius for sphere)", ExportVariableType::Vec3);

    // Collision properties
    AddExportVariable("collision_layer", m_collision_layer, "Collision layer (0-31)", ExportVariableType::Int);
    AddExportVariable("collision_mask", m_collision_mask, "Collision mask (which layers to detect)", ExportVariableType::Int);

    // Area properties
    AddExportVariable("monitoring", m_monitoring, "Whether area is actively monitoring for overlaps", ExportVariableType::Bool);
    AddExportVariable("monitorable", m_monitorable, "Whether area can be detected by other areas", ExportVariableType::Bool);
}

void Area3D::UpdateExportVariables() {
    SetExportVariable("shape_type", ShapeTypeToInt(m_shape_type));
    SetExportVariable("size", m_size);
    SetExportVariable("collision_layer", m_collision_layer);
    SetExportVariable("collision_mask", m_collision_mask);
    SetExportVariable("monitoring", m_monitoring);
    SetExportVariable("monitorable", m_monitorable);
}

void Area3D::UpdateFromExportVariables() {
    // Get values from export variables
    CollisionShapeType new_shape_type = IntToShapeType(GetExportVariableValue<int>("shape_type", ShapeTypeToInt(m_shape_type)));
    glm::vec3 new_size = GetExportVariableValue<glm::vec3>("size", m_size);
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

void Area3D::RecreatePhysicsBody() {
    // Remove existing physics body
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody3D(m_physics_body);
        m_physics_body = nullptr;
    }

    // Get the owner node
    Node3D* node3d = dynamic_cast<Node3D*>(GetOwner());
    if (!node3d) {
        std::cerr << "Area3D can only be attached to Node3D!" << std::endl;
        return;
    }

    // Create physics body as a sensor (trigger)
    PhysicsMaterial material;
    material.density = 0.0f; // Areas have no mass

    // Check if there's a CollisionMesh3D component and use its data
    CollisionShapeType shape_type = m_shape_type;
    glm::vec3 shape_size = m_size;

    if (auto* collision_mesh = GetOwner()->GetComponent<CollisionMesh3D>()) {
        if (!collision_mesh->GetMeshPath().empty()) {
            std::cout << "Area3D using CollisionMesh3D: " << collision_mesh->GetMeshPath() << std::endl;
            m_physics_body = PhysicsManager::CreatePhysicsBody3D(node3d, PhysicsBodyType::Static, collision_mesh, material);
        } else {
            // Use primitive shape
            m_physics_body = PhysicsManager::CreatePhysicsBody3D(node3d, PhysicsBodyType::Static, shape_type, shape_size, material);
        }
    } else {
        // Use primitive collision shape
        std::cout << "Area3D using primitive shape: type=" << (int)shape_type
                  << " size=(" << shape_size.x << "," << shape_size.y << "," << shape_size.z << ")" << std::endl;
        m_physics_body = PhysicsManager::CreatePhysicsBody3D(node3d, PhysicsBodyType::Static, shape_type, shape_size, material);
    }

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

            if (auto* other_node3d = dynamic_cast<Node3D*>(other)) {
                // Check if this is an exit event (indicated by negative normal.x)
                bool is_enter = (normal.x >= 0.0f);
                OnCollision(other_node3d, is_enter);
            }
        });
    }
}

void Area3D::OnCollision(Node3D* other_node, bool entered) {
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

int Area3D::ShapeTypeToInt(CollisionShapeType type) const {
    switch (type) {
        case CollisionShapeType::Box: return 0;
        case CollisionShapeType::Sphere: return 1;
        case CollisionShapeType::Capsule: return 2;
        case CollisionShapeType::Cylinder: return 3;
        case CollisionShapeType::Mesh: return 4;
        case CollisionShapeType::Heightfield: return 5;
        default: return 0;
    }
}

CollisionShapeType Area3D::IntToShapeType(int value) const {
    switch (value) {
        case 0: return CollisionShapeType::Box;
        case 1: return CollisionShapeType::Sphere;
        case 2: return CollisionShapeType::Capsule;
        case 3: return CollisionShapeType::Cylinder;
        case 4: return CollisionShapeType::Mesh;
        case 5: return CollisionShapeType::Heightfield;
        default: return CollisionShapeType::Box;
    }
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(Area3D, "Physics", "3D area for trigger detection")
