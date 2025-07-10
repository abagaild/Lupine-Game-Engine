#include "lupine/components/CollisionMesh3D.h"
#include "lupine/nodes/Node3D.h"
#include <iostream>

namespace Lupine {

CollisionMesh3D::CollisionMesh3D()
    : Component("CollisionMesh3D")
    , m_mesh_path("")
    , m_mesh_type(MeshType::Convex)
    , m_scale(1.0f, 1.0f, 1.0f)
    , m_offset(0.0f, 0.0f, 0.0f)
    , m_is_trigger(false)
    , m_collision_layer(0)
    , m_collision_mask(0xFFFF)
    , m_mesh_loaded(false)
    , m_needs_update(false) {
    
    InitializeExportVariables();
}

void CollisionMesh3D::SetMeshPath(const std::string& path) {
    if (m_mesh_path != path) {
        m_mesh_path = path;
        m_mesh_loaded = false;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionMesh3D::SetMeshType(MeshType type) {
    if (m_mesh_type != type) {
        m_mesh_type = type;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionMesh3D::SetScale(const glm::vec3& scale) {
    if (m_scale != scale) {
        m_scale = scale;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionMesh3D::SetOffset(const glm::vec3& offset) {
    if (m_offset != offset) {
        m_offset = offset;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionMesh3D::SetTrigger(bool trigger) {
    if (m_is_trigger != trigger) {
        m_is_trigger = trigger;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionMesh3D::SetCollisionLayer(int layer) {
    if (m_collision_layer != layer) {
        m_collision_layer = layer;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

void CollisionMesh3D::SetCollisionMask(int mask) {
    if (m_collision_mask != mask) {
        m_collision_mask = mask;
        m_needs_update = true;
        UpdateExportVariables();
    }
}

bool CollisionMesh3D::CollidesWithLayer(int layer) const {
    return (m_collision_mask & (1 << layer)) != 0;
}

void CollisionMesh3D::OnReady() {
    UpdateFromExportVariables();
    if (!m_mesh_path.empty()) {
        LoadMesh();
    }
}

void CollisionMesh3D::OnUpdate(float delta_time) {
    (void)delta_time;
    
    // Update from export variables if they changed
    UpdateFromExportVariables();
    
    // Load mesh if needed
    if (!m_mesh_path.empty() && !m_mesh_loaded) {
        LoadMesh();
    }
    
    // Note: This component defines collision shapes but doesn't create physics bodies itself.
    // Physics bodies (RigidBody3D, KinematicBody3D, Area3D) will use this component's data.
}

void CollisionMesh3D::InitializeExportVariables() {
    // Mesh properties
    AddExportVariable("mesh_path", m_mesh_path, "Path to collision mesh file", ExportVariableType::FilePath);

    // Create enum options for mesh type
    std::vector<std::string> meshTypeOptions = {
        "Convex", "Trimesh", "Simplified"
    };
    AddEnumExportVariable("mesh_type", MeshTypeToInt(m_mesh_type), "Mesh collision type", meshTypeOptions);

    AddExportVariable("scale", m_scale, "Mesh scale factor", ExportVariableType::Vec3);
    AddExportVariable("offset", m_offset, "Mesh offset from node center", ExportVariableType::Vec3);
    
    // Collision properties
    AddExportVariable("is_trigger", m_is_trigger, "Whether shape is a trigger (no collision response)", ExportVariableType::Bool);
    AddExportVariable("collision_layer", m_collision_layer, "Collision layer (0-31)", ExportVariableType::Int);
    AddExportVariable("collision_mask", m_collision_mask, "Collision mask (which layers to collide with)", ExportVariableType::Int);
}

void CollisionMesh3D::UpdateExportVariables() {
    SetExportVariable("mesh_path", m_mesh_path);
    SetExportVariable("mesh_type", MeshTypeToInt(m_mesh_type));
    SetExportVariable("scale", m_scale);
    SetExportVariable("offset", m_offset);
    SetExportVariable("is_trigger", m_is_trigger);
    SetExportVariable("collision_layer", m_collision_layer);
    SetExportVariable("collision_mask", m_collision_mask);
}

void CollisionMesh3D::UpdateFromExportVariables() {
    // Get values from export variables
    std::string new_mesh_path = GetExportVariableValue<std::string>("mesh_path", m_mesh_path);
    MeshType new_mesh_type = IntToMeshType(GetExportVariableValue<int>("mesh_type", MeshTypeToInt(m_mesh_type)));
    glm::vec3 new_scale = GetExportVariableValue<glm::vec3>("scale", m_scale);
    glm::vec3 new_offset = GetExportVariableValue<glm::vec3>("offset", m_offset);
    bool new_is_trigger = GetExportVariableValue<bool>("is_trigger", m_is_trigger);
    int new_collision_layer = GetExportVariableValue<int>("collision_layer", m_collision_layer);
    int new_collision_mask = GetExportVariableValue<int>("collision_mask", m_collision_mask);
    
    // Check if any values changed
    bool changed = false;
    if (m_mesh_path != new_mesh_path) {
        m_mesh_path = new_mesh_path;
        m_mesh_loaded = false;
        changed = true;
    }
    if (m_mesh_type != new_mesh_type) {
        m_mesh_type = new_mesh_type;
        changed = true;
    }
    if (m_scale != new_scale) {
        m_scale = new_scale;
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

void CollisionMesh3D::LoadMesh() {
    if (m_mesh_path.empty()) {
        // No external mesh path, will use primitive mesh from PrimitiveMesh component
        m_mesh_loaded = true;
        return;
    }

    std::cout << "CollisionMesh3D: Loading mesh from " << m_mesh_path << std::endl;

    // The actual mesh loading and collision shape creation is handled by PhysicsManager
    // when a physics body is created. This component just stores the configuration.
    m_mesh_loaded = true;

    std::cout << "CollisionMesh3D: Mesh configuration loaded successfully" << std::endl;
}

int CollisionMesh3D::MeshTypeToInt(MeshType type) const {
    switch (type) {
        case MeshType::Convex: return 0;
        case MeshType::Trimesh: return 1;
        case MeshType::Simplified: return 2;
        default: return 0;
    }
}

CollisionMesh3D::MeshType CollisionMesh3D::IntToMeshType(int value) const {
    switch (value) {
        case 0: return MeshType::Convex;
        case 1: return MeshType::Trimesh;
        case 2: return MeshType::Simplified;
        default: return MeshType::Convex;
    }
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(CollisionMesh3D, "Physics", "3D collision mesh for complex shapes")
