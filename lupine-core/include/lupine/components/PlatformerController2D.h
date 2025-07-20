#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>

namespace Lupine {

class AnimatedSprite2D;

/**
 * @brief 2D Platformer player controller component
 * 
 * This component provides platformer-style 2D movement with gravity, jumping,
 * and left/right movement. It integrates with KinematicBody2D for proper
 * physics-based movement and collision detection.
 */
class PlatformerController2D : public Component {
public:
    /**
     * @brief Constructor
     */
    PlatformerController2D();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~PlatformerController2D() = default;
    
    /**
     * @brief Component lifecycle - called when component is ready
     */
    void OnReady() override;
    
    /**
     * @brief Component lifecycle - called every frame
     * @param delta_time Time since last frame in seconds
     */
    void OnUpdate(float delta_time) override;
    
    /**
     * @brief Component lifecycle - called for input events
     * @param event Input event data
     */
    void OnInput(const void* event) override;
    
    // === Movement Settings ===
    
    /**
     * @brief Set horizontal movement speed
     * @param speed Movement speed in units per second
     */
    void SetSpeed(float speed) { m_speed = speed; }
    
    /**
     * @brief Get horizontal movement speed
     * @return Movement speed in units per second
     */
    float GetSpeed() const { return m_speed; }
    
    /**
     * @brief Set jump force
     * @param force Jump force in units per second
     */
    void SetJumpForce(float force) { m_jump_force = force; }
    
    /**
     * @brief Get jump force
     * @return Jump force in units per second
     */
    float GetJumpForce() const { return m_jump_force; }
    
    /**
     * @brief Set gravity force
     * @param gravity Gravity force in units per second squared
     */
    void SetGravity(float gravity) { m_gravity = gravity; }
    
    /**
     * @brief Get gravity force
     * @return Gravity force in units per second squared
     */
    float GetGravity() const { return m_gravity; }
    
    /**
     * @brief Enable/disable gravity
     * @param enabled True to enable gravity
     */
    void SetGravityEnabled(bool enabled) { m_gravity_enabled = enabled; }
    
    /**
     * @brief Check if gravity is enabled
     * @return True if gravity is enabled
     */
    bool IsGravityEnabled() const { return m_gravity_enabled; }
    
    // === Action Settings ===
    
    /**
     * @brief Set left movement action name
     * @param action Action name
     */
    void SetMoveLeftAction(const std::string& action) { m_move_left_action = action; }
    
    /**
     * @brief Get left movement action name
     * @return Action name
     */
    const std::string& GetMoveLeftAction() const { return m_move_left_action; }
    
    /**
     * @brief Set right movement action name
     * @param action Action name
     */
    void SetMoveRightAction(const std::string& action) { m_move_right_action = action; }
    
    /**
     * @brief Get right movement action name
     * @return Action name
     */
    const std::string& GetMoveRightAction() const { return m_move_right_action; }
    
    /**
     * @brief Set jump action name
     * @param action Action name
     */
    void SetJumpAction(const std::string& action) { m_jump_action = action; }
    
    /**
     * @brief Get jump action name
     * @return Action name
     */
    const std::string& GetJumpAction() const { return m_jump_action; }
    
    // === State Queries ===
    
    /**
     * @brief Check if player is on ground
     * @return True if on ground
     */
    bool IsOnGround() const { return m_is_on_ground; }
    
    /**
     * @brief Get current velocity
     * @return Current velocity vector
     */
    const glm::vec2& GetVelocity() const { return m_velocity; }

    // === Animation Settings ===

    /**
     * @brief Set animation names for different movement states
     */
    void SetIdleAnimations(const std::string& left, const std::string& right);
    void SetWalkAnimations(const std::string& left, const std::string& right);
    void SetSprintAnimations(const std::string& left, const std::string& right);
    void SetJumpAnimations(const std::string& up, const std::string& left,
                          const std::string& right, const std::string& down);

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "PlatformerController2D"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "Player"; }

protected:
    /**
     * @brief Initialize export variables
     */
    void InitializeExportVariables() override;

private:
    // Movement settings
    float m_speed;
    float m_jump_force;
    float m_gravity;
    bool m_gravity_enabled;
    
    // Action names
    std::string m_move_left_action;
    std::string m_move_right_action;
    std::string m_jump_action;
    
    // Movement state
    glm::vec2 m_velocity;
    glm::vec2 m_input_vector;
    bool m_is_on_ground;

    // Animation settings
    std::string m_idle_left_animation;
    std::string m_idle_right_animation;
    std::string m_walk_left_animation;
    std::string m_walk_right_animation;
    std::string m_sprint_left_animation;
    std::string m_sprint_right_animation;
    std::string m_jump_up_animation;
    std::string m_jump_left_animation;
    std::string m_jump_right_animation;
    std::string m_jump_down_animation;

    // Animation state
    std::string m_current_animation;
    
    /**
     * @brief Update input vector based on action states
     */
    void UpdateInputVector();
    
    /**
     * @brief Update movement based on input vector
     * @param delta_time Time since last frame
     */
    void UpdateMovement(float delta_time);
    
    /**
     * @brief Apply movement to the owner node
     * @param delta_time Time since last frame
     */
    void ApplyMovement(float delta_time);

    /**
     * @brief Update animation based on current movement state
     */
    void UpdateAnimation();

    /**
     * @brief Update component properties from export variables
     */
    void UpdateFromExportVariables();
};

} // namespace Lupine
