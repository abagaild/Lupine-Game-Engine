#include "lupine/components/RigidBody2D.h"
#include "lupine/components/CollisionShape2D.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/physics/PhysicsManager.h"
#include <iostream>

namespace Lupine {

RigidBody2D::RigidBody2D()
    : Component("RigidBody2D")
    , m_body_type(PhysicsBodyType::Dynamic)
    , m_shape_type(CollisionShapeType::Box)
    , m_shape_size(1.0f, 1.0f)
    , m_material()
    , m_gravity_scale(1.0f)
    , m_collision_enabled(true)
    , m_collision_layer(0)
    , m_collision_mask(0xFFFF)
    , m_physics_body(nullptr)
    , m_needs_recreation(false) {
    
    InitializeExportVariables();
}

RigidBody2D::~RigidBody2D() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody2D(m_physics_body);
        m_physics_body = nullptr;
    }
}

void RigidBody2D::SetBodyType(PhysicsBodyType type) {
    if (m_body_type != type) {
        m_body_type = type;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void RigidBody2D::SetCollisionShape(CollisionShapeType shape_type, const glm::vec2& size) {
    if (m_shape_type != shape_type || m_shape_size != size) {
        m_shape_type = shape_type;
        m_shape_size = size;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void RigidBody2D::SetMaterial(const PhysicsMaterial& material) {
    m_material = material;
    if (m_physics_body) {
        // Update existing body properties
        m_physics_body->SetMass(material.density);
        m_physics_body->SetGravityScale(1.0f); // Reset to default, can be customized later
    }
    UpdateExportVariables();
}

void RigidBody2D::SetMass(float mass) {
    m_material.density = mass;
    if (m_physics_body) {
        m_physics_body->SetMass(mass);
    }
    UpdateExportVariables();
}

float RigidBody2D::GetMass() const {
    if (m_physics_body) {
        return m_physics_body->GetMass();
    }
    return m_material.density;
}

void RigidBody2D::SetGravityScale(float scale) {
    if (m_physics_body) {
        m_physics_body->SetGravityScale(scale);
    }
}

float RigidBody2D::GetGravityScale() const {
    if (m_physics_body) {
        return m_physics_body->GetGravityScale();
    }
    return 1.0f;
}

void RigidBody2D::SetLinearVelocity(const glm::vec2& velocity) {
    if (m_physics_body) {
        m_physics_body->SetLinearVelocity(velocity);
    }
}

glm::vec2 RigidBody2D::GetLinearVelocity() const {
    if (m_physics_body) {
        return m_physics_body->GetLinearVelocity();
    }
    return glm::vec2(0.0f);
}

void RigidBody2D::SetAngularVelocity(float velocity) {
    if (m_physics_body) {
        m_physics_body->SetAngularVelocity(velocity);
    }
}

float RigidBody2D::GetAngularVelocity() const {
    if (m_physics_body) {
        return m_physics_body->GetAngularVelocity();
    }
    return 0.0f;
}

void RigidBody2D::ApplyForce(const glm::vec2& force) {
    if (m_physics_body) {
        m_physics_body->ApplyForce(force);
    }
}

void RigidBody2D::ApplyForceAtPoint(const glm::vec2& force, const glm::vec2& point) {
    if (m_physics_body) {
        m_physics_body->ApplyForce(force, point);
    }
}

void RigidBody2D::ApplyImpulse(const glm::vec2& impulse) {
    if (m_physics_body) {
        m_physics_body->ApplyImpulse(impulse);
    }
}

void RigidBody2D::ApplyImpulseAtPoint(const glm::vec2& impulse, const glm::vec2& point) {
    if (m_physics_body) {
        m_physics_body->ApplyImpulse(impulse, point);
    }
}

void RigidBody2D::ApplyTorque(float torque) {
    if (m_physics_body) {
        m_physics_body->ApplyTorque(torque);
    }
}

void RigidBody2D::SetCollisionCallback(CollisionCallback callback) {
    m_collision_callback = callback;
    if (m_physics_body) {
        m_physics_body->SetCollisionCallback(callback);
    }
}

void RigidBody2D::SetCollisionEnabled(bool enabled) {
    m_collision_enabled = enabled;
    // TODO: Implement collision enable/disable in physics body
    UpdateExportVariables();
}

void RigidBody2D::SetCollisionLayer(int layer) {
    if (m_collision_layer != layer) {
        m_collision_layer = layer;
        if (m_physics_body) {
            m_physics_body->SetCollisionLayer(layer);
        }
        UpdateExportVariables();
    }
}

void RigidBody2D::SetCollisionMask(int mask) {
    if (m_collision_mask != mask) {
        m_collision_mask = mask;
        if (m_physics_body) {
            m_physics_body->SetCollisionMask(mask);
        }
        UpdateExportVariables();
    }
}

void RigidBody2D::OnReady() {
    RecreatePhysicsBody();
}

void RigidBody2D::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if CollisionShape2D size has changed and recreate physics body if needed
    if (auto* collision_shape = GetOwner()->GetComponent<CollisionShape2D>()) {
        glm::vec2 current_shape_size = collision_shape->GetSize();
        CollisionShapeType current_shape_type = collision_shape->GetShapeType();
        if (current_shape_size != m_shape_size || current_shape_type != m_shape_type) {
            std::cout << "RigidBody2D: CollisionShape2D changed from type=" << (int)m_shape_type
                      << " size=(" << m_shape_size.x << "," << m_shape_size.y
                      << ") to type=" << (int)current_shape_type
                      << " size=(" << current_shape_size.x << "," << current_shape_size.y
                      << "), recreating physics body" << std::endl;
            m_shape_size = current_shape_size;
            m_shape_type = current_shape_type;
            m_needs_recreation = true;
        }
    }

    // Check if we need to recreate the physics body
    if (m_needs_recreation) {
        RecreatePhysicsBody();
        m_needs_recreation = false;
    }

    // Update from export variables if they changed
    UpdateFromExportVariables();
}

void RigidBody2D::OnDestroy() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody2D(m_physics_body);
        m_physics_body = nullptr;
    }
}

void RigidBody2D::InitializeExportVariables() {
    // Body type enum
    std::vector<std::string> bodyTypeOptions = {
        "Static", "Kinematic", "Dynamic"
    };
    AddEnumExportVariable("body_type", static_cast<int>(m_body_type), "Physics body type", bodyTypeOptions);

    // Shape type enum
    std::vector<std::string> shapeTypeOptions = {
        "Box", "Circle", "Capsule"
    };
    AddEnumExportVariable("shape_type", static_cast<int>(m_shape_type), "Collision shape type", shapeTypeOptions);

    // Shape size - only expose if there's no CollisionShape2D component
    if (!GetOwner() || !GetOwner()->GetComponent<CollisionShape2D>()) {
        AddExportVariable("shape_size", m_shape_size, "Collision shape size", ExportVariableType::Vec2);
    }

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

void RigidBody2D::UpdateExportVariables() {
    // Export variables are automatically updated through their getters
}

void RigidBody2D::UpdateFromExportVariables() {
    // Update gravity scale from export variable
    auto gravity_scale_var = GetExportVariable("gravity_scale");
    if (gravity_scale_var && std::holds_alternative<float>(*gravity_scale_var)) {
        float new_gravity_scale = std::get<float>(*gravity_scale_var);
        if (new_gravity_scale != m_gravity_scale) {
            m_gravity_scale = new_gravity_scale;
            SetGravityScale(m_gravity_scale);
        }
    }
}

void RigidBody2D::RecreatePhysicsBody() {
    // Remove existing physics body
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody2D(m_physics_body);
        m_physics_body = nullptr;
    }
    
    // Get the owner node
    Node2D* node2d = dynamic_cast<Node2D*>(GetOwner());
    if (!node2d) {
        std::cerr << "RigidBody2D can only be attached to Node2D!" << std::endl;
        return;
    }
    
    // Check if there's a CollisionShape2D component and use its data
    CollisionShapeType shape_type = m_shape_type;
    glm::vec2 shape_size = m_shape_size;
    glm::vec2 shape_offset(0.0f, 0.0f);

    if (auto* collision_shape = GetOwner()->GetComponent<CollisionShape2D>()) {
        shape_type = collision_shape->GetShapeType();
        shape_size = collision_shape->GetSize();
        shape_offset = collision_shape->GetOffset();
        std::cout << "RigidBody2D using CollisionShape2D: type=" << (int)shape_type
                  << " size=(" << shape_size.x << "," << shape_size.y << ")"
                  << " offset=(" << shape_offset.x << "," << shape_offset.y << ")" << std::endl;
    }

    // Create new physics body
    m_physics_body = PhysicsManager::CreatePhysicsBody2D(node2d, m_body_type, shape_type, shape_size, m_material, shape_offset);

    if (m_physics_body) {
        // Set collision layer and mask
        m_physics_body->SetCollisionLayer(m_collision_layer);
        m_physics_body->SetCollisionMask(m_collision_mask);

        // Set collision callback if provided
        if (m_collision_callback) {
            m_physics_body->SetCollisionCallback(m_collision_callback);
        }
    }
}

glm::vec2 RigidBody2D::GetVelocity() const {
    if (m_physics_body) {
        return m_physics_body->GetLinearVelocity();
    }
    return glm::vec2(0.0f);
}

bool RigidBody2D::IsSleeping() const {
    if (m_physics_body) {
        return m_physics_body->IsSleeping();
    }
    return true;
}

void RigidBody2D::WakeUp() {
    if (m_physics_body && b2Body_IsValid(m_physics_body->GetBox2DBodyId())) {
        b2Body_SetAwake(m_physics_body->GetBox2DBodyId(), true);
    }
}

void RigidBody2D::Sleep() {
    if (m_physics_body && b2Body_IsValid(m_physics_body->GetBox2DBodyId())) {
        b2Body_SetAwake(m_physics_body->GetBox2DBodyId(), false);
    }
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(RigidBody2D, "Physics", "2D rigid body physics component")
