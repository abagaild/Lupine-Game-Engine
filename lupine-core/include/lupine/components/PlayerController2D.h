#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>

namespace Lupine {

class AnimatedSprite2D;

/**
 * @brief Basic 2D player controller component
 * 
 * This component provides basic 2D movement using action mappings.
 * It responds to move_up, move_down, move_left, and move_right actions
 * and moves the Node2D accordingly.
 */
class PlayerController2D : public Component {
public:
    /**
     * @brief Constructor
     */
    PlayerController2D();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~PlayerController2D() = default;
    
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
     * @brief Set movement speed
     * @param speed Movement speed in units per second
     */
    void SetSpeed(float speed) { m_speed = speed; }
    
    /**
     * @brief Get movement speed
     * @return Movement speed in units per second
     */
    float GetSpeed() const { return m_speed; }
    
    /**
     * @brief Set whether to normalize diagonal movement
     * @param normalize True to normalize diagonal movement
     */
    void SetNormalizeDiagonal(bool normalize) { m_normalize_diagonal = normalize; }
    
    /**
     * @brief Get whether diagonal movement is normalized
     * @return True if diagonal movement is normalized
     */
    bool GetNormalizeDiagonal() const { return m_normalize_diagonal; }
    
    // === Action Names ===
    
    /**
     * @brief Set the action name for moving up
     * @param action_name Action name
     */
    void SetMoveUpAction(const std::string& action_name) { m_move_up_action = action_name; }
    
    /**
     * @brief Get the action name for moving up
     * @return Action name
     */
    const std::string& GetMoveUpAction() const { return m_move_up_action; }
    
    /**
     * @brief Set the action name for moving down
     * @param action_name Action name
     */
    void SetMoveDownAction(const std::string& action_name) { m_move_down_action = action_name; }
    
    /**
     * @brief Get the action name for moving down
     * @return Action name
     */
    const std::string& GetMoveDownAction() const { return m_move_down_action; }
    
    /**
     * @brief Set the action name for moving left
     * @param action_name Action name
     */
    void SetMoveLeftAction(const std::string& action_name) { m_move_left_action = action_name; }
    
    /**
     * @brief Get the action name for moving left
     * @return Action name
     */
    const std::string& GetMoveLeftAction() const { return m_move_left_action; }
    
    /**
     * @brief Set the action name for moving right
     * @param action_name Action name
     */
    void SetMoveRightAction(const std::string& action_name) { m_move_right_action = action_name; }
    
    /**
     * @brief Get the action name for moving right
     * @return Action name
     */
    const std::string& GetMoveRightAction() const { return m_move_right_action; }
    
    // === Movement State ===
    
    /**
     * @brief Get current movement velocity
     * @return Current velocity vector
     */
    glm::vec2 GetVelocity() const { return m_velocity; }
    
    /**
     * @brief Get current movement direction
     * @return Normalized movement direction vector
     */
    glm::vec2 GetMovementDirection() const { return m_movement_direction; }
    
    /**
     * @brief Check if the player is currently moving
     * @return True if moving
     */
    bool IsMoving() const { return glm::length(m_velocity) > 0.001f; }

    // === Animation Settings ===

    /**
     * @brief Set animation names for different movement states
     */
    void SetIdleAnimations(const std::string& left, const std::string& right,
                          const std::string& up, const std::string& down);
    void SetWalkAnimations(const std::string& left, const std::string& right,
                          const std::string& up, const std::string& down);

protected:
    /**
     * @brief Initialize export variables for the editor
     */
    void InitializeExportVariables() override;

private:
    // Movement settings
    float m_speed;
    bool m_normalize_diagonal;
    
    // Action names
    std::string m_move_up_action;
    std::string m_move_down_action;
    std::string m_move_left_action;
    std::string m_move_right_action;
    
    // Movement state
    glm::vec2 m_velocity;
    glm::vec2 m_movement_direction;
    glm::vec2 m_input_vector;

    // Animation settings
    std::string m_idle_left_animation;
    std::string m_idle_right_animation;
    std::string m_idle_up_animation;
    std::string m_idle_down_animation;
    std::string m_walk_left_animation;
    std::string m_walk_right_animation;
    std::string m_walk_up_animation;
    std::string m_walk_down_animation;

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
    std::string GetTypeName() const override { return "PlayerController2D"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "Player"; }
};

} // namespace Lupine
