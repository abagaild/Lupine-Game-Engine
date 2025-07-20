#pragma once

#include "lupine/core/Component.h"
#include "lupine/rendering/Camera.h"
#include <glm/glm.hpp>
#include <memory>

namespace Lupine {

class Node3D;

/**
 * @brief 3D camera component for controlling 3D view and projection
 * 
 * Camera3D component provides 3D camera functionality with perspective or orthographic projection.
 * It can follow a target node and provides FOV, position, rotation, and clipping plane controls.
 * Should be attached to Node3D nodes.
 */
class Camera3D : public Component {
public:
    /**
     * @brief Constructor
     */
    Camera3D();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Camera3D() = default;
    
    /**
     * @brief Get camera position offset
     * @return 3D position offset from node position
     */
    const glm::vec3& GetOffset() const { return m_offset; }
    
    /**
     * @brief Set camera position offset
     * @param offset 3D position offset from node position
     */
    void SetOffset(const glm::vec3& offset);
    
    /**
     * @brief Get field of view
     * @return FOV in radians (for perspective projection)
     */
    float GetFOV() const { return m_fov; }
    
    /**
     * @brief Set field of view
     * @param fov FOV in radians (for perspective projection)
     */
    void SetFOV(float fov);
    
    /**
     * @brief Get near clipping plane distance
     * @return Near plane distance
     */
    float GetNearPlane() const { return m_near_plane; }
    
    /**
     * @brief Set near clipping plane distance
     * @param near_plane Near plane distance
     */
    void SetNearPlane(float near_plane);
    
    /**
     * @brief Get far clipping plane distance
     * @return Far plane distance
     */
    float GetFarPlane() const { return m_far_plane; }
    
    /**
     * @brief Set far clipping plane distance
     * @param far_plane Far plane distance
     */
    void SetFarPlane(float far_plane);
    
    /**
     * @brief Get projection type
     * @return Projection type (Perspective or Orthographic)
     */
    ProjectionType GetProjectionType() const { return m_projection_type; }
    
    /**
     * @brief Set projection type
     * @param type Projection type (Perspective or Orthographic)
     */
    void SetProjectionType(ProjectionType type);
    
    /**
     * @brief Get orthographic size (for orthographic projection)
     * @return Orthographic size (half-height of view volume)
     */
    float GetOrthographicSize() const { return m_orthographic_size; }

    /**
     * @brief Set orthographic size (for orthographic projection)
     * @param size Orthographic size (half-height of view volume)
     */
    void SetOrthographicSize(float size);

    /**
     * @brief Get aspect ratio
     * @return Current aspect ratio (width/height)
     */
    float GetAspectRatio() const { return m_aspect_ratio; }
    
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
     * @brief Get follow distance for 3rd person cameras
     * @return Distance from target for 3rd person view
     */
    float GetFollowDistance() const { return m_follow_distance; }

    /**
     * @brief Set follow distance for 3rd person cameras
     * @param distance Distance from target for 3rd person view
     */
    void SetFollowDistance(float distance);

    /**
     * @brief Get follow height for 3rd person cameras
     * @return Height offset from target for 3rd person view
     */
    float GetFollowHeight() const { return m_follow_height; }

    /**
     * @brief Set follow height for 3rd person cameras
     * @param height Height offset from target for 3rd person view
     */
    void SetFollowHeight(float height);

    /**
     * @brief Get whether mouse orbit is enabled
     * @return True if right mouse + movement controls camera orbit
     */
    bool IsMouseOrbitEnabled() const { return m_mouse_orbit_enabled; }

    /**
     * @brief Set whether mouse orbit is enabled
     * @param enabled True to enable right mouse + movement for camera orbit
     */
    void SetMouseOrbitEnabled(bool enabled);

    /**
     * @brief Get whether tank control orbit is enabled
     * @return True if action-based camera orbit is enabled
     */
    bool IsTankOrbitEnabled() const { return m_tank_orbit_enabled; }

