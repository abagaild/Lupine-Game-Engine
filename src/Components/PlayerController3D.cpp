#include "lupine/components/PlayerController3D.h"
#include "lupine/components/KinematicBody3D.h"
#include "lupine/components/Camera3D.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/core/Scene.h"
#include "lupine/input/InputManager.h"
#include "lupine/physics/PhysicsManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

namespace Lupine {

PlayerController3D::PlayerController3D()
    : Component("PlayerController3D")
    , m_speed(5.0f)
    , m_jump_force(10.0f)
    , m_gravity(20.0f)
    , m_normalize_diagonal(true)
    , m_move_forward_action("move_forward")
    , m_move_backward_action("move_backward")
    , m_move_left_action("move_left")
    , m_move_right_action("move_right")
    , m_jump_action("jump")
    , m_velocity(0.0f, 0.0f, 0.0f)
    , m_movement_direction(0.0f, 0.0f, 0.0f)
    , m_input_vector(0.0f, 0.0f, 0.0f)
    , m_is_on_ground(false)
    , m_ground_y(0.0f)
    , m_camera_relative_movement(false)
    , m_camera_node_reference("") {
    
    InitializeExportVariables();
}

void PlayerController3D::InitializeExportVariables() {
    // Movement settings
    AddExportVariable("speed", m_speed, "Movement speed in units per second", ExportVariableType::Float);
    AddExportVariable("jump_force", m_jump_force, "Jump force in units per second", ExportVariableType::Float);
    AddExportVariable("gravity", m_gravity, "Gravity strength in units per second squared", ExportVariableType::Float);
    AddExportVariable("normalize_diagonal", m_normalize_diagonal, "Normalize diagonal movement to prevent faster diagonal movement", ExportVariableType::Bool);
    
    // Action mappings
    AddExportVariable("move_forward_action", m_move_forward_action, "Action name for forward movement (default: move_forward)", ExportVariableType::String);
    AddExportVariable("move_backward_action", m_move_backward_action, "Action name for backward movement (default: move_backward)", ExportVariableType::String);
    AddExportVariable("move_left_action", m_move_left_action, "Action name for left movement (default: move_left)", ExportVariableType::String);
    AddExportVariable("move_right_action", m_move_right_action, "Action name for right movement (default: move_right)", ExportVariableType::String);
    AddExportVariable("jump_action", m_jump_action, "Action name for jumping (default: jump)", ExportVariableType::String);

    // Camera-relative movement
    AddExportVariable("camera_relative_movement", m_camera_relative_movement, "Enable camera-relative movement (forward = away from camera)", ExportVariableType::Bool);
    AddExportVariable("camera_node_reference", m_camera_node_reference, "UUID of camera node for relative movement", ExportVariableType::NodeReference);
}

void PlayerController3D::OnReady() {
    // Update values from export variables
    m_speed = GetExportVariableValue<float>("speed", m_speed);
    m_jump_force = GetExportVariableValue<float>("jump_force", m_jump_force);
    m_gravity = GetExportVariableValue<float>("gravity", m_gravity);
    m_normalize_diagonal = GetExportVariableValue<bool>("normalize_diagonal", m_normalize_diagonal);
    
    m_move_forward_action = GetExportVariableValue<std::string>("move_forward_action", m_move_forward_action);
    m_move_backward_action = GetExportVariableValue<std::string>("move_backward_action", m_move_backward_action);
    m_move_left_action = GetExportVariableValue<std::string>("move_left_action", m_move_left_action);
    m_move_right_action = GetExportVariableValue<std::string>("move_right_action", m_move_right_action);
    m_jump_action = GetExportVariableValue<std::string>("jump_action", m_jump_action);

    // Camera-relative movement
    m_camera_relative_movement = GetExportVariableValue<bool>("camera_relative_movement", m_camera_relative_movement);

    // Handle camera_node_reference as UUID (NodeReference type)
    UUID camera_node_uuid = GetExportVariableValue<UUID>("camera_node_reference", UUID::Nil());
    m_camera_node_reference = camera_node_uuid.IsNil() ? "" : camera_node_uuid.ToString();

    // Initialize ground position
    if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
        m_ground_y = node3d->GetPosition().y;
        m_is_on_ground = true;
    }
}

void PlayerController3D::OnUpdate(float delta_time) {
    if (!IsActive()) return;
    
    // Update input vector based on current action states
    UpdateInputVector();
    
    // Update movement based on input
    UpdateMovement(delta_time);
    
    // Apply movement to the owner node
    ApplyMovement(delta_time);
}

void PlayerController3D::OnInput(const void* event) {
    // Input is handled through action system in OnUpdate
    (void)event;
}

void PlayerController3D::SetCameraRelativeMovement(bool enabled) {
    m_camera_relative_movement = enabled;
    SetExportVariable("camera_relative_movement", enabled);
}

void PlayerController3D::SetCameraNodeReference(const std::string& camera_uuid) {
    m_camera_node_reference = camera_uuid;
    SetExportVariable("camera_node_reference", camera_uuid);
}

