#include "lupine/components/KinematicBody3D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/physics/PhysicsManager.h"
#include "lupine/components/CollisionMesh3D.h"
#include "lupine/components/RigidBody3D.h"
#include <iostream>
#include <limits>
#include <algorithm>

namespace Lupine {

KinematicBody3D::KinematicBody3D()
    : Component("KinematicBody3D")
    , m_shape_type(CollisionShapeType::Box)
    , m_size(1.0f, 1.0f, 1.0f)
    , m_collision_layer(0)
    , m_collision_mask(0xFFFF)
    , m_velocity(0.0f, 0.0f, 0.0f)
    , m_last_collision_normal(0.0f, 0.0f, 0.0f)
    , m_on_floor(false)
    , m_on_wall(false)
    , m_on_ceiling(false)
    , m_physics_body(nullptr)
    , m_needs_recreation(false) {

    InitializeExportVariables();
}

KinematicBody3D::~KinematicBody3D() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody3D(m_physics_body);
        m_physics_body = nullptr;
    }
}

void KinematicBody3D::SetShapeType(CollisionShapeType type) {
    if (m_shape_type != type) {
        m_shape_type = type;
        m_needs_recreation = true;
    }
}

void KinematicBody3D::SetSize(const glm::vec3& size) {
    if (m_size != size) {
        m_size = size;
        m_needs_recreation = true;
    }
}

bool KinematicBody3D::MoveAndSlide(const glm::vec3& velocity, float delta_time) {
    m_velocity = velocity;

    auto* node3d = dynamic_cast<Node3D*>(GetOwner());
    if (!node3d || !m_physics_body) {
        return false;
    }

    // Reset collision flags before movement
    m_on_floor = false;
    m_on_wall = false;
    m_on_ceiling = false;

    glm::vec3 movement = velocity * delta_time;
    glm::vec3 current_pos = node3d->GetPosition();
    glm::vec3 target_pos = current_pos + movement;

    // If there's no movement, just return
    if (glm::length(movement) < 0.001f) {
        return false;
    }



    // Use proper convex sweep test for collision detection
    PhysicsBody3D* hit_body = nullptr;
    glm::vec3 hit_point, hit_normal(0.0f, 1.0f, 0.0f);
    float hit_fraction = 1.0f;
    bool collision_detected = false;

    // Use swept AABB test for collision detection (convex sweep disabled temporarily)
    collision_detected = PerformSweptAABBTest(current_pos, target_pos, movement,
                                             hit_body, hit_point, hit_normal, hit_fraction);

    if (collision_detected && hit_body) {
        // Calculate the safe position just before collision with a proper margin
        const float collision_margin = 0.001f; // Small margin to prevent overlap
        float movement_length = glm::length(movement);

        // Calculate margin as a fraction of the movement
        float margin_fraction = (movement_length > 0.0f) ? collision_margin / movement_length : 0.0f;
        float safe_fraction = std::max(0.0f, hit_fraction - margin_fraction);

        // Ensure we don't move if collision is at the very start
        if (hit_fraction <= 0.001f) {
            safe_fraction = 0.0f;
        }

        glm::vec3 safe_movement = movement * safe_fraction;
        glm::vec3 safe_pos = current_pos + safe_movement;



        // Determine collision type based on normal direction
        if (hit_normal.y > 0.7f) {
            m_on_floor = true;
        } else if (hit_normal.y < -0.7f) {
            m_on_ceiling = true;
        } else {
            m_on_wall = true;
        }

        // Store collision normal for potential sliding
        m_last_collision_normal = hit_normal;

        // Calculate sliding movement along the surface
        glm::vec3 remaining_movement = movement - safe_movement;
        glm::vec3 slide_direction = remaining_movement - glm::dot(remaining_movement, hit_normal) * hit_normal;

        // Apply the safe movement
        if (m_physics_body) {
            m_physics_body->SetPosition(safe_pos);
        }
        node3d->SetPosition(safe_pos);

        // Try to slide along the surface if there's remaining movement
        if (glm::length(slide_direction) > 0.001f) {
            glm::vec3 slide_target = safe_pos + slide_direction;

            // Simple AABB test for slide collision
            glm::vec3 slide_min = slide_target - m_size * 0.5f;
            glm::vec3 slide_max = slide_target + m_size * 0.5f;
            bool slide_collision = false;

            // Check against all static bodies
            auto& all_bodies = PhysicsManager::GetAllBodies3D();
            for (auto& body : all_bodies) {
                if (body.get() == m_physics_body) continue;

                Node3D* other_node = body->GetOwner();
                if (!other_node) continue;

                RigidBody3D* rigid_body = other_node->GetComponent<RigidBody3D>();
                if (!rigid_body || rigid_body->GetBodyType() != PhysicsBodyType::Static) continue;

                glm::vec3 other_pos = other_node->GetPosition();
                glm::vec3 other_size = rigid_body->GetCollisionShapeSize();
                glm::vec3 other_min = other_pos - other_size * 0.5f;
                glm::vec3 other_max = other_pos + other_size * 0.5f;

                if (slide_max.x >= other_min.x && slide_min.x <= other_max.x &&
                    slide_max.y >= other_min.y && slide_min.y <= other_max.y &&
                    slide_max.z >= other_min.z && slide_min.z <= other_max.z) {
                    slide_collision = true;
                    break;
                }
            }

            if (!slide_collision) {
                // Safe to slide
                if (m_physics_body) {
                    m_physics_body->SetPosition(slide_target);
                }
                node3d->SetPosition(slide_target);
            } else {
                // Can't slide, stay at safe position
                if (m_physics_body) {
                    m_physics_body->SetPosition(safe_pos);
                }
                node3d->SetPosition(safe_pos);
            }
        }

        return true;
    } else {
        // No collision, move freely
        m_physics_body->SetPosition(target_pos);
        node3d->SetPosition(target_pos);
        return false;
    }
}

