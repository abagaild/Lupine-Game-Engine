#include "lupine/components/TerrainCollider.h"
#include "lupine/components/TerrainRenderer.h"
#include "lupine/terrain/TerrainData.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Lupine {

TerrainCollider::TerrainCollider()
    : Component("TerrainCollider")
    , m_collision_type(TerrainCollisionType::Heightfield)
    , m_collision_enabled(true)
    , m_collision_layer(1)
    , m_collision_mask(0xFFFFFFFF)
    , m_collision_margin(0.04f)
    , m_friction(0.7f)
    , m_restitution(0.0f)
    , m_simplification_level(0.0f)
    , m_collision_resolution(1.0f)
    , m_terrain_data(nullptr)
    , m_terrain_renderer(nullptr)
    , m_collision_shape(nullptr)
    , m_rigid_body(nullptr)
    , m_collision_mesh_dirty(true)
    , m_needs_physics_update(true)
    , m_terrain_bounds_min(0.0f)
    , m_terrain_bounds_max(0.0f)
{
    InitializeExportVariables();
}

void TerrainCollider::SetCollisionType(TerrainCollisionType type) {
    if (m_collision_type != type) {
        m_collision_type = type;
        m_collision_mesh_dirty = true;
        UpdateExportVariables();
    }
}

void TerrainCollider::SetCollisionEnabled(bool enabled) {
    if (m_collision_enabled != enabled) {
        m_collision_enabled = enabled;
        m_needs_physics_update = true;
        UpdateExportVariables();
    }
}

void TerrainCollider::SetCollisionLayer(unsigned int layer) {
    if (m_collision_layer != layer) {
        m_collision_layer = layer;
        m_needs_physics_update = true;
        UpdateExportVariables();
    }
}

void TerrainCollider::SetCollisionMask(unsigned int mask) {
    if (m_collision_mask != mask) {
        m_collision_mask = mask;
        m_needs_physics_update = true;
        UpdateExportVariables();
    }
}

void TerrainCollider::SetCollisionMargin(float margin) {
    if (m_collision_margin != margin) {
        m_collision_margin = margin;
        m_collision_mesh_dirty = true;
        UpdateExportVariables();
    }
}

void TerrainCollider::SetFriction(float friction) {
    m_friction = glm::clamp(friction, 0.0f, 1.0f);
    m_needs_physics_update = true;
    UpdateExportVariables();
}

void TerrainCollider::SetRestitution(float restitution) {
    m_restitution = glm::clamp(restitution, 0.0f, 1.0f);
    m_needs_physics_update = true;
    UpdateExportVariables();
}

void TerrainCollider::SetSimplificationLevel(float level) {
    float clamped_level = glm::clamp(level, 0.0f, 1.0f);
    if (m_simplification_level != clamped_level) {
        m_simplification_level = clamped_level;
        m_collision_mesh_dirty = true;
        UpdateExportVariables();
    }
}

void TerrainCollider::SetCollisionResolution(float resolution) {
    if (resolution > 0.0f && m_collision_resolution != resolution) {
        m_collision_resolution = resolution;
        m_collision_mesh_dirty = true;
        UpdateExportVariables();
    }
}

void TerrainCollider::RegenerateCollisionMesh() {
    m_collision_mesh_dirty = true;
}

void TerrainCollider::UpdateCollisionRegion(const glm::vec3& min_bounds, const glm::vec3& max_bounds) {
    // For now, regenerate entire collision mesh
    // In a full implementation, this would update only the specified region
    std::cout << "TerrainCollider: Updating collision region (" << min_bounds.x << ", " << min_bounds.y << ", " << min_bounds.z 
              << ") to (" << max_bounds.x << ", " << max_bounds.y << ", " << max_bounds.z << ")" << std::endl;
    m_collision_mesh_dirty = true;
}

void TerrainCollider::SetTerrainRenderer(TerrainRenderer* terrain_renderer) {
    m_terrain_renderer = terrain_renderer;
    if (terrain_renderer) {
        m_terrain_data = terrain_renderer->GetTerrainData();
        m_collision_mesh_dirty = true;
    }
}

void TerrainCollider::SetTerrainData(std::shared_ptr<TerrainData> data) {
    m_terrain_data = data;
    m_collision_mesh_dirty = true;
}

