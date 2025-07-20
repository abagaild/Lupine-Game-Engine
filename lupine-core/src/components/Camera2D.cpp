#include "lupine/components/Camera2D.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/core/Scene.h"
#include "lupine/rendering/ViewportManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

namespace Lupine {

Camera2D::Camera2D()
    : Component("Camera2D")
    , m_camera(std::make_unique<Camera>(ProjectionType::Orthographic))
    , m_offset(0.0f, 0.0f)
    , m_zoom(1.0f)
    , m_rotation(0.0f)
    , m_follow_target("")
    , m_follow_smoothing(0.1f)
    , m_enabled(true)
    , m_is_current(false)
    , m_viewport_size(1920.0f, 1080.0f)
    , m_matrices_dirty(true) {

    std::cout << "Camera2D component created" << std::endl;

    // Initialize export variables
    InitializeExportVariables();

    // Set up default orthographic projection using viewport manager
    ViewportManager::ScreenBounds bounds = ViewportManager::GetDefaultScreenBounds();
    m_viewport_size = glm::vec2(bounds.width, bounds.height);
    float half_width = bounds.width * 0.5f;
    float half_height = bounds.height * 0.5f;
    // Standard orthographic projection: (0,0) at center, positive Y goes up
    m_camera->SetOrthographic(-half_width, half_width, -half_height, half_height, -1000.0f, 1000.0f);
}

void Camera2D::SetOffset(const glm::vec2& offset) {
    m_offset = offset;
    SetExportVariable("offset", offset);
    m_matrices_dirty = true;
}

void Camera2D::SetZoom(float zoom) {
    m_zoom = std::max(0.01f, zoom); // Prevent zero or negative zoom
    SetExportVariable("zoom", m_zoom);
    m_matrices_dirty = true;
}

void Camera2D::SetRotation(float rotation) {
    m_rotation = rotation;
    SetExportVariable("rotation", rotation);
    m_matrices_dirty = true;
}

void Camera2D::SetFollowTarget(const std::string& target_uuid) {
    m_follow_target = target_uuid;
    SetExportVariable("follow_target", target_uuid);
}

void Camera2D::SetFollowSmoothing(float smoothing) {
    m_follow_smoothing = std::clamp(smoothing, 0.0f, 1.0f);
    SetExportVariable("follow_smoothing", m_follow_smoothing);
}

void Camera2D::SetEnabled(bool enabled) {
    m_enabled = enabled;
    SetExportVariable("enabled", enabled);
}

void Camera2D::SetCurrent(bool current) {
    m_is_current = current;
    SetExportVariable("is_current", current);
}

glm::vec4 Camera2D::GetBounds() const {
    float half_width = (m_viewport_size.x * 0.5f) / m_zoom;
    float half_height = (m_viewport_size.y * 0.5f) / m_zoom;
    
    glm::vec3 camera_pos = m_camera->GetPosition();
    
    return glm::vec4(
        camera_pos.x - half_width,  // left
        camera_pos.y + half_height, // top
        camera_pos.x + half_width,  // right
        camera_pos.y - half_height  // bottom
    );
}

glm::vec2 Camera2D::ScreenToWorld(const glm::vec2& screen_pos, const glm::vec2& viewport_size) const {
    // Convert screen coordinates (0,0 = top-left) to normalized device coordinates (-1 to 1)
    glm::vec2 ndc = glm::vec2(
        (screen_pos.x / viewport_size.x) * 2.0f - 1.0f,
        1.0f - (screen_pos.y / viewport_size.y) * 2.0f
    );
    
    // Apply inverse projection and view transforms
    glm::mat4 inv_proj = glm::inverse(m_camera->GetProjectionMatrix());
    glm::mat4 inv_view = glm::inverse(m_camera->GetViewMatrix());
    
    glm::vec4 world_pos = inv_view * inv_proj * glm::vec4(ndc, 0.0f, 1.0f);
    return glm::vec2(world_pos.x, world_pos.y);
}

glm::vec2 Camera2D::WorldToScreen(const glm::vec2& world_pos, const glm::vec2& viewport_size) const {
    // Apply view and projection transforms
    glm::mat4 view_proj = m_camera->GetProjectionMatrix() * m_camera->GetViewMatrix();
    glm::vec4 clip_pos = view_proj * glm::vec4(world_pos, 0.0f, 1.0f);
    
    // Convert to normalized device coordinates
    glm::vec2 ndc = glm::vec2(clip_pos.x, clip_pos.y);
    
    // Convert to screen coordinates (0,0 = top-left)
    return glm::vec2(
        (ndc.x + 1.0f) * 0.5f * viewport_size.x,
        (1.0f - ndc.y) * 0.5f * viewport_size.y
    );
}

void Camera2D::OnAwake() {
    // Component is now attached to a node
}

void Camera2D::OnReady() {
    // Node is now in the scene tree
    UpdateCamera();
}

void Camera2D::OnUpdate(float delta_time) {
    static int update_counter = 0;
    if (update_counter % 60 == 0) { // Print every 60 frames to avoid spam
        std::cout << "Camera2D::OnUpdate called (frame " << update_counter << ")" << std::endl;
    }
    update_counter++;

    // Check if export variables have changed
    UpdateFromExportVariables();

    if (!m_enabled) {
        std::cout << "Camera2D: Disabled, skipping update" << std::endl;
        return;
    }

    UpdateFollowTarget(delta_time);
    UpdateCamera();
}

void Camera2D::InitializeExportVariables() {
    AddExportVariable("offset", m_offset, "Camera position offset from node position", ExportVariableType::Vec2);
    AddExportVariable("zoom", m_zoom, "Camera zoom level (1.0 = normal)", ExportVariableType::Float);
    AddExportVariable("rotation", m_rotation, "Camera rotation in radians", ExportVariableType::Float);
    AddExportVariable("follow_target", m_follow_target, "UUID of node to follow", ExportVariableType::NodeReference);
    AddExportVariable("follow_smoothing", m_follow_smoothing, "Follow smoothing factor (0.0 = instant)", ExportVariableType::Float);
    AddExportVariable("enabled", m_enabled, "Whether camera is enabled", ExportVariableType::Bool);
    AddExportVariable("is_current", m_is_current, "Whether camera is currently active", ExportVariableType::Bool);
}

void Camera2D::UpdateCamera() {
    if (!m_camera || !GetOwner()) return;

    // Update viewport size from ViewportManager
    ViewportManager::ScreenBounds bounds = ViewportManager::GetCurrentBounds();
    m_viewport_size = glm::vec2(bounds.width, bounds.height);

    // Get camera position from owner node
    glm::vec2 camera_pos(0.0f);
    if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
        camera_pos = node2d->GetGlobalPosition();
    }

