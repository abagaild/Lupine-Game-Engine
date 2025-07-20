#include "lupine/components/KinematicBody2D.h"
#include "lupine/components/CollisionShape2D.h"
#include "lupine/nodes/Node2D.h"
#include <iostream>

namespace Lupine {

KinematicBody2D::KinematicBody2D()
    : Component("KinematicBody2D")
    , m_shape_type(CollisionShapeType::Box)
    , m_size(1.0f, 1.0f)
    , m_collision_layer(0)
    , m_collision_mask(0xFFFF)
    , m_velocity(0.0f, 0.0f)
    , m_last_collision_normal(0.0f, 0.0f)
    , m_on_floor(false)
    , m_on_wall(false)
    , m_on_ceiling(false)
    , m_physics_body(nullptr)
    , m_needs_recreation(false) {
    
    InitializeExportVariables();
}

KinematicBody2D::~KinematicBody2D() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody2D(m_physics_body);
        m_physics_body = nullptr;
    }
}

void KinematicBody2D::SetShapeType(CollisionShapeType type) {
    if (m_shape_type != type) {
        m_shape_type = type;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void KinematicBody2D::SetSize(const glm::vec2& size) {
    if (m_size != size) {
        m_size = size;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

bool KinematicBody2D::MoveAndSlide(const glm::vec2& velocity, float delta_time) {
    m_velocity = velocity;

    auto* node2d = dynamic_cast<Node2D*>(GetOwner());
    if (!node2d || !m_physics_body) {
        std::cout << "KinematicBody2D::MoveAndSlide: Early return - node2d=" << (node2d ? "valid" : "null")
                  << " physics_body=" << (m_physics_body ? "valid" : "null") << std::endl;
        return false;
    }

    std::cout << "KinematicBody2D::MoveAndSlide: velocity=(" << velocity.x << "," << velocity.y
              << ") delta_time=" << delta_time << " current_pos=(" << node2d->GetPosition().x
              << "," << node2d->GetPosition().y << ")" << std::endl;

    // Reset collision flags
    m_on_floor = false;
    m_on_wall = false;
    m_on_ceiling = false;
    m_last_collision_normal = glm::vec2(0.0f);

    glm::vec2 movement = velocity * delta_time;
    glm::vec2 current_pos = node2d->GetPosition();

    // If no movement, just return
    if (glm::length(movement) < 0.001f) {
        return true;
    }

    glm::vec2 final_position = current_pos;
    glm::vec2 remaining_movement = movement;
    const int max_iterations = 4; // Prevent infinite loops
    const float min_movement_threshold = 0.001f;
    bool moved = false;

    for (int iteration = 0; iteration < max_iterations && glm::length(remaining_movement) > min_movement_threshold; ++iteration) {
        glm::vec2 target_position = final_position + remaining_movement;

        // Perform shape cast to detect collisions
        PhysicsBody2D* hit_body = nullptr;
        glm::vec2 hit_point;
        glm::vec2 hit_normal;
        float hit_fraction;

        bool collision_detected = PhysicsManager::ShapeCast2D(
            m_physics_body,
            final_position,
            target_position,
            hit_body,
            hit_point,
            hit_normal,
            hit_fraction
        );

        if (collision_detected && hit_fraction > 0.0f) {
            // Move to the collision point (with a small margin to prevent overlap)
            const float collision_margin = 0.01f;
            float safe_fraction = std::max(0.0f, hit_fraction - collision_margin / glm::length(remaining_movement));
            final_position += remaining_movement * safe_fraction;
            moved = true;

            // Store collision information
            m_last_collision_normal = hit_normal;

            // Determine collision type based on normal
            const float floor_threshold = 0.7f; // cos(45 degrees)
            if (hit_normal.y > floor_threshold) {
                m_on_floor = true;
            } else if (hit_normal.y < -floor_threshold) {
                m_on_ceiling = true;
            } else {
                m_on_wall = true;
            }

            std::cout << "KinematicBody2D: Collision detected at fraction " << hit_fraction
                      << " with normal (" << hit_normal.x << ", " << hit_normal.y << ")" << std::endl;

            // Calculate remaining movement after collision
            glm::vec2 remaining_after_collision = remaining_movement * (1.0f - safe_fraction);

            // Project remaining movement along the collision surface (slide)
            float dot_product = glm::dot(remaining_after_collision, hit_normal);
            remaining_movement = remaining_after_collision - hit_normal * dot_product;

            // Call collision callback if set
            if (m_collision_callback && hit_body) {
                glm::vec3 contact_point_3d(hit_point.x, hit_point.y, 0.0f);
                glm::vec3 normal_3d(hit_normal.x, hit_normal.y, 0.0f);
                m_collision_callback(GetOwner(), hit_body->GetOwner(), contact_point_3d, normal_3d);
            }
        } else {
            // No collision, move to target position
            final_position = target_position;
            moved = true;
            break;
        }
    }

    if (moved) {
        // Update physics body position
        m_physics_body->SetPosition(final_position);

        // Update node position to match physics body
        node2d->SetPosition(final_position);
    }

    return moved;
}

bool KinematicBody2D::MoveAndCollide(const glm::vec2& velocity, float delta_time) {
    m_velocity = velocity;

    auto* node2d = dynamic_cast<Node2D*>(GetOwner());
    if (!node2d || !m_physics_body) {
        return false;
    }

    glm::vec2 movement = velocity * delta_time;
    glm::vec2 current_pos = node2d->GetPosition();
    glm::vec2 target_pos = current_pos + movement;

    // TODO: Implement proper collision detection using Box2D queries
    // For now, just move and return whether we would have collided

    m_physics_body->SetPosition(target_pos);
    node2d->SetPosition(target_pos);

    return false; // No collision detected in this simplified implementation
}

void KinematicBody2D::SetVelocity(const glm::vec2& velocity) {
    m_velocity = velocity;
}

void KinematicBody2D::SetMaterial(const PhysicsMaterial& material) {
    m_material = material;
    UpdateExportVariables();
}

void KinematicBody2D::SetCollisionLayer(int layer) {
    if (m_collision_layer != layer) {
        m_collision_layer = layer;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void KinematicBody2D::SetCollisionMask(int mask) {
    if (m_collision_mask != mask) {
        m_collision_mask = mask;
        m_needs_recreation = true;
        UpdateExportVariables();
    }
}

void KinematicBody2D::SetCollisionCallback(CollisionCallback callback) {
    m_collision_callback = callback;
    if (m_physics_body) {
        m_physics_body->SetCollisionCallback(callback);
    }
}

bool KinematicBody2D::IsOnFloor(const glm::vec2& floor_normal) const {
    (void)floor_normal;
    return m_on_floor;
}

bool KinematicBody2D::IsOnWall(const glm::vec2& wall_normal) const {
    (void)wall_normal;
    return m_on_wall;
}

bool KinematicBody2D::IsOnCeiling(const glm::vec2& ceiling_normal) const {
    (void)ceiling_normal;
    return m_on_ceiling;
}

void KinematicBody2D::OnReady() {
    UpdateFromExportVariables();
    RecreatePhysicsBody();
}

void KinematicBody2D::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if CollisionShape2D size has changed and recreate physics body if needed
    if (auto* collision_shape = GetOwner()->GetComponent<CollisionShape2D>()) {
        glm::vec2 current_shape_size = collision_shape->GetSize();
        if (current_shape_size != m_size) {
            std::cout << "KinematicBody2D: CollisionShape2D size changed from (" << m_size.x << "," << m_size.y
                      << ") to (" << current_shape_size.x << "," << current_shape_size.y << "), recreating physics body" << std::endl;
            m_size = current_shape_size;
            m_needs_recreation = true;
        }
    }

    if (m_needs_recreation) {
        RecreatePhysicsBody();
        m_needs_recreation = false;
    }

    // Note: We don't call UpdateFromExportVariables() here because we manually
    // sync with CollisionShape2D size, which would conflict with our own size export variable
}

void KinematicBody2D::OnDestroy() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody2D(m_physics_body);
        m_physics_body = nullptr;
    }
}

void KinematicBody2D::InitializeExportVariables() {
    // Shape type enum
    std::vector<std::string> shapeTypeOptions = {
        "Box", "Circle", "Capsule"
    };
    AddEnumExportVariable("shape_type", ShapeTypeToInt(m_shape_type), "Collision shape type", shapeTypeOptions);

    // Only expose size if there's no CollisionShape2D component
    // If CollisionShape2D is present, it will control the size
    if (!GetOwner() || !GetOwner()->GetComponent<CollisionShape2D>()) {
        AddExportVariable("size", m_size, "Shape size", ExportVariableType::Vec2);
    }

    AddExportVariable("collision_layer", m_collision_layer, "Collision layer", ExportVariableType::Int);
    AddExportVariable("collision_mask", m_collision_mask, "Collision mask", ExportVariableType::Int);
}

void KinematicBody2D::UpdateExportVariables() {
    SetExportVariable("shape_type", ShapeTypeToInt(m_shape_type));

    // Only update size if there's no CollisionShape2D component
    if (!GetOwner() || !GetOwner()->GetComponent<CollisionShape2D>()) {
        SetExportVariable("size", m_size);
    }

    SetExportVariable("collision_layer", m_collision_layer);
    SetExportVariable("collision_mask", m_collision_mask);
}

void KinematicBody2D::UpdateFromExportVariables() {
    // Get values from export variables and update internal state
    auto shape_type_var = GetExportVariables().find("shape_type");
    if (shape_type_var != GetExportVariables().end()) {
        int shape_type_int = std::get<int>(shape_type_var->second.value);
        CollisionShapeType new_shape_type = IntToShapeType(shape_type_int);
        if (new_shape_type != m_shape_type) {
            SetShapeType(new_shape_type);
        }
    }

    auto size_var = GetExportVariables().find("size");
    if (size_var != GetExportVariables().end()) {
        glm::vec2 new_size = std::get<glm::vec2>(size_var->second.value);
        if (new_size != m_size) {
            SetSize(new_size);
        }
    }

    auto layer_var = GetExportVariables().find("collision_layer");
    if (layer_var != GetExportVariables().end()) {
        int new_layer = std::get<int>(layer_var->second.value);
        if (new_layer != m_collision_layer) {
            SetCollisionLayer(new_layer);
        }
    }

    auto mask_var = GetExportVariables().find("collision_mask");
    if (mask_var != GetExportVariables().end()) {
        int new_mask = std::get<int>(mask_var->second.value);
        if (new_mask != m_collision_mask) {
            SetCollisionMask(new_mask);
        }
    }
}

void KinematicBody2D::RecreatePhysicsBody() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody2D(m_physics_body);
        m_physics_body = nullptr;
    }

    auto* node2d = dynamic_cast<Node2D*>(GetOwner());
    if (!node2d) {
        std::cerr << "KinematicBody2D can only be attached to Node2D!" << std::endl;
        return;
    }

    // Check if there's a CollisionShape2D component and use its data
    CollisionShapeType shape_type = m_shape_type;
    glm::vec2 shape_size = m_size;
    glm::vec2 shape_offset(0.0f, 0.0f);

    if (auto* collision_shape = GetOwner()->GetComponent<CollisionShape2D>()) {
        shape_type = collision_shape->GetShapeType();
        shape_size = collision_shape->GetSize();
        shape_offset = collision_shape->GetOffset();
        std::cout << "KinematicBody2D using CollisionShape2D: type=" << (int)shape_type
                  << " size=(" << shape_size.x << "," << shape_size.y << ")"
                  << " offset=(" << shape_offset.x << "," << shape_offset.y << ")" << std::endl;
    }

    // Update our internal size to match what we're actually using
    m_shape_type = shape_type;
    m_size = shape_size;

    // Create kinematic physics body
    m_physics_body = PhysicsManager::CreatePhysicsBody2D(node2d, PhysicsBodyType::Kinematic, shape_type, shape_size, m_material, shape_offset);

    if (m_physics_body) {
        // Set collision layer and mask
        m_physics_body->SetCollisionLayer(m_collision_layer);
        m_physics_body->SetCollisionMask(m_collision_mask);

        // Set up collision callback for kinematic body collision detection
        m_physics_body->SetCollisionCallback([this](Node* self, Node* other, const glm::vec3& contact_point, const glm::vec3& normal) {
            (void)self;
            (void)contact_point;

            if (auto* other_node2d = dynamic_cast<Node2D*>(other)) {
                OnCollision(other_node2d, true);

                // Store collision normal for movement calculations
                m_last_collision_normal = glm::vec2(normal.x, normal.y);

                // Determine collision type based on normal direction
                if (normal.y > 0.7f) {
                    m_on_floor = true;
                } else if (normal.y < -0.7f) {
                    m_on_ceiling = true;
                } else {
                    m_on_wall = true;
                }
            }
        });

        // Set collision callback if provided
        if (m_collision_callback) {
            m_physics_body->SetCollisionCallback(m_collision_callback);
        }
    }
}

void KinematicBody2D::OnCollision(Node2D* other_node, bool entered) {
    if (!other_node) {
        return;
    }

    // Handle collision events - can be extended by derived classes or scripts
    if (entered) {
        // Collision started
        std::cout << "KinematicBody2D collision with: " << other_node->GetName() << std::endl;
    } else {
        // Collision ended
        std::cout << "KinematicBody2D collision ended with: " << other_node->GetName() << std::endl;
    }
}

int KinematicBody2D::ShapeTypeToInt(CollisionShapeType type) const {
    switch (type) {
        case CollisionShapeType::Box: return 0;
        case CollisionShapeType::Circle: return 1;
        case CollisionShapeType::Capsule: return 2;
        default: return 0;
    }
}

CollisionShapeType KinematicBody2D::IntToShapeType(int value) const {
    switch (value) {
        case 0: return CollisionShapeType::Box;
        case 1: return CollisionShapeType::Circle;
        case 2: return CollisionShapeType::Capsule;
        default: return CollisionShapeType::Box;
    }
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(KinematicBody2D, "Physics", "2D kinematic body for programmatic movement")