bool TerrainCollider::Raycast(const glm::vec3& ray_origin, const glm::vec3& ray_direction, 
                             float max_distance, glm::vec3& hit_point, glm::vec3& hit_normal) const {
    if (!m_terrain_data || !m_collision_enabled) return false;
    
    // Simple raycast implementation - will be enhanced when we have proper terrain data
    // For now, just check if ray hits a horizontal plane at y=0
    if (std::abs(ray_direction.y) < 0.001f) return false; // Ray is horizontal
    
    float t = -ray_origin.y / ray_direction.y;
    if (t < 0.0f || t > max_distance) return false;
    
    hit_point = ray_origin + t * ray_direction;
    hit_normal = glm::vec3(0.0f, 1.0f, 0.0f);
    
    return IsPointInBounds(hit_point);
}

bool TerrainCollider::IsPointInBounds(const glm::vec3& point) const {
    return point.x >= m_terrain_bounds_min.x && point.x <= m_terrain_bounds_max.x &&
           point.z >= m_terrain_bounds_min.z && point.z <= m_terrain_bounds_max.z;
}

float TerrainCollider::GetHeightAtPosition(const glm::vec3& world_pos) const {
    if (!m_terrain_data || !IsPointInBounds(world_pos)) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    
    // This will be implemented when we have proper terrain data
    return 0.0f;
}

void TerrainCollider::OnReady() {
    UpdateFromExportVariables();
    
    // Try to find terrain renderer on the same node if not set
    if (!m_terrain_renderer && GetOwner()) {
        m_terrain_renderer = GetOwner()->GetComponent<TerrainRenderer>();
        if (m_terrain_renderer) {
            m_terrain_data = m_terrain_renderer->GetTerrainData();
        }
    }
    
    if (m_collision_mesh_dirty) {
        CreateCollisionShape();
    }
}

void TerrainCollider::OnUpdate(float delta_time) {
    (void)delta_time;
    
    UpdateFromExportVariables();
    
    if (m_collision_mesh_dirty) {
        UpdateCollisionShape();
        m_collision_mesh_dirty = false;
    }
    
    if (m_needs_physics_update) {
        UpdatePhysicsBody();
        m_needs_physics_update = false;
    }
}

void TerrainCollider::OnPhysicsProcess(float delta_time) {
    (void)delta_time;
    // Physics processing will be implemented when we have physics integration
}

void TerrainCollider::InitializeExportVariables() {
    // Collision type
    std::vector<std::string> collisionTypeOptions = {
        "None", "Heightfield", "Trimesh", "ConvexHull", "Simplified"
    };
    AddEnumExportVariable("collision_type", CollisionTypeToInt(m_collision_type), "Collision shape type", collisionTypeOptions);
    
    // Collision properties
    AddExportVariable("collision_enabled", m_collision_enabled, "Enable collision detection", ExportVariableType::Bool);
    AddExportVariable("collision_layer", static_cast<int>(m_collision_layer), "Collision layer bitmask", ExportVariableType::Int);
    AddExportVariable("collision_mask", static_cast<int>(m_collision_mask), "Collision mask bitmask", ExportVariableType::Int);
    AddExportVariable("collision_margin", m_collision_margin, "Collision margin for stability", ExportVariableType::Float);
    AddExportVariable("friction", m_friction, "Surface friction coefficient", ExportVariableType::Float);
    AddExportVariable("restitution", m_restitution, "Surface bounciness", ExportVariableType::Float);
    
    // Mesh generation
    AddExportVariable("simplification_level", m_simplification_level, "Mesh simplification level", ExportVariableType::Float);
    AddExportVariable("collision_resolution", m_collision_resolution, "Collision mesh resolution", ExportVariableType::Float);
}

void TerrainCollider::UpdateFromExportVariables() {
    m_collision_type = IntToCollisionType(GetExportVariableValue<int>("collision_type", CollisionTypeToInt(m_collision_type)));
    m_collision_enabled = GetExportVariableValue<bool>("collision_enabled", m_collision_enabled);
    m_collision_layer = static_cast<unsigned int>(GetExportVariableValue<int>("collision_layer", static_cast<int>(m_collision_layer)));
    m_collision_mask = static_cast<unsigned int>(GetExportVariableValue<int>("collision_mask", static_cast<int>(m_collision_mask)));
    m_collision_margin = GetExportVariableValue<float>("collision_margin", m_collision_margin);
    m_friction = GetExportVariableValue<float>("friction", m_friction);
    m_restitution = GetExportVariableValue<float>("restitution", m_restitution);
    m_simplification_level = GetExportVariableValue<float>("simplification_level", m_simplification_level);
    m_collision_resolution = GetExportVariableValue<float>("collision_resolution", m_collision_resolution);
}

