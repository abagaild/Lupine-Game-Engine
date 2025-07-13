#include "lupine/components/PlayerController2D.h"
#include "lupine/components/KinematicBody2D.h"
#include "lupine/components/AnimatedSprite2D.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/input/InputManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

namespace Lupine {

PlayerController2D::PlayerController2D()
    : Component("PlayerController2D")
    , m_speed(200.0f)
    , m_normalize_diagonal(true)
    , m_move_up_action("move_up")
    , m_move_down_action("move_down")
    , m_move_left_action("move_left")
    , m_move_right_action("move_right")
    , m_velocity(0.0f, 0.0f)
    , m_movement_direction(0.0f, 0.0f)
    , m_input_vector(0.0f, 0.0f)
    , m_idle_left_animation("idle_left")
    , m_idle_right_animation("idle_right")
    , m_idle_up_animation("idle_up")
    , m_idle_down_animation("idle_down")
    , m_walk_left_animation("walk_left")
    , m_walk_right_animation("walk_right")
    , m_walk_up_animation("walk_up")
    , m_walk_down_animation("walk_down") {
    
    InitializeExportVariables();
}

void PlayerController2D::InitializeExportVariables() {
    // Movement settings
    AddExportVariable("speed", m_speed, "Movement speed in units per second", ExportVariableType::Float);
    AddExportVariable("normalize_diagonal", m_normalize_diagonal,
                     "Whether to normalize diagonal movement to prevent faster diagonal movement", ExportVariableType::Bool);

    // Action names
    AddExportVariable("move_up_action", m_move_up_action, "Action name for moving up", ExportVariableType::String);
    AddExportVariable("move_down_action", m_move_down_action, "Action name for moving down", ExportVariableType::String);
    AddExportVariable("move_left_action", m_move_left_action, "Action name for moving left", ExportVariableType::String);
    AddExportVariable("move_right_action", m_move_right_action, "Action name for moving right", ExportVariableType::String);

    // Animation names
    AddExportVariable("idle_left_animation", m_idle_left_animation, "Animation for idle left", ExportVariableType::String);
    AddExportVariable("idle_right_animation", m_idle_right_animation, "Animation for idle right", ExportVariableType::String);
    AddExportVariable("idle_up_animation", m_idle_up_animation, "Animation for idle up", ExportVariableType::String);
    AddExportVariable("idle_down_animation", m_idle_down_animation, "Animation for idle down", ExportVariableType::String);
    AddExportVariable("walk_left_animation", m_walk_left_animation, "Animation for walking left", ExportVariableType::String);
    AddExportVariable("walk_right_animation", m_walk_right_animation, "Animation for walking right", ExportVariableType::String);
    AddExportVariable("walk_up_animation", m_walk_up_animation, "Animation for walking up", ExportVariableType::String);
    AddExportVariable("walk_down_animation", m_walk_down_animation, "Animation for walking down", ExportVariableType::String);
}

void PlayerController2D::OnReady() {
    // Component is ready, no special initialization needed
}

void PlayerController2D::OnUpdate(float delta_time) {
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

void PlayerController2D::OnInput(const void* event) {
    // Input is handled through action system in OnUpdate
    (void)event;
}

void PlayerController2D::UpdateInputVector() {
    m_input_vector = glm::vec2(0.0f, 0.0f);
    
    // Check each movement action
    if (InputManager::IsActionPressed(m_move_up_action)) {
        m_input_vector.y += 1.0f; // Positive Y is up
    }
    if (InputManager::IsActionPressed(m_move_down_action)) {
        m_input_vector.y -= 1.0f; // Negative Y is down
    }
    if (InputManager::IsActionPressed(m_move_left_action)) {
        m_input_vector.x -= 1.0f; // Negative X is left
    }
    if (InputManager::IsActionPressed(m_move_right_action)) {
        m_input_vector.x += 1.0f; // Positive X is right
    }
    
    // Normalize diagonal movement if enabled
    if (m_normalize_diagonal && glm::length(m_input_vector) > 1.0f) {
        m_input_vector = glm::normalize(m_input_vector);
    }
}

void PlayerController2D::UpdateMovement(float delta_time) {
    (void)delta_time; // Unused parameter

    // Calculate movement direction
    if (glm::length(m_input_vector) > 0.001f) {
        m_movement_direction = glm::normalize(m_input_vector);
    } else {
        m_movement_direction = glm::vec2(0.0f, 0.0f);
    }
    
    // Calculate velocity
    m_velocity = m_input_vector * m_speed;
}

void PlayerController2D::ApplyMovement(float delta_time) {
    // Try to find a KinematicBody2D component on the same node
    KinematicBody2D* kinematic_body = GetOwner()->GetComponent<KinematicBody2D>();

    if (kinematic_body) {
        // Use kinematic body for physics-based movement
        if (glm::length(m_velocity) > 0.001f) {
            std::cout << "PlayerController2D: Using KinematicBody2D for movement with velocity ("
                      << m_velocity.x << ", " << m_velocity.y << ")" << std::endl;
            kinematic_body->MoveAndSlide(m_velocity, delta_time);
        }
    } else {
        // Fallback to direct node movement if no kinematic body is present
        Node2D* node2d = dynamic_cast<Node2D*>(GetOwner());
        if (!node2d) {
            std::cout << "PlayerController2D: Owner is not Node2D" << std::endl;
            return; // This component only works with Node2D
        }

        // Apply movement directly to node
        if (glm::length(m_velocity) > 0.001f) {
            glm::vec2 currentPosition = node2d->GetPosition();
            glm::vec2 newPosition = currentPosition + (m_velocity * delta_time);
            std::cout << "PlayerController2D: Direct movement from (" << currentPosition.x << ", " << currentPosition.y
                      << ") to (" << newPosition.x << ", " << newPosition.y << ")" << std::endl;
            node2d->SetPosition(newPosition);
        } else {
            static int idle_counter = 0;
            if (idle_counter % 60 == 0) {
                std::cout << "PlayerController2D: No movement (velocity length: " << glm::length(m_velocity) << ")" << std::endl;
            }
            idle_counter++;
        }
    }
}

void PlayerController2D::SetIdleAnimations(const std::string& left, const std::string& right,
                                          const std::string& up, const std::string& down) {
    m_idle_left_animation = left;
    m_idle_right_animation = right;
    m_idle_up_animation = up;
    m_idle_down_animation = down;
}

void PlayerController2D::SetWalkAnimations(const std::string& left, const std::string& right,
                                          const std::string& up, const std::string& down) {
    m_walk_left_animation = left;
    m_walk_right_animation = right;
    m_walk_up_animation = up;
    m_walk_down_animation = down;
}

void PlayerController2D::UpdateAnimation() {
    // Find AnimatedSprite2D component on the same node
    AnimatedSprite2D* animated_sprite = GetOwner()->GetComponent<AnimatedSprite2D>();
    if (!animated_sprite) {
        return; // No animated sprite to control
    }

    std::string target_animation;

    if (IsMoving()) {
        // Determine walk animation based on primary movement direction
        if (std::abs(m_input_vector.x) > std::abs(m_input_vector.y)) {
            // Horizontal movement is dominant
            if (m_input_vector.x > 0) {
                target_animation = m_walk_right_animation;
            } else {
                target_animation = m_walk_left_animation;
            }
        } else {
            // Vertical movement is dominant
            if (m_input_vector.y > 0) {
                target_animation = m_walk_up_animation;
            } else {
                target_animation = m_walk_down_animation;
            }
        }
    } else {
        // Use last movement direction for idle animation
        if (std::abs(m_movement_direction.x) > std::abs(m_movement_direction.y)) {
            // Last movement was horizontal
            if (m_movement_direction.x > 0) {
                target_animation = m_idle_right_animation;
            } else {
                target_animation = m_idle_left_animation;
            }
        } else if (glm::length(m_movement_direction) > 0.001f) {
            // Last movement was vertical
            if (m_movement_direction.y > 0) {
                target_animation = m_idle_up_animation;
            } else {
                target_animation = m_idle_down_animation;
            }
        } else {
            // Default to idle right if no previous movement
            target_animation = m_idle_right_animation;
        }
    }

    // Only change animation if it's different from current
    if (target_animation != m_current_animation && !target_animation.empty()) {
        m_current_animation = target_animation;
        animated_sprite->Play(target_animation);
    }
}

void PlayerController2D::UpdateFromExportVariables() {
    m_speed = GetExportVariableValue<float>("speed", m_speed);
    m_normalize_diagonal = GetExportVariableValue<bool>("normalize_diagonal", m_normalize_diagonal);
    m_move_up_action = GetExportVariableValue<std::string>("move_up_action", m_move_up_action);
    m_move_down_action = GetExportVariableValue<std::string>("move_down_action", m_move_down_action);
    m_move_left_action = GetExportVariableValue<std::string>("move_left_action", m_move_left_action);
    m_move_right_action = GetExportVariableValue<std::string>("move_right_action", m_move_right_action);
    m_idle_left_animation = GetExportVariableValue<std::string>("idle_left_animation", m_idle_left_animation);
    m_idle_right_animation = GetExportVariableValue<std::string>("idle_right_animation", m_idle_right_animation);
    m_idle_up_animation = GetExportVariableValue<std::string>("idle_up_animation", m_idle_up_animation);
    m_idle_down_animation = GetExportVariableValue<std::string>("idle_down_animation", m_idle_down_animation);
    m_walk_left_animation = GetExportVariableValue<std::string>("walk_left_animation", m_walk_left_animation);
    m_walk_right_animation = GetExportVariableValue<std::string>("walk_right_animation", m_walk_right_animation);
    m_walk_up_animation = GetExportVariableValue<std::string>("walk_up_animation", m_walk_up_animation);
    m_walk_down_animation = GetExportVariableValue<std::string>("walk_down_animation", m_walk_down_animation);
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(PlayerController2D, "Player", "Basic 2D player controller with action-based movement")
