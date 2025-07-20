#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>

namespace Lupine {

class AnimatedSprite3D;

/**
 * @brief 2.5D Player controller component
 * 
 * This component provides 3D movement with 2D sprite animation support.
 * It combines 3D positioning and physics with sprite sheet animations
 * for a 2.5D game style. Uses AnimatedSprite3D for rendering.
 */
class PlayerController25D : public Component {
public:
    /**
     * @brief Constructor
     */
    PlayerController25D();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~PlayerController25D() = default;
    
    /**
     * @brief Component lifecycle - called when component is ready
     */
    void OnReady() override;
    
    /**
     * @brief Component lifecycle - called every frame
     * @param delta_time Time since last frame in seconds
     */
    void OnUpdate(float delta_time) override;
    
    // === Movement Settings ===
    
    /**
     * @brief Set movement speed
     * @param speed Movement speed in units per second
     */
    void SetSpeed(float speed) { m_speed = speed; }
    
    /**
     * @brief Get movement speed
     * @return Current movement speed
     */
    float GetSpeed() const { return m_speed; }
    
    /**
     * @brief Set jump force
     * @param force Jump force in units per second
     */
    void SetJumpForce(float force) { m_jump_force = force; }
    
    /**
     * @brief Get jump force
     * @return Current jump force
     */
    float GetJumpForce() const { return m_jump_force; }
    
    /**
     * @brief Set gravity force
     * @param gravity Gravity force in units per second squared
     */
    void SetGravity(float gravity) { m_gravity = gravity; }
    
    /**
     * @brief Get gravity force
     * @return Current gravity force
     */
    float GetGravity() const { return m_gravity; }
    
    /**
     * @brief Set sprint multiplier
     * @param multiplier Sprint speed multiplier
     */
    void SetSprintMultiplier(float multiplier) { m_sprint_multiplier = multiplier; }
    
    /**
     * @brief Get sprint multiplier
     * @return Current sprint multiplier
     */
    float GetSprintMultiplier() const { return m_sprint_multiplier; }
    
    // === State Queries ===
    
    /**
     * @brief Check if the player is on the ground
     * @return True if on ground
     */
    bool IsOnGround() const { return m_is_on_ground; }
    
    /**
     * @brief Check if the player is currently moving
     * @return True if moving
     */
    bool IsMoving() const { return glm::length(glm::vec2(m_velocity.x, m_velocity.z)) > 0.001f; }
    
    /**
     * @brief Check if the player is sprinting
     * @return True if sprinting
     */
    bool IsSprinting() const;
    
    /**
     * @brief Get current velocity
     * @return Current velocity vector
     */
    const glm::vec3& GetVelocity() const { return m_velocity; }
    
    // === Animation Settings ===
    
    /**
     * @brief Set animation names for different movement states
     */
    void SetIdleAnimations(const std::string& left, const std::string& right);
    void SetWalkAnimations(const std::string& left, const std::string& right);
    void SetSprintAnimations(const std::string& left, const std::string& right);
    void SetJumpAnimations(const std::string& up, const std::string& left, 
                          const std::string& right, const std::string& down);

protected:
    void InitializeExportVariables() override;

private:
    // Movement settings
    float m_speed;
    float m_jump_force;
    float m_gravity;
    float m_sprint_multiplier;
    
    // Action names
    std::string m_move_left_action;
    std::string m_move_right_action;
    std::string m_move_forward_action;
    std::string m_move_backward_action;
    std::string m_jump_action;
    std::string m_sprint_action;
    
    // Movement state
    glm::vec3 m_velocity;
    glm::vec3 m_input_vector;
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

public:
    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "PlayerController2.5D"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "Player"; }
};

} // namespace Lupine
