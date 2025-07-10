#include "lupine/components/Camera3D.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/core/Scene.h"
#include "lupine/rendering/ViewportManager.h"
#include "lupine/input/InputManager.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <algorithm>
#include <iostream>

namespace Lupine {

Camera3D::Camera3D()
    : Component("Camera3D")
    , m_camera(std::make_unique<Camera>(ProjectionType::Perspective))
    , m_offset(0.0f, 0.0f, 0.0f)
    , m_fov(glm::radians(45.0f))
    , m_near_plane(0.1f)
    , m_far_plane(100.0f)
    , m_projection_type(ProjectionType::Perspective)
    , m_orthographic_size(10.0f)
    , m_follow_target("")
    , m_follow_smoothing(0.1f)
    , m_enabled(true)
    , m_is_current(false)
    , m_follow_distance(5.0f)
    , m_follow_height(2.0f)
    , m_mouse_orbit_enabled(true)
    , m_tank_orbit_enabled(false)
    , m_scroll_zoom_enabled(true)
    , m_min_zoom(1.0f)
    , m_max_zoom(20.0f)
    , m_orbit_yaw(180.0f)  // Start behind the player
    , m_orbit_pitch(-15.0f) // Slight downward angle
    , m_current_zoom(5.0f)
    , m_orbit_left_action("camera_orbit_left")
    , m_orbit_right_action("camera_orbit_right")
    , m_orbit_up_action("camera_orbit_up")
    , m_orbit_down_action("camera_orbit_down")
    , m_viewport_size(800.0f, 600.0f)
    , m_aspect_ratio(800.0f / 600.0f)
    , m_matrices_dirty(true) {
    
    // Initialize export variables
    InitializeExportVariables();
    
    // Set up default perspective projection
    m_camera->SetPerspective(m_fov, m_aspect_ratio, m_near_plane, m_far_plane);
}

void Camera3D::SetOffset(const glm::vec3& offset) {
    m_offset = offset;
    SetExportVariable("offset", offset);
    m_matrices_dirty = true;
}

void Camera3D::SetFOV(float fov) {
    m_fov = std::clamp(fov, glm::radians(1.0f), glm::radians(179.0f));
    SetExportVariable("fov", m_fov);
    m_matrices_dirty = true;
}

void Camera3D::SetNearPlane(float near_plane) {
    m_near_plane = std::max(0.001f, near_plane);
    SetExportVariable("near_plane", m_near_plane);
    m_matrices_dirty = true;
}

void Camera3D::SetFarPlane(float far_plane) {
    m_far_plane = std::max(m_near_plane + 0.001f, far_plane);
    SetExportVariable("far_plane", m_far_plane);
    m_matrices_dirty = true;
}

void Camera3D::SetProjectionType(ProjectionType type) {
    m_projection_type = type;
    SetExportVariable("projection_type", static_cast<int>(type));
    m_matrices_dirty = true;
}

void Camera3D::SetOrthographicSize(float size) {
    m_orthographic_size = std::max(0.1f, size);
    SetExportVariable("orthographic_size", m_orthographic_size);
    m_matrices_dirty = true;
}

void Camera3D::SetFollowTarget(const std::string& target_uuid) {
    m_follow_target = target_uuid;
    SetExportVariable("follow_target", target_uuid);
}

void Camera3D::SetFollowSmoothing(float smoothing) {
    m_follow_smoothing = std::clamp(smoothing, 0.0f, 1.0f);
    SetExportVariable("follow_smoothing", m_follow_smoothing);
}

void Camera3D::SetEnabled(bool enabled) {
    m_enabled = enabled;
    SetExportVariable("enabled", enabled);
}

void Camera3D::SetCurrent(bool current) {
    m_is_current = current;
    SetExportVariable("is_current", current);
}

void Camera3D::SetFollowDistance(float distance) {
    m_follow_distance = std::max(0.1f, distance);
    SetExportVariable("follow_distance", m_follow_distance);
    m_current_zoom = m_follow_distance; // Update current zoom to match
}

void Camera3D::SetFollowHeight(float height) {
    m_follow_height = height;
    SetExportVariable("follow_height", m_follow_height);
}

void Camera3D::SetMouseOrbitEnabled(bool enabled) {
    m_mouse_orbit_enabled = enabled;
    SetExportVariable("mouse_orbit_enabled", enabled);
}

void Camera3D::SetTankOrbitEnabled(bool enabled) {
    m_tank_orbit_enabled = enabled;
    SetExportVariable("tank_orbit_enabled", enabled);
}

void Camera3D::SetScrollZoomEnabled(bool enabled) {
    m_scroll_zoom_enabled = enabled;
    SetExportVariable("scroll_zoom_enabled", enabled);
}

void Camera3D::SetMinZoom(float min_zoom) {
    m_min_zoom = std::max(0.1f, min_zoom);
    SetExportVariable("min_zoom", m_min_zoom);
    // Clamp current zoom if needed
    if (m_current_zoom < m_min_zoom) {
        m_current_zoom = m_min_zoom;
    }
}

void Camera3D::SetMaxZoom(float max_zoom) {
    m_max_zoom = std::max(m_min_zoom, max_zoom);
    SetExportVariable("max_zoom", m_max_zoom);
    // Clamp current zoom if needed
    if (m_current_zoom > m_max_zoom) {
        m_current_zoom = m_max_zoom;
    }
}

glm::mat4 Camera3D::GetFrustumMatrix() const {
    return m_camera->GetViewProjectionMatrix();
}

std::pair<glm::vec3, glm::vec3> Camera3D::ScreenToWorldRay(const glm::vec2& screen_pos, const glm::vec2& viewport_size) const {
    // Convert screen coordinates to normalized device coordinates (-1 to 1)
    glm::vec2 ndc = glm::vec2(
        (screen_pos.x / viewport_size.x) * 2.0f - 1.0f,
        1.0f - (screen_pos.y / viewport_size.y) * 2.0f
    );
    
    // Create ray in clip space
    glm::vec4 ray_clip = glm::vec4(ndc.x, ndc.y, -1.0f, 1.0f);
    
    // Transform to eye space
    glm::mat4 inv_proj = glm::inverse(m_camera->GetProjectionMatrix());
    glm::vec4 ray_eye = inv_proj * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);
    
