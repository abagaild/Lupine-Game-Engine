#include "lupine/components/PlayerController25D.h"
#include "lupine/components/KinematicBody3D.h"
#include "lupine/components/AnimatedSprite3D.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/input/InputManager.h"
#include <algorithm>
#include <iostream>

namespace Lupine {

PlayerController25D::PlayerController25D()
    : Component("PlayerController2.5D")
    , m_speed(300.0f)
    , m_jump_force(500.0f)
    , m_gravity(980.0f)
    , m_sprint_multiplier(1.5f)
    , m_move_left_action("move_left")
    , m_move_right_action("move_right")
    , m_move_forward_action("move_forward")
    , m_move_backward_action("move_backward")
    , m_jump_action("jump")
    , m_sprint_action("sprint")
    , m_velocity(0.0f, 0.0f, 0.0f)
    , m_input_vector(0.0f, 0.0f, 0.0f)
    , m_is_on_ground(false)
    , m_idle_left_animation("idle_left")
    , m_idle_right_animation("idle_right")
    , m_walk_left_animation("walk_left")
    , m_walk_right_animation("walk_right")
    , m_sprint_left_animation("sprint_left")
    , m_sprint_right_animation("sprint_right")
    , m_jump_up_animation("jump_up")
    , m_jump_left_animation("jump_left")
    , m_jump_right_animation("jump_right")
    , m_jump_down_animation("jump_down") {
    
    InitializeExportVariables();
}

void PlayerController25D::InitializeExportVariables() {
    // Movement settings
    AddExportVariable("speed", m_speed, "Movement speed in units per second", ExportVariableType::Float);
    AddExportVariable("jump_force", m_jump_force, "Jump force in units per second", ExportVariableType::Float);
    AddExportVariable("gravity", m_gravity, "Gravity force in units per second squared", ExportVariableType::Float);
    AddExportVariable("sprint_multiplier", m_sprint_multiplier, "Sprint speed multiplier", ExportVariableType::Float);
    
    // Action mappings
    AddExportVariable("move_left_action", m_move_left_action, "Action name for moving left", ExportVariableType::String);
    AddExportVariable("move_right_action", m_move_right_action, "Action name for moving right", ExportVariableType::String);
    AddExportVariable("move_forward_action", m_move_forward_action, "Action name for moving forward", ExportVariableType::String);
    AddExportVariable("move_backward_action", m_move_backward_action, "Action name for moving backward", ExportVariableType::String);
    AddExportVariable("jump_action", m_jump_action, "Action name for jumping", ExportVariableType::String);
    AddExportVariable("sprint_action", m_sprint_action, "Action name for sprinting", ExportVariableType::String);
    
    // Animation names
    AddExportVariable("idle_left_animation", m_idle_left_animation, "Animation for idle left", ExportVariableType::String);
    AddExportVariable("idle_right_animation", m_idle_right_animation, "Animation for idle right", ExportVariableType::String);
    AddExportVariable("walk_left_animation", m_walk_left_animation, "Animation for walking left", ExportVariableType::String);
    AddExportVariable("walk_right_animation", m_walk_right_animation, "Animation for walking right", ExportVariableType::String);
    AddExportVariable("sprint_left_animation", m_sprint_left_animation, "Animation for sprinting left", ExportVariableType::String);
    AddExportVariable("sprint_right_animation", m_sprint_right_animation, "Animation for sprinting right", ExportVariableType::String);
    AddExportVariable("jump_up_animation", m_jump_up_animation, "Animation for jumping up", ExportVariableType::String);
    AddExportVariable("jump_left_animation", m_jump_left_animation, "Animation for jumping left", ExportVariableType::String);
    AddExportVariable("jump_right_animation", m_jump_right_animation, "Animation for jumping right", ExportVariableType::String);
    AddExportVariable("jump_down_animation", m_jump_down_animation, "Animation for jumping down", ExportVariableType::String);
}

void PlayerController25D::OnReady() {
    // Component is ready, no special initialization needed
}

void PlayerController25D::OnUpdate(float delta_time) {
    if (!IsActive()) return;

    // Update from export variables
    UpdateFromExportVariables();

    // Update input vector based on current action states
    UpdateInputVector();

    // Update movement based on input
    UpdateMovement(delta_time);

    // Apply movement to the owner node
    ApplyMovement(delta_time);

    // Update animation based on movement state
    UpdateAnimation();
}

void PlayerController25D::UpdateInputVector() {
    m_input_vector = glm::vec3(0.0f, 0.0f, 0.0f);
    
    // Check each movement action
    if (InputManager::IsActionPressed(m_move_left_action)) {
        m_input_vector.x -= 1.0f; // Negative X is left
    }
    if (InputManager::IsActionPressed(m_move_right_action)) {
        m_input_vector.x += 1.0f; // Positive X is right
    }
    if (InputManager::IsActionPressed(m_move_forward_action)) {
        m_input_vector.z -= 1.0f; // Negative Z is forward
    }
    if (InputManager::IsActionPressed(m_move_backward_action)) {
        m_input_vector.z += 1.0f; // Positive Z is backward
    }
    
    // Normalize horizontal movement
    if (glm::length(glm::vec2(m_input_vector.x, m_input_vector.z)) > 1.0f) {
        glm::vec2 horizontal = glm::normalize(glm::vec2(m_input_vector.x, m_input_vector.z));
        m_input_vector.x = horizontal.x;
        m_input_vector.z = horizontal.y;
    }
}

void PlayerController25D::UpdateMovement(float delta_time) {
    // Apply gravity
    if (!m_is_on_ground) {
        m_velocity.y -= m_gravity * delta_time;
    }
    
    // Handle jumping
    if (InputManager::IsActionJustPressed(m_jump_action) && m_is_on_ground) {
        m_velocity.y = m_jump_force;
        m_is_on_ground = false;
    }
    
    // Calculate horizontal movement
    float current_speed = m_speed;
    if (IsSprinting()) {
        current_speed *= m_sprint_multiplier;
    }
    
    // Apply horizontal movement
    m_velocity.x = m_input_vector.x * current_speed;
    m_velocity.z = m_input_vector.z * current_speed;
}

void PlayerController25D::ApplyMovement(float delta_time) {
    // Try to find a KinematicBody3D component on the same node
    KinematicBody3D* kinematic_body = GetOwner()->GetComponent<KinematicBody3D>();

    if (kinematic_body) {
        // Use kinematic body for physics-based movement
        kinematic_body->MoveAndSlide(m_velocity, delta_time);

        // Update ground state from kinematic body
        m_is_on_ground = kinematic_body->IsOnFloor();
    } else {
        // Fallback to direct node movement if no kinematic body is present
        Node3D* node3d = dynamic_cast<Node3D*>(GetOwner());
        if (!node3d) {
            std::cout << "PlayerController2.5D: Owner is not Node3D" << std::endl;
            return; // This component only works with Node3D
        }

        // Apply movement directly to node position
        glm::vec3 current_pos = node3d->GetPosition();
        glm::vec3 movement = m_velocity * delta_time;
        node3d->SetPosition(current_pos + movement);
        
        // Simple ground check (Y = 0)
        if (current_pos.y <= 0.0f && m_velocity.y <= 0.0f) {
            m_is_on_ground = true;
            m_velocity.y = 0.0f;
            node3d->SetPosition(glm::vec3(current_pos.x + movement.x, 0.0f, current_pos.z + movement.z));
        } else {
            m_is_on_ground = false;
        }
    }
}

bool PlayerController25D::IsSprinting() const {
    return InputManager::IsActionPressed(m_sprint_action);
}

void PlayerController25D::SetIdleAnimations(const std::string& left, const std::string& right) {
    m_idle_left_animation = left;
    m_idle_right_animation = right;
}

void PlayerController25D::SetWalkAnimations(const std::string& left, const std::string& right) {
    m_walk_left_animation = left;
    m_walk_right_animation = right;
}

void PlayerController25D::SetSprintAnimations(const std::string& left, const std::string& right) {
    m_sprint_left_animation = left;
    m_sprint_right_animation = right;
}

void PlayerController25D::SetJumpAnimations(const std::string& up, const std::string& left, 
                                           const std::string& right, const std::string& down) {
    m_jump_up_animation = up;
    m_jump_left_animation = left;
    m_jump_right_animation = right;
    m_jump_down_animation = down;
}

void PlayerController25D::UpdateAnimation() {
    // Find AnimatedSprite3D component on the same node
    AnimatedSprite3D* animated_sprite = GetOwner()->GetComponent<AnimatedSprite3D>();
    if (!animated_sprite) {
        return; // No animated sprite to control
    }
    
    std::string target_animation;
    
    if (!m_is_on_ground) {
        // In air - use jump animations based on velocity
        if (m_velocity.y > 0) {
            // Moving up
            if (std::abs(m_velocity.x) > 50.0f) {
                // Horizontal movement while jumping
                target_animation = (m_velocity.x > 0) ? m_jump_right_animation : m_jump_left_animation;
            } else {
                target_animation = m_jump_up_animation;
            }
        } else {
            // Falling down
            if (std::abs(m_velocity.x) > 50.0f) {
                // Horizontal movement while falling
                target_animation = (m_velocity.x > 0) ? m_jump_right_animation : m_jump_left_animation;
            } else {
                target_animation = m_jump_down_animation;
            }
        }
    } else if (IsMoving()) {
        // On ground and moving horizontally
        bool is_sprinting = IsSprinting();
        
        // Determine direction based on primary movement
        if (std::abs(m_velocity.x) > std::abs(m_velocity.z)) {
            // Horizontal movement is dominant
            if (is_sprinting) {
                target_animation = (m_velocity.x > 0) ? m_sprint_right_animation : m_sprint_left_animation;
            } else {
                target_animation = (m_velocity.x > 0) ? m_walk_right_animation : m_walk_left_animation;
            }
        } else {
            // Forward/backward movement - use right/left based on X component or default to right
            if (is_sprinting) {
                target_animation = (m_velocity.x >= 0) ? m_sprint_right_animation : m_sprint_left_animation;
            } else {
                target_animation = (m_velocity.x >= 0) ? m_walk_right_animation : m_walk_left_animation;
            }
        }
    } else {
        // On ground and idle - use last movement direction or default to right
        if (std::abs(m_input_vector.x) > 0.1f) {
            target_animation = (m_input_vector.x > 0) ? m_idle_right_animation : m_idle_left_animation;
        } else {
            // Default to idle right if no input
            target_animation = m_idle_right_animation;
        }
    }
    
    // Only change animation if it's different from current
    if (target_animation != m_current_animation && !target_animation.empty()) {
        m_current_animation = target_animation;
        animated_sprite->Play(target_animation);
    }
}

void PlayerController25D::UpdateFromExportVariables() {
    m_speed = GetExportVariableValue<float>("speed", m_speed);
    m_jump_force = GetExportVariableValue<float>("jump_force", m_jump_force);
    m_gravity = GetExportVariableValue<float>("gravity", m_gravity);
    m_sprint_multiplier = GetExportVariableValue<float>("sprint_multiplier", m_sprint_multiplier);
    m_move_left_action = GetExportVariableValue<std::string>("move_left_action", m_move_left_action);
    m_move_right_action = GetExportVariableValue<std::string>("move_right_action", m_move_right_action);
    m_move_forward_action = GetExportVariableValue<std::string>("move_forward_action", m_move_forward_action);
    m_move_backward_action = GetExportVariableValue<std::string>("move_backward_action", m_move_backward_action);
    m_jump_action = GetExportVariableValue<std::string>("jump_action", m_jump_action);
    m_sprint_action = GetExportVariableValue<std::string>("sprint_action", m_sprint_action);
    m_idle_left_animation = GetExportVariableValue<std::string>("idle_left_animation", m_idle_left_animation);
    m_idle_right_animation = GetExportVariableValue<std::string>("idle_right_animation", m_idle_right_animation);
    m_walk_left_animation = GetExportVariableValue<std::string>("walk_left_animation", m_walk_left_animation);
    m_walk_right_animation = GetExportVariableValue<std::string>("walk_right_animation", m_walk_right_animation);
    m_sprint_left_animation = GetExportVariableValue<std::string>("sprint_left_animation", m_sprint_left_animation);
    m_sprint_right_animation = GetExportVariableValue<std::string>("sprint_right_animation", m_sprint_right_animation);
    m_jump_up_animation = GetExportVariableValue<std::string>("jump_up_animation", m_jump_up_animation);
    m_jump_left_animation = GetExportVariableValue<std::string>("jump_left_animation", m_jump_left_animation);
    m_jump_right_animation = GetExportVariableValue<std::string>("jump_right_animation", m_jump_right_animation);
    m_jump_down_animation = GetExportVariableValue<std::string>("jump_down_animation", m_jump_down_animation);
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(PlayerController25D, "Player", "2.5D player controller with 3D movement and sprite animations")
