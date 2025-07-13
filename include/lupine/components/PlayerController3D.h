#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>

namespace Lupine {

/**
 * @brief Basic 3D player controller component
 * 
 * This component provides basic 3D movement using action mappings.
 * It responds to move_forward, move_backward, move_left, move_right, and jump actions
 * and moves the Node3D accordingly with gravity and ground detection.
 */
class PlayerController3D : public Component {
public:
    /**
     * @brief Constructor
     */
    PlayerController3D();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~PlayerController3D() = default;
    
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
     * @brief Set gravity strength
     * @param gravity Gravity in units per second squared
     */
    void SetGravity(float gravity) { m_gravity = gravity; }
    
    /**
     * @brief Get gravity strength
     * @return Gravity in units per second squared
     */
    float GetGravity() const { return m_gravity; }
    
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
    
    // === Action Settings ===
    
    /**
     * @brief Set forward movement action name
     * @param action Action name
     */
    void SetMoveForwardAction(const std::string& action) { m_move_forward_action = action; }
    
    /**
     * @brief Get forward movement action name
     * @return Action name
     */
    const std::string& GetMoveForwardAction() const { return m_move_forward_action; }
    
    /**
     * @brief Set backward movement action name
     * @param action Action name
     */
    void SetMoveBackwardAction(const std::string& action) { m_move_backward_action = action; }
    
    /**
     * @brief Get backward movement action name
     * @return Action name
     */
    const std::string& GetMoveBackwardAction() const { return m_move_backward_action; }
    
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
    const glm::vec3& GetVelocity() const { return m_velocity; }

    /**
     * @brief Get whether camera-relative movement is enabled
     * @return True if movement is relative to camera direction
     */
    bool IsCameraRelativeMovement() const { return m_camera_relative_movement; }

    /**
     * @brief Set whether camera-relative movement is enabled
     * @param enabled True to enable camera-relative movement
     */
    void SetCameraRelativeMovement(bool enabled);

    /**
     * @brief Get camera node reference UUID
     * @return UUID of camera node for relative movement
     */
    const std::string& GetCameraNodeReference() const { return m_camera_node_reference; }

    /**
     * @brief Set camera node reference by UUID
     * @param camera_uuid UUID of camera node for relative movement
     */
    void SetCameraNodeReference(const std::string& camera_uuid);

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "PlayerController3D"; }

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
    bool m_normalize_diagonal;
    
    // Action names
    std::string m_move_forward_action;
    std::string m_move_backward_action;
    std::string m_move_left_action;
    std::string m_move_right_action;
    std::string m_jump_action;
    
    // Movement state
    glm::vec3 m_velocity;
    glm::vec3 m_movement_direction;
    glm::vec3 m_input_vector;
    bool m_is_on_ground;
    float m_ground_y;

    // Camera-relative movement
    bool m_camera_relative_movement;
    std::string m_camera_node_reference;
    
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
     * @brief Check ground collision (simple Y-based for now)
     * @param position Current position
     * @return True if on ground
     */
    bool CheckGroundCollision(const glm::vec3& position);

    /**
     * @brief Get camera direction vectors for relative movement
     * @param forward Output forward direction
     * @param right Output right direction
     * @return True if camera was found and directions calculated
     */
    bool GetCameraDirections(glm::vec3& forward, glm::vec3& right) const;
};

} // namespace Lupine