void PlayerController3D::UpdateInputVector() {
    m_input_vector = glm::vec3(0.0f, 0.0f, 0.0f);
    
    // Check each movement action
    if (InputManager::IsActionPressed(m_move_forward_action)) {
        m_input_vector.z += 1.0f; // Negative Z is forward
    }
    if (InputManager::IsActionPressed(m_move_backward_action)) {
        m_input_vector.z -= 1.0f; // Positive Z is backward
    }
    if (InputManager::IsActionPressed(m_move_left_action)) {
        m_input_vector.x -= 1.0f; // Negative X is left
    }
    if (InputManager::IsActionPressed(m_move_right_action)) {
        m_input_vector.x += 1.0f; // Positive X is right
    }
    
    // Normalize diagonal movement if enabled (only for horizontal movement)
    glm::vec2 horizontal_input(m_input_vector.x, m_input_vector.z);
    if (m_normalize_diagonal && glm::length(horizontal_input) > 1.0f) {
        horizontal_input = glm::normalize(horizontal_input);
        m_input_vector.x = horizontal_input.x;
        m_input_vector.z = horizontal_input.y;
    }
}

void PlayerController3D::UpdateMovement(float delta_time) {
    // Calculate movement direction based on camera-relative setting
    glm::vec3 movement_velocity(0.0f);

    if (m_camera_relative_movement) {
        // Use camera-relative movement
        glm::vec3 camera_forward, camera_right;
        if (GetCameraDirections(camera_forward, camera_right)) {
            // Project camera directions onto horizontal plane (ignore Y component)
            camera_forward.y = 0.0f;
            camera_right.y = 0.0f;

            if (glm::length(camera_forward) > 0.001f) {
                camera_forward = glm::normalize(camera_forward);
            }
            if (glm::length(camera_right) > 0.001f) {
                camera_right = glm::normalize(camera_right);
            }

            // Calculate movement based on camera directions
            movement_velocity = (camera_forward * -m_input_vector.z) + (camera_right * m_input_vector.x);
            movement_velocity *= m_speed;

            // Debug camera-relative movement
            static int cam_debug_counter = 0;
            cam_debug_counter++;
            if (cam_debug_counter % 120 == 0 && glm::length(m_input_vector) > 0.001f) {
                std::cout << "PlayerController3D: Camera-relative movement - Input: ("
                          << m_input_vector.x << ", " << m_input_vector.z << "), "
                          << "Camera Forward: (" << camera_forward.x << ", " << camera_forward.z << "), "
                          << "Camera Right: (" << camera_right.x << ", " << camera_right.z << "), "
                          << "Result: (" << movement_velocity.x << ", " << movement_velocity.z << ")" << std::endl;
            }
        } else {
            // Fallback to world-space movement if camera not found
            std::cout << "PlayerController3D: Camera not found, using world-space movement" << std::endl;
            movement_velocity.x = m_input_vector.x * m_speed;
            movement_velocity.z = m_input_vector.z * m_speed;
        }
    } else {
        // Use world-space movement (original behavior)
        movement_velocity.x = m_input_vector.x * m_speed;
        movement_velocity.z = m_input_vector.z * m_speed;
    }

    // Update horizontal velocity
    m_velocity.x = movement_velocity.x;
    m_velocity.z = movement_velocity.z;

    // Update movement direction for other systems
    glm::vec2 horizontal_velocity(m_velocity.x, m_velocity.z);
    if (glm::length(horizontal_velocity) > 0.001f) {
        glm::vec2 normalized_velocity = glm::normalize(horizontal_velocity);
        m_movement_direction.x = normalized_velocity.x;
        m_movement_direction.z = normalized_velocity.y;
    } else {
        m_movement_direction.x = 0.0f;
        m_movement_direction.z = 0.0f;
    }
    
    // Handle jumping
    if (InputManager::IsActionJustPressed(m_jump_action)) {
        if (m_is_on_ground) {
            std::cout << "PlayerController3D: Jump! Force = " << m_jump_force << std::endl;
            m_velocity.y = m_jump_force;
            m_is_on_ground = false;
        } else {
            std::cout << "PlayerController3D: Jump attempted but not on ground" << std::endl;
        }
    }

    // Apply gravity
    if (!m_is_on_ground) {
        m_velocity.y -= m_gravity * delta_time;
    }

    // Debug output every few frames
    static int debug_counter = 0;
    debug_counter++;
    if (debug_counter % 60 == 0) {
        std::cout << "PlayerController3D: On ground = " << m_is_on_ground
                  << ", Velocity = (" << m_velocity.x << ", " << m_velocity.y << ", " << m_velocity.z << ")" << std::endl;
    }
}

