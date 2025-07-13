#include "lupine/components/PlatformerController2D.h"
#include "lupine/components/KinematicBody2D.h"
#include "lupine/components/AnimatedSprite2D.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/input/InputManager.h"
#include <algorithm>
#include <iostream>

namespace Lupine {

PlatformerController2D::PlatformerController2D()
    : Component("PlatformerController2D")
    , m_speed(200.0f)
    , m_jump_force(400.0f)
    , m_gravity(800.0f)
    , m_gravity_enabled(true)
    , m_move_left_action("move_left")
    , m_move_right_action("move_right")
    , m_jump_action("jump")
    , m_velocity(0.0f, 0.0f)
    , m_input_vector(0.0f, 0.0f)
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

void PlatformerController2D::InitializeExportVariables() {
    // Movement settings
    AddExportVariable("speed", m_speed, "Horizontal movement speed in units per second", ExportVariableType::Float);
    AddExportVariable("jump_force", m_jump_force, "Jump force in units per second", ExportVariableType::Float);
    AddExportVariable("gravity", m_gravity, "Gravity force in units per second squared", ExportVariableType::Float);
    AddExportVariable("gravity_enabled", m_gravity_enabled, "Enable gravity", ExportVariableType::Bool);
    
    // Action mappings
    AddExportVariable("move_left_action", m_move_left_action, "Action name for moving left", ExportVariableType::String);
    AddExportVariable("move_right_action", m_move_right_action, "Action name for moving right", ExportVariableType::String);
    AddExportVariable("jump_action", m_jump_action, "Action name for jumping", ExportVariableType::String);

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

void PlatformerController2D::OnReady() {
    // Component is ready
}

void PlatformerController2D::OnUpdate(float delta_time) {
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

void PlatformerController2D::OnInput(const void* event) {
    // Input is handled through action system in OnUpdate
    (void)event;
}

void PlatformerController2D::UpdateInputVector() {
    m_input_vector.x = 0.0f;
    
    // Check horizontal movement actions
    if (InputManager::IsActionPressed(m_move_left_action)) {
        m_input_vector.x -= 1.0f; // Negative X is left
    }
    if (InputManager::IsActionPressed(m_move_right_action)) {
        m_input_vector.x += 1.0f; // Positive X is right
    }
}

void PlatformerController2D::UpdateMovement(float delta_time) {
    // Update horizontal velocity based on input
    m_velocity.x = m_input_vector.x * m_speed;
    
    // Handle jumping
    if (InputManager::IsActionJustPressed(m_jump_action) && m_is_on_ground) {
        m_velocity.y = m_jump_force;
        m_is_on_ground = false;
        std::cout << "PlatformerController2D: Jump! Velocity Y = " << m_velocity.y << std::endl;
    }
    
    // Apply gravity if enabled
    if (m_gravity_enabled && !m_is_on_ground) {
        m_velocity.y -= m_gravity * delta_time;
    }
}

void PlatformerController2D::ApplyMovement(float delta_time) {
    // Try to find a KinematicBody2D component on the same node
    KinematicBody2D* kinematic_body = GetOwner()->GetComponent<KinematicBody2D>();

    if (kinematic_body) {
        // Use kinematic body for physics-based movement
        if (glm::length(m_velocity) > 0.001f) {
            std::cout << "PlatformerController2D: Using KinematicBody2D for movement with velocity ("
                      << m_velocity.x << ", " << m_velocity.y << ")" << std::endl;
            
            // Use MoveAndSlide for proper platformer physics
            bool moved = kinematic_body->MoveAndSlide(m_velocity, delta_time);
            (void)moved; // Unused variable
            
            // Update ground state from kinematic body
            m_is_on_ground = kinematic_body->IsOnFloor();
            
            // If we hit the ground, stop vertical velocity
            if (m_is_on_ground && m_velocity.y < 0.0f) {
                m_velocity.y = 0.0f;
                std::cout << "PlatformerController2D: Landed on ground" << std::endl;
            }
        }
    } else {
        // Fallback to direct node movement if no kinematic body is present
        Node2D* node2d = dynamic_cast<Node2D*>(GetOwner());
        if (!node2d) {
            std::cout << "PlatformerController2D: Owner is not Node2D" << std::endl;
            return; // This component only works with Node2D
        }

        // Apply movement directly to node (simplified physics)
        if (glm::length(m_velocity) > 0.001f) {
            glm::vec2 currentPosition = node2d->GetPosition();
            glm::vec2 newPosition = currentPosition + (m_velocity * delta_time);
            
            // Simple ground collision at Y = 0
            if (newPosition.y <= 0.0f) {
                newPosition.y = 0.0f;
                m_velocity.y = 0.0f;
                m_is_on_ground = true;
            } else {
                m_is_on_ground = false;
            }
            
            std::cout << "PlatformerController2D: Direct movement from (" << currentPosition.x << ", " << currentPosition.y
                      << ") to (" << newPosition.x << ", " << newPosition.y << ")" << std::endl;
            node2d->SetPosition(newPosition);
        }
    }
}

void PlatformerController2D::SetIdleAnimations(const std::string& left, const std::string& right) {
    m_idle_left_animation = left;
    m_idle_right_animation = right;
}

void PlatformerController2D::SetWalkAnimations(const std::string& left, const std::string& right) {
    m_walk_left_animation = left;
    m_walk_right_animation = right;
}

void PlatformerController2D::SetSprintAnimations(const std::string& left, const std::string& right) {
    m_sprint_left_animation = left;
    m_sprint_right_animation = right;
}

void PlatformerController2D::SetJumpAnimations(const std::string& up, const std::string& left,
                                              const std::string& right, const std::string& down) {
    m_jump_up_animation = up;
    m_jump_left_animation = left;
    m_jump_right_animation = right;
    m_jump_down_animation = down;
}

void PlatformerController2D::UpdateAnimation() {
    // Find AnimatedSprite2D component on the same node
    AnimatedSprite2D* animated_sprite = GetOwner()->GetComponent<AnimatedSprite2D>();
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
    } else if (std::abs(m_velocity.x) > 0.1f) {
        // On ground and moving horizontally
        // For now, just use walk animations (could add sprint detection later)
        target_animation = (m_velocity.x > 0) ? m_walk_right_animation : m_walk_left_animation;
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

void PlatformerController2D::UpdateFromExportVariables() {
    m_speed = GetExportVariableValue<float>("speed", m_speed);
    m_jump_force = GetExportVariableValue<float>("jump_force", m_jump_force);
    m_gravity = GetExportVariableValue<float>("gravity", m_gravity);
    m_move_left_action = GetExportVariableValue<std::string>("move_left_action", m_move_left_action);
    m_move_right_action = GetExportVariableValue<std::string>("move_right_action", m_move_right_action);
    m_jump_action = GetExportVariableValue<std::string>("jump_action", m_jump_action);
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
REGISTER_COMPONENT(PlatformerController2D, "Player", "2D platformer controller with gravity, jumping, and left/right movement")
