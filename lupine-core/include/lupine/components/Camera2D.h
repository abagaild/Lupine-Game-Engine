#pragma once

#include "lupine/core/Component.h"
#include "lupine/rendering/Camera.h"
#include <glm/glm.hpp>
#include <memory>

namespace Lupine {

class Node2D;

/**
 * @brief 2D camera component for controlling 2D view and projection
 * 
 * Camera2D component provides 2D camera functionality with orthographic projection.
 * It can follow a target node and provides zoom, position, and rotation controls.
 * Should be attached to Node2D nodes.
 */
class Camera2D : public Component {
public:
    /**
     * @brief Constructor
     */
    Camera2D();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Camera2D() = default;
    
    /**
     * @brief Get camera position offset
     * @return 2D position offset from node position
     */
    const glm::vec2& GetOffset() const { return m_offset; }
    
    /**
     * @brief Set camera position offset
     * @param offset 2D position offset from node position
     */
    void SetOffset(const glm::vec2& offset);
    
    /**
     * @brief Get camera zoom level
     * @return Zoom level (1.0 = normal, >1.0 = zoomed in, <1.0 = zoomed out)
     */
    float GetZoom() const { return m_zoom; }
    
    /**
     * @brief Set camera zoom level
     * @param zoom Zoom level (1.0 = normal, >1.0 = zoomed in, <1.0 = zoomed out)
     */
    void SetZoom(float zoom);
    
    /**
     * @brief Get camera rotation
     * @return Rotation in radians
     */
    float GetRotation() const { return m_rotation; }
    
    /**
     * @brief Set camera rotation
     * @param rotation Rotation in radians
     */
    void SetRotation(float rotation);
    
    /**
     * @brief Get follow target node UUID
     * @return UUID of target node to follow (empty if no target)
     */
    const std::string& GetFollowTarget() const { return m_follow_target; }
    
    /**
     * @brief Set follow target node by UUID
     * @param target_uuid UUID of target node to follow (empty to disable following)
     */
    void SetFollowTarget(const std::string& target_uuid);
    
    /**
     * @brief Get follow smoothing factor
     * @return Smoothing factor (0.0 = instant, 1.0 = no movement)
     */
    float GetFollowSmoothing() const { return m_follow_smoothing; }
    
    /**
     * @brief Set follow smoothing factor
     * @param smoothing Smoothing factor (0.0 = instant, 1.0 = no movement)
     */
    void SetFollowSmoothing(float smoothing);
    
    /**
     * @brief Get whether camera is enabled
     * @return True if camera is enabled and should be used for rendering
     */
    bool IsEnabled() const { return m_enabled; }
    
    /**
     * @brief Set whether camera is enabled
     * @param enabled True to enable camera for rendering
     */
    void SetEnabled(bool enabled);
    
    /**
     * @brief Get whether camera is current (actively used for rendering)
     * @return True if this camera is currently active
     */
    bool IsCurrent() const { return m_is_current; }
    
    /**
     * @brief Set whether camera is current (actively used for rendering)
     * @param current True to make this camera active
     */
    void SetCurrent(bool current);
    
    /**
     * @brief Get the internal Camera object
     * @return Pointer to internal Camera object
     */
    Camera* GetCamera() const { return m_camera.get(); }
    
    /**
     * @brief Get camera bounds (for culling)
     * @return Camera bounds in world space (left, top, right, bottom)
     */
    glm::vec4 GetBounds() const;
    
    /**
     * @brief Convert screen coordinates to world coordinates
     * @param screen_pos Screen position (0,0 = top-left)
     * @param viewport_size Viewport size
     * @return World position
     */
    glm::vec2 ScreenToWorld(const glm::vec2& screen_pos, const glm::vec2& viewport_size) const;
    
    /**
     * @brief Convert world coordinates to screen coordinates
     * @param world_pos World position
     * @param viewport_size Viewport size
     * @return Screen position (0,0 = top-left)
     */
    glm::vec2 WorldToScreen(const glm::vec2& world_pos, const glm::vec2& viewport_size) const;

    /**
     * @brief Get component type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Camera2D"; }

    /**
     * @brief Update camera matrices and position
     */
    void UpdateCamera();

    // Component lifecycle methods
    virtual void OnAwake() override;
    virtual void OnReady() override;
    virtual void OnUpdate(float delta_time) override;

protected:
    /**
     * @brief Initialize export variables
     */
    virtual void InitializeExportVariables() override;
    
    /**
     * @brief Update internal state from export variables
     */
    void UpdateFromExportVariables();

    /**
     * @brief Update follow target position
     * @param delta_time Time since last update
     */
    void UpdateFollowTarget(float delta_time);

private:
    std::unique_ptr<Camera> m_camera;
    glm::vec2 m_offset;
    float m_zoom;
    float m_rotation;
    std::string m_follow_target;
    float m_follow_smoothing;
    bool m_enabled;
    bool m_is_current;
    
    // Cached values
    glm::vec2 m_viewport_size;
    bool m_matrices_dirty;
};

} // namespace Lupine