    // Transform to world space
    glm::mat4 inv_view = glm::inverse(m_camera->GetViewMatrix());
    glm::vec4 ray_world = inv_view * ray_eye;
    
    glm::vec3 ray_direction = glm::normalize(glm::vec3(ray_world));
    glm::vec3 ray_origin = m_camera->GetPosition();
    
    return std::make_pair(ray_origin, ray_direction);
}

glm::vec3 Camera3D::WorldToScreen(const glm::vec3& world_pos, const glm::vec2& viewport_size) const {
    // Apply view and projection transforms
    glm::mat4 view_proj = m_camera->GetViewProjectionMatrix();
    glm::vec4 clip_pos = view_proj * glm::vec4(world_pos, 1.0f);
    
    // Perspective divide
    if (clip_pos.w != 0.0f) {
        clip_pos /= clip_pos.w;
    }
    
    // Convert to screen coordinates (0,0 = top-left)
    glm::vec3 screen_pos;
    screen_pos.x = (clip_pos.x + 1.0f) * 0.5f * viewport_size.x;
    screen_pos.y = (1.0f - clip_pos.y) * 0.5f * viewport_size.y;
    screen_pos.z = clip_pos.z; // Depth value
    
    return screen_pos;
}

void Camera3D::OnAwake() {
    // Component is now attached to a node
}

void Camera3D::OnReady() {
    // Node is now in the scene tree
    UpdateCamera();
}

void Camera3D::OnUpdate(float delta_time) {
    // Check if export variables have changed
    UpdateFromExportVariables();

    if (!m_enabled) return;

    // Update camera controls
    UpdateOrbitControls(delta_time);
    UpdateZoomControls(delta_time);

    UpdateFollowTarget(delta_time);
    UpdateCamera();
}