bool KinematicBody3D::MoveAndCollide(const glm::vec3& velocity, float delta_time) {
    m_velocity = velocity;

    auto* node3d = dynamic_cast<Node3D*>(GetOwner());
    if (!node3d || !m_physics_body) {
        return false;
    }

    // Reset collision flags before movement
    m_on_floor = false;
    m_on_wall = false;
    m_on_ceiling = false;

    glm::vec3 movement = velocity * delta_time;
    glm::vec3 current_pos = node3d->GetPosition();
    glm::vec3 target_pos = current_pos + movement;

    // If there's no movement, just return
    if (glm::length(movement) < 0.001f) {
        return false;
    }

    // Use proper convex sweep test for collision detection
    PhysicsBody3D* hit_body = nullptr;
    glm::vec3 hit_point, hit_normal(0.0f, 1.0f, 0.0f);
    float hit_fraction = 1.0f;
    bool collision_detected = false;

    // Use swept AABB test for collision detection (convex sweep disabled temporarily)
    collision_detected = PerformSweptAABBTest(current_pos, target_pos, movement,
                                             hit_body, hit_point, hit_normal, hit_fraction);

    if (collision_detected && hit_body) {
        // Calculate the position just before collision with a proper margin
        const float collision_margin = 0.001f; // Small margin to prevent overlap
        float movement_length = glm::length(movement);

        // Calculate margin as a fraction of the movement
        float margin_fraction = (movement_length > 0.0f) ? collision_margin / movement_length : 0.0f;
        float safe_fraction = std::max(0.0f, hit_fraction - margin_fraction);

        // Ensure we don't move if collision is at the very start
        if (hit_fraction <= 0.001f) {
            safe_fraction = 0.0f;
        }

        glm::vec3 safe_movement = movement * safe_fraction;
        glm::vec3 collision_pos = current_pos + safe_movement;

        // Determine collision type based on normal direction
        if (hit_normal.y > 0.7f) {
            m_on_floor = true;
        } else if (hit_normal.y < -0.7f) {
            m_on_ceiling = true;
        } else {
            m_on_wall = true;
        }

        // Store collision normal
        m_last_collision_normal = hit_normal;

        // Move to the collision position
        if (m_physics_body) {
            m_physics_body->SetPosition(collision_pos);
        }
        node3d->SetPosition(collision_pos);

        // Trigger collision callback if set
        if (m_collision_callback && hit_body->GetOwner()) {
            m_collision_callback(GetOwner(), hit_body->GetOwner(), hit_point, hit_normal);
        }

        return true;
    } else {
        // No collision, move freely
        if (m_physics_body) {
            m_physics_body->SetPosition(target_pos);
        }
        node3d->SetPosition(target_pos);
        return false;
    }
}