void TerrainCollider::UpdateExportVariables() {
    SetExportVariable("collision_type", CollisionTypeToInt(m_collision_type));
    SetExportVariable("collision_enabled", m_collision_enabled);
    SetExportVariable("collision_layer", static_cast<int>(m_collision_layer));
    SetExportVariable("collision_mask", static_cast<int>(m_collision_mask));
    SetExportVariable("collision_margin", m_collision_margin);
    SetExportVariable("friction", m_friction);
    SetExportVariable("restitution", m_restitution);
    SetExportVariable("simplification_level", m_simplification_level);
    SetExportVariable("collision_resolution", m_collision_resolution);
}

void TerrainCollider::CreateCollisionShape() {
    if (!m_terrain_data || !m_collision_enabled) return;
    
    // Destroy existing collision shape
    DestroyCollisionShape();
    
    // Create new collision shape based on type
    switch (m_collision_type) {
        case TerrainCollisionType::None:
            break;
        case TerrainCollisionType::Heightfield:
            GenerateHeightfieldCollision();
            break;
        case TerrainCollisionType::Trimesh:
            GenerateTrimeshCollision();
            break;
        case TerrainCollisionType::ConvexHull:
            GenerateConvexHullCollision();
            break;
        case TerrainCollisionType::Simplified:
            GenerateSimplifiedCollision();
            break;
    }
    
    std::cout << "TerrainCollider: Created collision shape of type " << static_cast<int>(m_collision_type) << std::endl;
}

void TerrainCollider::UpdateCollisionShape() {
    CreateCollisionShape();
}

void TerrainCollider::DestroyCollisionShape() {
    // Clean up physics objects
    if (m_rigid_body) {
        // Physics engine cleanup will be implemented
        m_rigid_body = nullptr;
    }
    
    if (m_collision_shape) {
        // Physics engine cleanup will be implemented
        m_collision_shape = nullptr;
    }
}

void TerrainCollider::GenerateHeightfieldCollision() {
    std::cout << "TerrainCollider: Generating heightfield collision" << std::endl;
    // Heightfield collision generation will be implemented when we have physics integration
}

void TerrainCollider::GenerateTrimeshCollision() {
    std::cout << "TerrainCollider: Generating trimesh collision" << std::endl;
    // Trimesh collision generation will be implemented when we have physics integration
}

void TerrainCollider::GenerateConvexHullCollision() {
    std::cout << "TerrainCollider: Generating convex hull collision" << std::endl;
    // Convex hull collision generation will be implemented when we have physics integration
}

void TerrainCollider::GenerateSimplifiedCollision() {
    std::cout << "TerrainCollider: Generating simplified collision" << std::endl;
    // Simplified collision generation will be implemented when we have physics integration
}

void TerrainCollider::UpdatePhysicsBody() {
    if (!m_collision_shape) return;
    
    std::cout << "TerrainCollider: Updating physics body properties" << std::endl;
    // Physics body update will be implemented when we have physics integration
}

std::vector<glm::vec3> TerrainCollider::GenerateCollisionVertices() const {
    std::vector<glm::vec3> vertices;
    
    if (!m_terrain_data) return vertices;
    
    // Generate vertices based on terrain data and resolution
    // This will be implemented when we have TerrainData structure
    
    return vertices;
}

std::vector<unsigned int> TerrainCollider::GenerateCollisionIndices() const {
    std::vector<unsigned int> indices;
    
    if (!m_terrain_data) return indices;
    
    // Generate triangle indices for collision mesh
    // This will be implemented when we have TerrainData structure
    
    return indices;
}

int TerrainCollider::CollisionTypeToInt(TerrainCollisionType type) const {
    switch (type) {
        case TerrainCollisionType::None: return 0;
        case TerrainCollisionType::Heightfield: return 1;
        case TerrainCollisionType::Trimesh: return 2;
        case TerrainCollisionType::ConvexHull: return 3;
        case TerrainCollisionType::Simplified: return 4;
        default: return 1;
    }
}

TerrainCollisionType TerrainCollider::IntToCollisionType(int value) const {
    switch (value) {
        case 0: return TerrainCollisionType::None;
        case 1: return TerrainCollisionType::Heightfield;
        case 2: return TerrainCollisionType::Trimesh;
        case 3: return TerrainCollisionType::ConvexHull;
        case 4: return TerrainCollisionType::Simplified;
        default: return TerrainCollisionType::Heightfield;
    }
}

} // namespace Lupine