void Camera3D::InitializeExportVariables() {
    AddExportVariable("offset", m_offset, "Camera position offset from node position", ExportVariableType::Vec3);
    AddExportVariable("fov", m_fov, "Field of view in radians (perspective projection)", ExportVariableType::Float);
    AddExportVariable("near_plane", m_near_plane, "Near clipping plane distance", ExportVariableType::Float);
    AddExportVariable("far_plane", m_far_plane, "Far clipping plane distance", ExportVariableType::Float);

    // Add enum for projection type
    std::vector<std::string> projectionOptions = {"Perspective", "Orthographic"};
    AddEnumExportVariable("projection_type", static_cast<int>(m_projection_type), "Projection type", projectionOptions);

    AddExportVariable("orthographic_size", m_orthographic_size, "Orthographic size (half-height of view volume)", ExportVariableType::Float);
    AddExportVariable("follow_target", m_follow_target, "UUID of node to follow", ExportVariableType::NodeReference);
    AddExportVariable("follow_smoothing", m_follow_smoothing, "Follow smoothing factor (0.0 = instant)", ExportVariableType::Float);

    // 3rd person camera features
    AddExportVariable("follow_distance", m_follow_distance, "Distance from target for 3rd person view", ExportVariableType::Float);
    AddExportVariable("follow_height", m_follow_height, "Height offset from target for 3rd person view", ExportVariableType::Float);
    AddExportVariable("mouse_orbit_enabled", m_mouse_orbit_enabled, "Enable right mouse + movement for camera orbit", ExportVariableType::Bool);
    AddExportVariable("tank_orbit_enabled", m_tank_orbit_enabled, "Enable action-based camera orbit controls", ExportVariableType::Bool);
    AddExportVariable("scroll_zoom_enabled", m_scroll_zoom_enabled, "Enable scroll wheel zoom", ExportVariableType::Bool);
    AddExportVariable("min_zoom", m_min_zoom, "Minimum zoom distance", ExportVariableType::Float);
    AddExportVariable("max_zoom", m_max_zoom, "Maximum zoom distance", ExportVariableType::Float);

    // Tank control action names
    AddExportVariable("orbit_left_action", m_orbit_left_action, "Action name for orbit left", ExportVariableType::String);
    AddExportVariable("orbit_right_action", m_orbit_right_action, "Action name for orbit right", ExportVariableType::String);
    AddExportVariable("orbit_up_action", m_orbit_up_action, "Action name for orbit up", ExportVariableType::String);
    AddExportVariable("orbit_down_action", m_orbit_down_action, "Action name for orbit down", ExportVariableType::String);

    AddExportVariable("enabled", m_enabled, "Whether camera is enabled", ExportVariableType::Bool);
    AddExportVariable("is_current", m_is_current, "Whether camera is currently active", ExportVariableType::Bool);
}

void Camera3D::UpdateFromExportVariables() {
    // Update properties from export variables
    glm::vec3 new_offset = GetExportVariableValue<glm::vec3>("offset", glm::vec3(0.0f));
    if (new_offset != m_offset) {
        m_offset = new_offset;
        m_matrices_dirty = true;
    }

    float new_fov = GetExportVariableValue<float>("fov", glm::radians(45.0f));
    if (new_fov != m_fov) {
        m_fov = new_fov;
        m_matrices_dirty = true;
    }

    float new_near_plane = GetExportVariableValue<float>("near_plane", 0.1f);
    if (new_near_plane != m_near_plane) {
        m_near_plane = new_near_plane;
        m_matrices_dirty = true;
    }

    float new_far_plane = GetExportVariableValue<float>("far_plane", 100.0f);
    if (new_far_plane != m_far_plane) {
        m_far_plane = new_far_plane;
        m_matrices_dirty = true;
    }

    int new_projection_type = GetExportVariableValue<int>("projection_type", static_cast<int>(ProjectionType::Perspective));
    ProjectionType new_proj_type = static_cast<ProjectionType>(new_projection_type);
    if (new_proj_type != m_projection_type) {
        m_projection_type = new_proj_type;
        m_matrices_dirty = true;
    }

    float new_orthographic_size = GetExportVariableValue<float>("orthographic_size", 10.0f);
    if (new_orthographic_size != m_orthographic_size) {
        m_orthographic_size = new_orthographic_size;
        m_matrices_dirty = true;
    }

    // Handle follow_target as UUID (NodeReference type)
    UUID follow_target_uuid = GetExportVariableValue<UUID>("follow_target", UUID::Nil());
    std::string new_follow_target = follow_target_uuid.IsNil() ? "" : follow_target_uuid.ToString();
    if (new_follow_target != m_follow_target) {
        m_follow_target = new_follow_target;
    }

    float new_follow_smoothing = GetExportVariableValue<float>("follow_smoothing", 0.1f);
    if (new_follow_smoothing != m_follow_smoothing) {
        m_follow_smoothing = std::clamp(new_follow_smoothing, 0.0f, 1.0f);
    }

    bool new_enabled = GetExportVariableValue<bool>("enabled", true);
    if (new_enabled != m_enabled) {
        m_enabled = new_enabled;
    }

    bool new_is_current = GetExportVariableValue<bool>("is_current", false);
    if (new_is_current != m_is_current) {
        m_is_current = new_is_current;
    }

    // Update 3rd person camera features
    float new_follow_distance = GetExportVariableValue<float>("follow_distance", 5.0f);
    if (new_follow_distance != m_follow_distance) {
        m_follow_distance = std::max(0.1f, new_follow_distance);
        m_current_zoom = m_follow_distance; // Update current zoom to match
    }

    float new_follow_height = GetExportVariableValue<float>("follow_height", 2.0f);
    if (new_follow_height != m_follow_height) {
        m_follow_height = new_follow_height;
    }

    bool new_mouse_orbit_enabled = GetExportVariableValue<bool>("mouse_orbit_enabled", true);
    if (new_mouse_orbit_enabled != m_mouse_orbit_enabled) {
        m_mouse_orbit_enabled = new_mouse_orbit_enabled;
    }

    bool new_tank_orbit_enabled = GetExportVariableValue<bool>("tank_orbit_enabled", false);
    if (new_tank_orbit_enabled != m_tank_orbit_enabled) {
        m_tank_orbit_enabled = new_tank_orbit_enabled;
    }

    bool new_scroll_zoom_enabled = GetExportVariableValue<bool>("scroll_zoom_enabled", true);
    if (new_scroll_zoom_enabled != m_scroll_zoom_enabled) {
        m_scroll_zoom_enabled = new_scroll_zoom_enabled;
    }

    float new_min_zoom = GetExportVariableValue<float>("min_zoom", 1.0f);
    if (new_min_zoom != m_min_zoom) {
        m_min_zoom = std::max(0.1f, new_min_zoom);
        if (m_current_zoom < m_min_zoom) {
            m_current_zoom = m_min_zoom;
        }
    }

    float new_max_zoom = GetExportVariableValue<float>("max_zoom", 20.0f);
    if (new_max_zoom != m_max_zoom) {
        m_max_zoom = std::max(m_min_zoom, new_max_zoom);
        if (m_current_zoom > m_max_zoom) {
            m_current_zoom = m_max_zoom;
        }
    }

    // Update tank control action names
    std::string new_orbit_left_action = GetExportVariableValue<std::string>("orbit_left_action", "camera_orbit_left");
    if (new_orbit_left_action != m_orbit_left_action) {
        m_orbit_left_action = new_orbit_left_action;
    }

    std::string new_orbit_right_action = GetExportVariableValue<std::string>("orbit_right_action", "camera_orbit_right");
    if (new_orbit_right_action != m_orbit_right_action) {
        m_orbit_right_action = new_orbit_right_action;
    }

    std::string new_orbit_up_action = GetExportVariableValue<std::string>("orbit_up_action", "camera_orbit_up");
    if (new_orbit_up_action != m_orbit_up_action) {
        m_orbit_up_action = new_orbit_up_action;
    }

    std::string new_orbit_down_action = GetExportVariableValue<std::string>("orbit_down_action", "camera_orbit_down");
    if (new_orbit_down_action != m_orbit_down_action) {
        m_orbit_down_action = new_orbit_down_action;
    }
}