void KinematicBody3D::SetVelocity(const glm::vec3& velocity) {
    m_velocity = velocity;
}

void KinematicBody3D::OnReady() {
    // Load export variables first to get correct values from scene file
    UpdateFromExportVariables();
    RecreatePhysicsBody();
}

void KinematicBody3D::OnUpdate(float delta_time) {
    (void)delta_time;

    if (m_needs_recreation) {
        RecreatePhysicsBody();
        m_needs_recreation = false;
    }

    UpdateFromExportVariables();
}

void KinematicBody3D::OnDestroy() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody3D(m_physics_body);
        m_physics_body = nullptr;
    }
}

void KinematicBody3D::InitializeExportVariables() {
    // Shape type
    std::vector<std::string> shapeTypeOptions = {
        "Box", "Sphere", "Capsule", "Cylinder", "Mesh"
    };
    AddEnumExportVariable("shape_type", ShapeTypeToInt(m_shape_type), "Collision shape type", shapeTypeOptions);

    AddExportVariable("size", m_size, "Shape size", ExportVariableType::Vec3);
    AddExportVariable("collision_layer", m_collision_layer, "Collision layer", ExportVariableType::Int);
    AddExportVariable("collision_mask", m_collision_mask, "Collision mask", ExportVariableType::Int);
}

void KinematicBody3D::UpdateFromExportVariables() {
    bool needs_update = false;

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
    auto shape_size_var = GetExportVariable("size");
    if (shape_size_var && std::holds_alternative<glm::vec3>(*shape_size_var)) {
        glm::vec3 new_shape_size = std::get<glm::vec3>(*shape_size_var);
        if (new_shape_size != m_size) {
            m_size = new_shape_size;
            needs_update = true;
        }
    }

    // Update collision layer from export variable
    auto collision_layer_var = GetExportVariable("collision_layer");
    if (collision_layer_var && std::holds_alternative<int>(*collision_layer_var)) {
        int new_collision_layer = std::get<int>(*collision_layer_var);
        if (new_collision_layer != m_collision_layer) {
            m_collision_layer = new_collision_layer;
            needs_update = true;
        }
    }

    // Update collision mask from export variable
    auto collision_mask_var = GetExportVariable("collision_mask");
    if (collision_mask_var && std::holds_alternative<int>(*collision_mask_var)) {
        int new_collision_mask = std::get<int>(*collision_mask_var);
        if (new_collision_mask != m_collision_mask) {
            m_collision_mask = new_collision_mask;
            needs_update = true;
        }
    }

    // Only set needs_recreation if we actually have changes and a physics body exists
    if (needs_update && m_physics_body) {
        m_needs_recreation = true;
    }
}

