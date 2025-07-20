#include "lupine/rendering/Camera.h"
#include <cmath>

namespace Lupine {

Camera::Camera(ProjectionType projection_type)
    : m_projection_type(projection_type)
    , m_position(0.0f, 0.0f, 3.0f)
    , m_rotation(0.0f, 0.0f, 0.0f)
    , m_target(0.0f, 0.0f, 0.0f)
    , m_up(0.0f, 1.0f, 0.0f)
    , m_fov(glm::radians(45.0f))
    , m_aspect_ratio(16.0f / 9.0f)
    , m_near_plane(0.1f)
    , m_far_plane(100.0f)
    , m_left(-10.0f)
    , m_right(10.0f)
    , m_bottom(-10.0f)
    , m_top(10.0f)
    , m_view_matrix(1.0f)
    , m_projection_matrix(1.0f)
    , m_use_target(false)
    , m_matrices_dirty(true) {
    UpdateMatrices();
}

void Camera::SetPosition(const glm::vec3& position) {
    m_position = position;
    m_matrices_dirty = true;
}

void Camera::SetRotation(const glm::vec3& rotation) {
    m_rotation = rotation;
    m_use_target = false;
    m_matrices_dirty = true;
}

void Camera::SetTarget(const glm::vec3& target) {
    m_target = target;
    m_use_target = true;
    m_matrices_dirty = true;
}

void Camera::SetPerspective(float fov, float aspect_ratio, float near_plane, float far_plane) {
    m_fov = fov;
    m_aspect_ratio = aspect_ratio;
    m_near_plane = near_plane;
    m_far_plane = far_plane;
    m_projection_type = ProjectionType::Perspective;
    m_matrices_dirty = true;
}

void Camera::SetOrthographic(float left, float right, float bottom, float top, float near_plane, float far_plane) {
    m_left = left;
    m_right = right;
    m_bottom = bottom;
    m_top = top;
    m_near_plane = near_plane;
    m_far_plane = far_plane;
    m_projection_type = ProjectionType::Orthographic;
    m_matrices_dirty = true;
}

glm::vec3 Camera::GetForward() const {
    if (m_use_target) {
        return glm::normalize(m_target - m_position);
    } else {
        // Calculate forward vector from rotation
        glm::mat4 rotation_matrix = glm::mat4(1.0f);
        rotation_matrix = glm::rotate(rotation_matrix, m_rotation.y, glm::vec3(0, 1, 0)); // Yaw
        rotation_matrix = glm::rotate(rotation_matrix, m_rotation.x, glm::vec3(1, 0, 0)); // Pitch
        rotation_matrix = glm::rotate(rotation_matrix, m_rotation.z, glm::vec3(0, 0, 1)); // Roll

        return glm::vec3(rotation_matrix * glm::vec4(0, 0, -1, 0));
    }
}

glm::vec3 Camera::GetRight() const {
    return glm::normalize(glm::cross(GetForward(), m_up));
}

glm::vec3 Camera::GetUp() const {
    return glm::normalize(glm::cross(GetRight(), GetForward()));
}

void Camera::UpdateMatrices() {
    if (!m_matrices_dirty) return;

    UpdateViewMatrix();
    UpdateProjectionMatrix();

    m_matrices_dirty = false;
}

void Camera::UpdateViewMatrix() {
    if (m_use_target) {
        m_view_matrix = glm::lookAt(m_position, m_target, m_up);
    } else {
        glm::vec3 forward = GetForward();
        m_view_matrix = glm::lookAt(m_position, m_position + forward, m_up);
    }
}

void Camera::UpdateProjectionMatrix() {
    if (m_projection_type == ProjectionType::Perspective) {
        m_projection_matrix = glm::perspective(m_fov, m_aspect_ratio, m_near_plane, m_far_plane);
    } else {
        m_projection_matrix = glm::ortho(m_left, m_right, m_bottom, m_top, m_near_plane, m_far_plane);
    }
}

} // namespace Lupine