void PlayerController3D::ApplyMovement(float delta_time) {
    // Try to find a KinematicBody3D component on the same node
    KinematicBody3D* kinematic_body = GetOwner()->GetComponent<KinematicBody3D>();

    if (kinematic_body) {
        // Use kinematic body for physics-based movement
        // Always call MoveAndSlide, even with small velocities (for gravity)
        std::cout << "PlayerController3D: Using KinematicBody3D for movement with velocity ("
                  << m_velocity.x << ", " << m_velocity.y << ", " << m_velocity.z << ")" << std::endl;
        kinematic_body->MoveAndSlide(m_velocity, delta_time);

        // Update ground state from kinematic body
        m_is_on_ground = kinematic_body->IsOnFloor();
    } else {
        // Fallback to direct node movement if no kinematic body is present
        Node3D* node3d = dynamic_cast<Node3D*>(GetOwner());
        if (!node3d) {
            std::cout << "PlayerController3D: Owner is not Node3D" << std::endl;
            return; // This component only works with Node3D
        }

        // Apply movement directly to node
        if (glm::length(m_velocity) > 0.001f) {
            glm::vec3 currentPosition = node3d->GetPosition();
            glm::vec3 newPosition = currentPosition + (m_velocity * delta_time);

            // Check ground collision
            if (CheckGroundCollision(newPosition)) {
                newPosition.y = m_ground_y;
                m_velocity.y = 0.0f;
                m_is_on_ground = true;
            }

            node3d->SetPosition(newPosition);
        }
    }
}

bool PlayerController3D::CheckGroundCollision(const glm::vec3& position) {
    // Check if the player is colliding with something below it
    if (!GetOwner()) {
        return false;
    }

    // Perform a raycast downward from the player position to check for ground
    glm::vec3 ray_start = position;
    glm::vec3 ray_end = position - glm::vec3(0.0f, 0.2f, 0.0f); // Cast 0.2 units down

    // Use physics manager's raycast functionality
    Node3D* hit_node = nullptr;
    glm::vec3 hit_point, hit_normal;

    // Check if we hit any static bodies (ground, platforms, etc.)
    bool hit = PhysicsManager::Raycast3D(ray_start, ray_end, hit_node, hit_point, hit_normal);

    if (hit && hit_node) {
        // Check if the hit surface is walkable (normal pointing mostly upward)
        float dot_product = glm::dot(hit_normal, glm::vec3(0.0f, 1.0f, 0.0f));
        return dot_product > 0.7f; // Surface is walkable if angle is less than ~45 degrees
    }

    // Fallback to simple ground plane check if no physics hit
    return position.y <= m_ground_y + 0.05f; // Small tolerance for ground detection
}

bool PlayerController3D::GetCameraDirections(glm::vec3& forward, glm::vec3& right) const {
    if (m_camera_node_reference.empty() || !GetOwner() || !GetOwner()->GetScene()) {
        return false;
    }

    // Find camera node by UUID
    Node* camera_node = GetOwner()->GetScene()->FindNodeByUUID(m_camera_node_reference);
    if (!camera_node) {
        return false;
    }

    // For player movement, we need to use the camera's orbit angles, not the actual look-at direction
    // This allows the camera to look at the player while the player moves relative to the camera's orbit position
    if (auto* camera3d = camera_node->GetComponent<Camera3D>()) {
        // Get the orbit angles from the Camera3D component
        float orbit_yaw = camera3d->GetOrbitYaw();
        float orbit_pitch = camera3d->GetOrbitPitch();

        // Calculate movement directions based on orbit angles (ignoring pitch for horizontal movement)
        float yaw_rad = glm::radians(orbit_yaw);

        // Calculate forward and right vectors for movement (on horizontal plane)
        // Forward should be the direction the camera is "looking" horizontally
        forward = glm::vec3(sin(yaw_rad), 0.0f, cos(yaw_rad));   // Forward relative to camera orbit
        right = glm::vec3(cos(yaw_rad), 0.0f, -sin(yaw_rad));    // Right relative to camera orbit

        // Debug camera directions
        static int dir_debug_counter = 0;
        dir_debug_counter++;
        if (dir_debug_counter % 120 == 0) {
            std::cout << "PlayerController3D: Orbit Yaw: " << orbit_yaw << ", Forward: (" << forward.x << ", " << forward.y << ", " << forward.z << "), "
                      << "Right: (" << right.x << ", " << right.y << ", " << right.z << ")" << std::endl;
        }

        return true;
    }

    // Fallback: Get camera rotation from Node3D if Camera3D component not found
    if (auto* camera_node3d = dynamic_cast<Node3D*>(camera_node)) {
        glm::quat camera_rotation = camera_node3d->GetGlobalRotation();

        // Calculate forward and right vectors from camera rotation
        forward = camera_rotation * glm::vec3(0.0f, 0.0f, -1.0f); // Camera looks down negative Z
        right = camera_rotation * glm::vec3(1.0f, 0.0f, 0.0f);    // Camera right is positive X

        return true;
    }

    return false;
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(PlayerController3D, "Player", "Basic 3D player controller with WASD movement, jump, and gravity")
