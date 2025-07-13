#include "lupine/components/RigidBody3D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/physics/PhysicsManager.h"
#include "lupine/components/CollisionMesh3D.h"
#include <iostream>

namespace Lupine {

RigidBody3D::RigidBody3D()
    : Component("RigidBody3D")
    , m_body_type(PhysicsBodyType::Dynamic)
    , m_shape_type(CollisionShapeType::Box)
    , m_shape_size(1.0f, 1.0f, 1.0f)
    , m_material()
    , m_gravity_scale(1.0f)
    , m_collision_enabled(true)
    , m_collision_layer(0)
    , m_collision_mask(0xFFFF)
    , m_physics_body(nullptr)
    , m_needs_recreation(false) {
    
    InitializeExportVariables();
}

RigidBody3D::~RigidBody3D() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody3D(m_physics_body);
        m_physics_body = nullptr;
    }
}

void RigidBody3D::SetBodyType(PhysicsBodyType type) {
    if (m_body_type != type) {
        m_body_type = type;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void RigidBody3D::SetCollisionShape(CollisionShapeType shape_type, const glm::vec3& size) {
    if (m_shape_type != shape_type || m_shape_size != size) {
        m_shape_type = shape_type;
        m_shape_size = size;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void RigidBody3D::SetMaterial(const PhysicsMaterial& material) {
    m_material = material;
    if (m_physics_body) {
        m_physics_body->SetMass(material.density);
        m_physics_body->SetGravityScale(1.0f);
    }
    UpdateExportVariables();
}

void RigidBody3D::SetMass(float mass) {
    m_material.density = mass;
    if (m_physics_body) {
        m_physics_body->SetMass(mass);
    }
    UpdateExportVariables();
}

float RigidBody3D::GetMass() const {
    if (m_physics_body) {
        return m_physics_body->GetMass();
    }
    return m_material.density;
}

void RigidBody3D::SetGravityScale(float scale) {
    if (m_physics_body) {
        m_physics_body->SetGravityScale(scale);
    }
}

float RigidBody3D::GetGravityScale() const {
    if (m_physics_body) {
        return m_physics_body->GetGravityScale();
    }
    return 1.0f;
}

void RigidBody3D::SetLinearVelocity(const glm::vec3& velocity) {
    if (m_physics_body) {
        m_physics_body->SetLinearVelocity(velocity);
    }
}

glm::vec3 RigidBody3D::GetLinearVelocity() const {
    if (m_physics_body) {
        return m_physics_body->GetLinearVelocity();
    }
    return glm::vec3(0.0f);
}

void RigidBody3D::SetAngularVelocity(const glm::vec3& velocity) {
    if (m_physics_body) {
        m_physics_body->SetAngularVelocity(velocity);
    }
}

glm::vec3 RigidBody3D::GetAngularVelocity() const {
    if (m_physics_body) {
        return m_physics_body->GetAngularVelocity();
    }
    return glm::vec3(0.0f);
}

void RigidBody3D::ApplyForce(const glm::vec3& force) {
    if (m_physics_body) {
        m_physics_body->ApplyForce(force);
    }
}

void RigidBody3D::ApplyForceAtPoint(const glm::vec3& force, const glm::vec3& point) {
    if (m_physics_body) {
        m_physics_body->ApplyForce(force, point);
    }
}

void RigidBody3D::ApplyImpulse(const glm::vec3& impulse) {
    if (m_physics_body) {
        m_physics_body->ApplyImpulse(impulse);
    }
}

void RigidBody3D::ApplyImpulseAtPoint(const glm::vec3& impulse, const glm::vec3& point) {
    if (m_physics_body) {
        m_physics_body->ApplyImpulse(impulse, point);
    }
}

void RigidBody3D::ApplyTorque(const glm::vec3& torque) {
    if (m_physics_body) {
        m_physics_body->ApplyTorque(torque);
    }
}

void RigidBody3D::SetCollisionCallback(CollisionCallback callback) {
    m_collision_callback = callback;
    if (m_physics_body) {
        m_physics_body->SetCollisionCallback(callback);
    }
}

void RigidBody3D::SetCollisionEnabled(bool enabled) {
    m_collision_enabled = enabled;
    UpdateExportVariables();
}

void RigidBody3D::SetCollisionLayer(int layer) {
    m_collision_layer = layer;
    UpdateExportVariables();
}

void RigidBody3D::SetCollisionMask(int mask) {
    m_collision_mask = mask;
    UpdateExportVariables();
}

void RigidBody3D::OnReady() {
    // Load export variables first to get correct values from scene file
    UpdateFromExportVariables();
    RecreatePhysicsBody();
}

void RigidBody3D::OnUpdate(float delta_time) {
    (void)delta_time;

    if (m_needs_recreation) {
        RecreatePhysicsBody();
        m_needs_recreation = false;
    }

    UpdateFromExportVariables();

    // Debug: Print position every few frames for dynamic bodies
    static int frame_counter = 0;
    frame_counter++;
    if (frame_counter % 60 == 0 && m_body_type == PhysicsBodyType::Dynamic) {
        auto* node3d = dynamic_cast<Node3D*>(GetOwner());
        if (node3d) {
            glm::vec3 pos = node3d->GetPosition();
            std::cout << "RigidBody3D [" << node3d->GetName() << "] position: ("
                      << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
        }
    }
}

void RigidBody3D::OnDestroy() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody3D(m_physics_body);
        m_physics_body = nullptr;
    }
}

void RigidBody3D::InitializeExportVariables() {
    // Body type
    std::vector<std::string> bodyTypeOptions = {
        "Static", "Kinematic", "Dynamic"
    };
    AddEnumExportVariable("body_type", static_cast<int>(m_body_type), "Physics body type", bodyTypeOptions);

    // Shape type
    std::vector<std::string> shapeTypeOptions = {
        "Box", "Sphere", "Capsule", "Cylinder", "Mesh"
    };
    AddEnumExportVariable("shape_type", static_cast<int>(m_shape_type), "Collision shape type", shapeTypeOptions);

    // Shape size
    AddExportVariable("shape_size", m_shape_size, "Collision shape size", ExportVariableType::Vec3);

    // Material properties
    AddExportVariable("density", m_material.density, "Physics material density", ExportVariableType::Float);
    AddExportVariable("friction", m_material.friction, "Physics material friction", ExportVariableType::Float);
    AddExportVariable("restitution", m_material.restitution, "Physics material restitution (bounciness)", ExportVariableType::Float);

    // Gravity properties
    AddExportVariable("gravity_scale", m_gravity_scale, "Gravity scale multiplier (1.0 = normal gravity, 0.0 = no gravity)", ExportVariableType::Float);

    // Collision properties
    AddExportVariable("collision_enabled", m_collision_enabled, "Enable collision detection", ExportVariableType::Bool);
    AddExportVariable("collision_layer", m_collision_layer, "Collision layer", ExportVariableType::Int);
    AddExportVariable("collision_mask", m_collision_mask, "Collision mask", ExportVariableType::Int);
}

void RigidBody3D::UpdateExportVariables() {
    // Export variables are automatically updated through their getters
}

void RigidBody3D::UpdateFromExportVariables() {
    bool needs_update = false;

    // Update body type from export variable
    auto body_type_var = GetExportVariable("body_type");
    if (body_type_var && std::holds_alternative<int>(*body_type_var)) {
        int new_body_type = std::get<int>(*body_type_var);
        PhysicsBodyType new_type = static_cast<PhysicsBodyType>(new_body_type);
        if (new_type != m_body_type) {
            m_body_type = new_type;
            needs_update = true;
        }
    }

    // Update shape type from export variable
    auto shape_type_var = GetExportVariable("shape_type");
    if (shape_type_var && std::holds_alternative<int>(*shape_type_var)) {
        int new_shape_type = std::get<int>(*shape_type_var);
        CollisionShapeType new_type = static_cast<CollisionShapeType>(new_shape_type);
        if (new_type != m_shape_type) {
            m_shape_type = new_type;
            needs_update = true;
        }
    }

    // Update shape size from export variable
    auto shape_size_var = GetExportVariable("shape_size");
    if (shape_size_var && std::holds_alternative<glm::vec3>(*shape_size_var)) {
        glm::vec3 new_shape_size = std::get<glm::vec3>(*shape_size_var);
        if (new_shape_size != m_shape_size) {
            m_shape_size = new_shape_size;
            needs_update = true;
        }
    }

    // Update gravity scale from export variable
    auto gravity_scale_var = GetExportVariable("gravity_scale");
    if (gravity_scale_var && std::holds_alternative<float>(*gravity_scale_var)) {
        float new_gravity_scale = std::get<float>(*gravity_scale_var);
        if (new_gravity_scale != m_gravity_scale) {
            m_gravity_scale = new_gravity_scale;
            SetGravityScale(m_gravity_scale);
        }
    }

    // Only set needs_recreation if we actually have changes and a physics body exists
    if (needs_update && m_physics_body) {
        m_needs_recreation = true;
    }
}

void RigidBody3D::RecreatePhysicsBody() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody3D(m_physics_body);
        m_physics_body = nullptr;
    }

    Node3D* node3d = dynamic_cast<Node3D*>(GetOwner());
    if (!node3d) {
        std::cerr << "RigidBody3D can only be attached to Node3D!" << std::endl;
        return;
    }

    // Check if there's a CollisionMesh3D component for mesh-based collision
    CollisionMesh3D* collision_mesh = node3d->GetComponent<CollisionMesh3D>();

    std::cout << "Creating RigidBody3D for node: " << node3d->GetName()
              << " (body_type: " << static_cast<int>(m_body_type) << ")" << std::endl;

    if (collision_mesh && (m_shape_type == CollisionShapeType::Mesh ||
                          !collision_mesh->GetMeshPath().empty())) {
        // Use mesh-based collision
        std::cout << "  Using mesh-based collision" << std::endl;
        m_physics_body = PhysicsManager::CreatePhysicsBody3D(node3d, m_body_type, collision_mesh, m_material);
    } else {
        // Use primitive collision shape
        std::cout << "  Using primitive collision shape (type: " << static_cast<int>(m_shape_type)
                  << ", size: " << m_shape_size.x << "x" << m_shape_size.y << "x" << m_shape_size.z << ")" << std::endl;
        m_physics_body = PhysicsManager::CreatePhysicsBody3D(node3d, m_body_type, m_shape_type, m_shape_size, m_material);
    }

    if (m_physics_body) {
        std::cout << "  Physics body created successfully!" << std::endl;
        if (m_collision_callback) {
            m_physics_body->SetCollisionCallback(m_collision_callback);
        }
    } else {
        std::cout << "  ERROR: Failed to create physics body!" << std::endl;
    }
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(RigidBody3D, "Physics", "3D rigid body physics component")