    /**
     * @brief Set whether tank control orbit is enabled
     * @param enabled True to enable action-based camera orbit
     */
    void SetTankOrbitEnabled(bool enabled);

    /**
     * @brief Get whether scroll wheel zoom is enabled
     * @return True if scroll wheel controls zoom
     */
    bool IsScrollZoomEnabled() const { return m_scroll_zoom_enabled; }

    /**
     * @brief Set whether scroll wheel zoom is enabled
     * @param enabled True to enable scroll wheel zoom
     */
    void SetScrollZoomEnabled(bool enabled);

    /**
     * @brief Get minimum zoom distance
     * @return Minimum zoom distance
     */
    float GetMinZoom() const { return m_min_zoom; }

    /**
     * @brief Set minimum zoom distance
     * @param min_zoom Minimum zoom distance
     */
    void SetMinZoom(float min_zoom);

    /**
     * @brief Get maximum zoom distance
     * @return Maximum zoom distance
     */
    float GetMaxZoom() const { return m_max_zoom; }

    /**
     * @brief Set maximum zoom distance
     * @param max_zoom Maximum zoom distance
     */
    void SetMaxZoom(float max_zoom);

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
     * @brief Get current orbit yaw angle
     * @return Orbit yaw angle in degrees
     */
    float GetOrbitYaw() const { return m_orbit_yaw; }

    /**
     * @brief Get current orbit pitch angle
     * @return Orbit pitch angle in degrees
     */
    float GetOrbitPitch() const { return m_orbit_pitch; }

    /**
     * @brief Get camera frustum bounds (for culling)
     * @return Camera frustum bounds in world space
     */
    glm::mat4 GetFrustumMatrix() const;
    
    /**
     * @brief Convert screen coordinates to world ray
     * @param screen_pos Screen position (0,0 = top-left)
     * @param viewport_size Viewport size
     * @return Ray origin and direction (origin.xyz, direction.xyz)
     */
    std::pair<glm::vec3, glm::vec3> ScreenToWorldRay(const glm::vec2& screen_pos, const glm::vec2& viewport_size) const;
    
    /**
     * @brief Convert world coordinates to screen coordinates
     * @param world_pos World position
     * @param viewport_size Viewport size
     * @return Screen position (0,0 = top-left) and depth
     */
    glm::vec3 WorldToScreen(const glm::vec3& world_pos, const glm::vec2& viewport_size) const;

    /**
     * @brief Get component type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Camera3D"; }

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

    /**
     * @brief Update 3rd person camera orbit controls
     * @param delta_time Time since last update
     */
    void UpdateOrbitControls(float delta_time);

    /**
     * @brief Update zoom controls
     * @param delta_time Time since last update
     */
    void UpdateZoomControls(float delta_time);

private:
    std::unique_ptr<Camera> m_camera;
    glm::vec3 m_offset;
    float m_fov;
    float m_near_plane;
    float m_far_plane;
    ProjectionType m_projection_type;
    float m_orthographic_size;
    std::string m_follow_target;
    float m_follow_smoothing;
    bool m_enabled;
    bool m_is_current;

    // 3rd person camera features
    float m_follow_distance;
    float m_follow_height;
    bool m_mouse_orbit_enabled;
    bool m_tank_orbit_enabled;
    bool m_scroll_zoom_enabled;
    float m_min_zoom;
    float m_max_zoom;

    // Orbit state
    float m_orbit_yaw;
    float m_orbit_pitch;
    float m_current_zoom;

    // Tank control action names
    std::string m_orbit_left_action;
    std::string m_orbit_right_action;
    std::string m_orbit_up_action;
    std::string m_orbit_down_action;

    // Cached values
    glm::vec2 m_viewport_size;
    float m_aspect_ratio;
    bool m_matrices_dirty;
};

} // namespace Lupine