    // Apply offset
    camera_pos += m_offset;

    // Set camera position (Z = 0 for 2D)
    m_camera->SetPosition(glm::vec3(camera_pos.x, camera_pos.y, 0.0f));

    // Update orthographic projection with zoom
    float half_width = (m_viewport_size.x * 0.5f) / m_zoom;
    float half_height = (m_viewport_size.y * 0.5f) / m_zoom;



    // Set up orthographic projection with proper coordinate system
    // In OpenGL, bottom must be < top, so we use -half_height for bottom and +half_height for top
    // This creates a coordinate system where (0,0) is at the center, positive Y goes up
    m_camera->SetOrthographic(
        -half_width, half_width,
        -half_height, half_height,  // Standard: bottom=-half_height, top=+half_height
        -1000.0f, 1000.0f
    );

    
    // Apply rotation if needed
    if (m_rotation != 0.0f) {
        m_camera->SetRotation(glm::vec3(0.0f, 0.0f, m_rotation));
    }
    
    m_matrices_dirty = false;
}

void Camera2D::UpdateFromExportVariables() {
    // Update properties from export variables
    glm::vec2 new_offset = GetExportVariableValue<glm::vec2>("offset", glm::vec2(0.0f));
    if (new_offset != m_offset) {
        m_offset = new_offset;
        m_matrices_dirty = true;
    }

    float new_zoom = GetExportVariableValue<float>("zoom", 1.0f);
    if (new_zoom != m_zoom) {
        m_zoom = std::max(0.01f, new_zoom);
        m_matrices_dirty = true;
    }

    float new_rotation = GetExportVariableValue<float>("rotation", 0.0f);
    if (new_rotation != m_rotation) {
        m_rotation = new_rotation;
        m_matrices_dirty = true;
    }

    // Handle follow_target as UUID (NodeReference type)
    UUID follow_target_uuid = GetExportVariableValue<UUID>("follow_target", UUID::Nil());
    std::string new_follow_target = follow_target_uuid.IsNil() ? "" : follow_target_uuid.ToString();

    // Debug: Print what we're reading from export variables
    static int debug_counter = 0;
    if (debug_counter % 60 == 0) { // Print every 60 frames to avoid spam
        std::cout << "Camera2D: Reading follow_target from export variables: '" << new_follow_target << "'" << std::endl;
        std::cout << "Camera2D: Current m_follow_target: '" << m_follow_target << "'" << std::endl;
    }
    debug_counter++;

    if (new_follow_target != m_follow_target) {
        std::cout << "Camera2D: Follow target changed from '" << m_follow_target << "' to '" << new_follow_target << "'" << std::endl;
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
}

void Camera2D::UpdateFollowTarget(float delta_time) {
    if (m_follow_target.empty()) {
        std::cout << "Camera2D: Follow target is empty" << std::endl;
        return;
    }

    if (!GetOwner()) {
        std::cout << "Camera2D: No owner node" << std::endl;
        return;
    }

    if (!GetOwner()->GetScene()) {
        std::cout << "Camera2D: Owner has no scene" << std::endl;
        return;
    }

    std::cout << "Camera2D: Looking for target UUID: " << m_follow_target << std::endl;

    // Find target node by UUID
    Node* target_node = GetOwner()->GetScene()->FindNodeByUUID(m_follow_target);
    if (!target_node) {
        std::cout << "Camera2D: Target node not found for UUID: " << m_follow_target << std::endl;
        return;
    }

    std::cout << "Camera2D: Found target node: " << target_node->GetName() << std::endl;

    // Get target position
    glm::vec2 target_pos(0.0f);
    if (auto* target_node2d = dynamic_cast<Node2D*>(target_node)) {
        target_pos = target_node2d->GetGlobalPosition();
        std::cout << "Camera2D: Target position: (" << target_pos.x << ", " << target_pos.y << ")" << std::endl;
    } else {
        std::cout << "Camera2D: Target node is not Node2D" << std::endl;
        return;
    }

    // Get current camera position
    glm::vec2 current_pos(0.0f);
    if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
        current_pos = node2d->GetGlobalPosition();
        std::cout << "Camera2D: Current position: (" << current_pos.x << ", " << current_pos.y << ")" << std::endl;
    } else {
        std::cout << "Camera2D: Camera owner is not Node2D" << std::endl;
        return;
    }

    // Apply smoothing
    if (m_follow_smoothing > 0.0f) {
        float lerp_factor = 1.0f - std::pow(m_follow_smoothing, delta_time);
        glm::vec2 new_pos = glm::mix(current_pos, target_pos, lerp_factor);

        std::cout << "Camera2D: Moving to position: (" << new_pos.x << ", " << new_pos.y << ") with smoothing" << std::endl;

        if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
            node2d->SetPosition(new_pos);
        }
    } else {
        // Instant follow
        std::cout << "Camera2D: Moving to position: (" << target_pos.x << ", " << target_pos.y << ") instantly" << std::endl;

        if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
            node2d->SetPosition(target_pos);
        }
    }

    m_matrices_dirty = true;
}

} // namespace Lupine