void Camera3D::UpdateCamera() {
    if (!m_camera || !GetOwner()) return;

    // Update viewport size and aspect ratio from ViewportManager
    ViewportManager::ScreenBounds bounds = ViewportManager::GetCurrentBounds();
    m_viewport_size = glm::vec2(bounds.width, bounds.height);
    m_aspect_ratio = bounds.aspect_ratio;

    // Get camera position from owner node
    glm::vec3 camera_pos(0.0f);
    glm::quat camera_rot(1.0f, 0.0f, 0.0f, 0.0f);

    if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
        camera_pos = node3d->GetGlobalPosition();
        camera_rot = node3d->GetGlobalRotation();
    }

    // Apply offset (rotated by node rotation)
    glm::vec3 rotated_offset = camera_rot * m_offset;
    camera_pos += rotated_offset;

    // Set camera position
    m_camera->SetPosition(camera_pos);

    // Only set rotation if we're not using target-based look-at
    if (!m_camera->IsUsingTarget()) {
        // Convert quaternion to Euler angles for the Camera class
        glm::vec3 euler_angles = glm::eulerAngles(camera_rot);
        m_camera->SetRotation(euler_angles);
    }

    // Update projection based on type
    if (m_projection_type == ProjectionType::Perspective) {
        m_camera->SetPerspective(m_fov, m_aspect_ratio, m_near_plane, m_far_plane);
    } else {
        float ortho_width = m_orthographic_size * m_aspect_ratio;
        float ortho_height = m_orthographic_size;
        m_camera->SetOrthographic(-ortho_width, ortho_width, -ortho_height, ortho_height, m_near_plane, m_far_plane);
    }

    m_matrices_dirty = false;
}