void KinematicBody3D::RecreatePhysicsBody() {
    if (m_physics_body) {
        PhysicsManager::RemovePhysicsBody3D(m_physics_body);
        m_physics_body = nullptr;
    }

    auto* node3d = dynamic_cast<Node3D*>(GetOwner());
    if (!node3d) {
        std::cerr << "KinematicBody3D can only be attached to Node3D!" << std::endl;
        return;
    }

    // Check if there's a CollisionMesh3D component for mesh-based collision
    CollisionMesh3D* collision_mesh = node3d->GetComponent<CollisionMesh3D>();

    std::cout << "Creating KinematicBody3D for node: " << node3d->GetName()
              << " (body_type: Kinematic)" << std::endl;

    if (collision_mesh && (m_shape_type == CollisionShapeType::Mesh ||
                          !collision_mesh->GetMeshPath().empty())) {
        // Use mesh-based collision
        std::cout << "  Using mesh-based collision" << std::endl;
        m_physics_body = PhysicsManager::CreatePhysicsBody3D(node3d, PhysicsBodyType::Kinematic, collision_mesh, m_material);
    } else {
        // Use primitive collision shape
        std::cout << "  Using primitive collision shape (type: " << static_cast<int>(m_shape_type)
                  << ", size: " << m_size.x << "x" << m_size.y << "x" << m_size.z << ")" << std::endl;
        m_physics_body = PhysicsManager::CreatePhysicsBody3D(node3d, PhysicsBodyType::Kinematic, m_shape_type, m_size, m_material);
    }

    if (m_physics_body) {
        std::cout << "  KinematicBody3D physics body created successfully!" << std::endl;

        // Set up collision callback for kinematic body collision detection
        // Note: This callback is mainly for debugging/events, actual collision detection
        // is handled by the convex sweep tests in MoveAndSlide/MoveAndCollide
        m_physics_body->SetCollisionCallback([this](Node* self, Node* other, const glm::vec3& contact_point, const glm::vec3& normal) {
            (void)self;
            (void)contact_point;

            if (auto* other_node3d = dynamic_cast<Node3D*>(other)) {
                OnCollision(other_node3d, true);

                // Store collision normal for movement calculations
                m_last_collision_normal = normal;

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
    } else {
        std::cout << "  ERROR: Failed to create KinematicBody3D physics body!" << std::endl;
    }
}

void KinematicBody3D::SetCollisionCallback(CollisionCallback callback) {
    m_collision_callback = callback;

    // If physics body already exists, update its callback
    if (m_physics_body) {
        m_physics_body->SetCollisionCallback(callback);
    }
}

void KinematicBody3D::OnCollision(Node3D* other_node, bool entered) {
    (void)other_node;
    (void)entered;
    // This method can be overridden by derived classes for custom collision handling
}

int KinematicBody3D::ShapeTypeToInt(CollisionShapeType type) const {
    return static_cast<int>(type);
}

CollisionShapeType KinematicBody3D::IntToShapeType(int value) const {
    return static_cast<CollisionShapeType>(value);
}

bool KinematicBody3D::PerformSweptAABBTest(const glm::vec3& start_pos, const glm::vec3& end_pos, const glm::vec3& movement,
                                          PhysicsBody3D*& hit_body, glm::vec3& hit_point, glm::vec3& hit_normal, float& hit_fraction) {
    hit_body = nullptr;
    hit_fraction = 1.0f;

    // Get all physics bodies to test against
    auto& all_bodies = PhysicsManager::GetAllBodies3D();



    float closest_fraction = 1.0f;
    PhysicsBody3D* closest_body = nullptr;
    glm::vec3 closest_normal(0.0f, 1.0f, 0.0f);
    glm::vec3 closest_point = end_pos;

    for (auto& body : all_bodies) {
        if (!body || body.get() == m_physics_body) continue; // Skip ourselves and null bodies

        Node3D* other_node = body->GetOwner();
        if (!other_node) continue;

        // Check against static and dynamic bodies (but not kinematic bodies to avoid conflicts)
        RigidBody3D* rigid_body = other_node->GetComponent<RigidBody3D>();
        if (!rigid_body) continue;

        PhysicsBodyType body_type = rigid_body->GetBodyType();
        if (body_type != PhysicsBodyType::Static && body_type != PhysicsBodyType::Dynamic) continue;

        // Get other body's AABB
        glm::vec3 other_pos = other_node->GetPosition();
        glm::vec3 other_size = rigid_body->GetCollisionShapeSize();


        glm::vec3 other_min = other_pos - other_size * 0.5f;
        glm::vec3 other_max = other_pos + other_size * 0.5f;

        // Perform swept AABB test
        float collision_time = PerformSweptAABBCollision(start_pos, m_size, other_min, other_max, movement);









        if (collision_time >= 0.0f && collision_time < closest_fraction) {

            closest_fraction = collision_time;
            closest_body = body.get();

            // Calculate collision point
            glm::vec3 collision_pos = start_pos + movement * collision_time;
            closest_point = collision_pos;

            // Calculate collision normal based on which face of the static body was hit
            // We need to determine which face of the static AABB the kinematic body collided with
            glm::vec3 kinematic_center = collision_pos;
            glm::vec3 static_center = other_pos;

            // Calculate the penetration on each axis
            glm::vec3 kinematic_min = kinematic_center - m_size * 0.5f;
            glm::vec3 kinematic_max = kinematic_center + m_size * 0.5f;

            // Calculate overlap on each axis
            float overlap_x = std::min(kinematic_max.x - other_min.x, other_max.x - kinematic_min.x);
            float overlap_y = std::min(kinematic_max.y - other_min.y, other_max.y - kinematic_min.y);
            float overlap_z = std::min(kinematic_max.z - other_min.z, other_max.z - kinematic_min.z);

            // The collision normal should point along the axis with the smallest overlap
            if (overlap_y <= overlap_x && overlap_y <= overlap_z) {
                // Y-axis collision (floor/ceiling)
                closest_normal = (kinematic_center.y > static_center.y) ? glm::vec3(0, 1, 0) : glm::vec3(0, -1, 0);
            } else if (overlap_x <= overlap_z) {
                // X-axis collision (wall)
                closest_normal = (kinematic_center.x > static_center.x) ? glm::vec3(1, 0, 0) : glm::vec3(-1, 0, 0);
            } else {
                // Z-axis collision (wall)
                closest_normal = (kinematic_center.z > static_center.z) ? glm::vec3(0, 0, 1) : glm::vec3(0, 0, -1);
            }
        }
    }

    if (closest_body) {
        hit_body = closest_body;
        hit_fraction = closest_fraction;
        hit_normal = closest_normal;
        hit_point = closest_point;
        return true;
    }

    return false;
}

float KinematicBody3D::PerformSweptAABBCollision(const glm::vec3& moving_pos, const glm::vec3& moving_size,
                                                const glm::vec3& static_min, const glm::vec3& static_max, const glm::vec3& movement) {
    // Calculate moving AABB bounds
    glm::vec3 moving_min = moving_pos - moving_size * 0.5f;
    glm::vec3 moving_max = moving_pos + moving_size * 0.5f;

    // If movement is zero, check for immediate overlap
    if (glm::length(movement) < 0.001f) {
        bool overlap = (moving_max.x >= static_min.x && moving_min.x <= static_max.x &&
                       moving_max.y >= static_min.y && moving_min.y <= static_max.y &&
                       moving_max.z >= static_min.z && moving_min.z <= static_max.z);
        return overlap ? 0.0f : -1.0f;
    }

    // Calculate entry and exit times for each axis
    glm::vec3 entry_time, exit_time;

    for (int i = 0; i < 3; i++) {
        if (std::abs(movement[i]) < 0.001f) {
            // No movement on this axis
            if (moving_max[i] < static_min[i] || moving_min[i] > static_max[i]) {
                return -1.0f; // No collision possible
            }
            entry_time[i] = -std::numeric_limits<float>::infinity();
            exit_time[i] = std::numeric_limits<float>::infinity();
        } else {
            // Calculate entry and exit times for this axis
            float t1 = (static_min[i] - moving_max[i]) / movement[i];
            float t2 = (static_max[i] - moving_min[i]) / movement[i];

            entry_time[i] = std::min(t1, t2);
            exit_time[i] = std::max(t1, t2);
        }
    }

    // Find the latest entry time and earliest exit time
    float latest_entry = std::max({entry_time.x, entry_time.y, entry_time.z});
    float earliest_exit = std::min({exit_time.x, exit_time.y, exit_time.z});

    // Check if collision occurs within the movement range [0, 1]
    if (latest_entry > earliest_exit || latest_entry < 0.0f || latest_entry > 1.0f) {
        return -1.0f; // No collision
    }

    return latest_entry;
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(KinematicBody3D, "Physics", "3D kinematic body for programmatic movement")