void Camera3D::UpdateFollowTarget(float delta_time) {
    if (m_follow_target.empty() || !GetOwner() || !GetOwner()->GetScene()) {
        // Disable target-based look-at when not following
        if (m_camera) {
            m_camera->SetUseTarget(false);
        }
        return;
    }

    // Find target node by UUID
    Node* target_node = GetOwner()->GetScene()->FindNodeByUUID(m_follow_target);
    if (!target_node) return;

    // Get target position
    glm::vec3 target_pos(0.0f);
    if (auto* target_node3d = dynamic_cast<Node3D*>(target_node)) {
        target_pos = target_node3d->GetGlobalPosition();
    }

    // Calculate 3rd person camera position based on orbit angles and zoom
    glm::vec3 desired_pos = target_pos;

    // Add height offset to the target position for look-at
    glm::vec3 look_target = target_pos + glm::vec3(0.0f, m_follow_height * 0.5f, 0.0f);

    // Calculate orbit position
    float yaw_rad = glm::radians(m_orbit_yaw);
    float pitch_rad = glm::radians(m_orbit_pitch);

    // Calculate camera offset from target based on orbit angles and current zoom
    glm::vec3 camera_offset;
    camera_offset.x = m_current_zoom * cos(pitch_rad) * sin(yaw_rad);
    camera_offset.y = m_current_zoom * sin(pitch_rad);
    camera_offset.z = m_current_zoom * cos(pitch_rad) * cos(yaw_rad);

    desired_pos = look_target + camera_offset;

    // Get current camera position
    glm::vec3 current_pos(0.0f);
    if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
        current_pos = node3d->GetGlobalPosition();
    }

    // Apply smoothing
    if (m_follow_smoothing > 0.0f) {
        float lerp_factor = 1.0f - std::pow(m_follow_smoothing, delta_time);
        glm::vec3 new_pos = glm::mix(current_pos, desired_pos, lerp_factor);

        if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
            node3d->SetPosition(new_pos);
        }
    } else {
        // Instant follow
        if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
            node3d->SetPosition(desired_pos);
        }
    }

    // Set the camera to look at the target using the Camera's built-in look-at functionality
    if (m_camera) {
        m_camera->SetTarget(look_target);
        m_camera->SetUseTarget(true);
    }

    m_matrices_dirty = true;
}

void Camera3D::UpdateOrbitControls(float delta_time) {
    // Only allow orbit controls when following a target
    if (m_follow_target.empty()) return;

    // Mouse orbit controls
    if (m_mouse_orbit_enabled && InputManager::IsMouseButtonPressed(MouseButton::Right)) {
        glm::vec2 mouse_delta = InputManager::GetMouseDelta();
        float sensitivity = 0.1f;

        // Invert left/right for more intuitive camera control
        m_orbit_yaw -= mouse_delta.x * sensitivity;
        m_orbit_pitch -= mouse_delta.y * sensitivity;

        // Clamp pitch to avoid gimbal lock
        m_orbit_pitch = glm::clamp(m_orbit_pitch, -89.0f, 89.0f);

        // Debug orbit controls
        static int orbit_debug_counter = 0;
        orbit_debug_counter++;
        if (orbit_debug_counter % 30 == 0) {
            std::cout << "Camera3D: Orbiting - Yaw: " << m_orbit_yaw << ", Pitch: " << m_orbit_pitch << std::endl;
        }
    }

    // Tank control orbit
    if (m_tank_orbit_enabled) {
        float orbit_speed = 90.0f; // degrees per second

        if (InputManager::IsActionPressed(m_orbit_left_action)) {
            m_orbit_yaw -= orbit_speed * delta_time;
        }
        if (InputManager::IsActionPressed(m_orbit_right_action)) {
            m_orbit_yaw += orbit_speed * delta_time;
        }
        if (InputManager::IsActionPressed(m_orbit_up_action)) {
            m_orbit_pitch += orbit_speed * delta_time;
        }
        if (InputManager::IsActionPressed(m_orbit_down_action)) {
            m_orbit_pitch -= orbit_speed * delta_time;
        }

        // Clamp pitch to avoid gimbal lock
        m_orbit_pitch = glm::clamp(m_orbit_pitch, -89.0f, 89.0f);
    }
}

void Camera3D::UpdateZoomControls(float delta_time) {
    if (m_scroll_zoom_enabled) {
        glm::vec2 wheel_delta = InputManager::GetMouseWheelDelta();
        if (wheel_delta.y != 0.0f) {
            float zoom_speed = 1.0f;
            m_current_zoom -= wheel_delta.y * zoom_speed;
            m_current_zoom = glm::clamp(m_current_zoom, m_min_zoom, m_max_zoom);
        }
    }
}

} // namespace Lupine
