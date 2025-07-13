#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Lupine {

/**
 * @brief Camera projection types
 */
enum class ProjectionType {
    Perspective,
    Orthographic
};

/**
 * @brief Camera class for 3D and 2D rendering
 */
class Camera {
public:
    /**
     * @brief Constructor
     * @param projection_type Camera projection type
     */
    explicit Camera(ProjectionType projection_type = ProjectionType::Perspective);

    /**
     * @brief Destructor
     */
    ~Camera() = default;

    /**
     * @brief Set camera position
     * @param position New position
     */
    void SetPosition(const glm::vec3& position);

    /**
     * @brief Get camera position
     * @return Camera position
     */
    const glm::vec3& GetPosition() const { return m_position; }

    /**
     * @brief Set camera rotation (Euler angles in radians)
     * @param rotation Rotation angles (pitch, yaw, roll)
     */
    void SetRotation(const glm::vec3& rotation);

    /**
     * @brief Get camera rotation
     * @return Camera rotation
     */
    const glm::vec3& GetRotation() const { return m_rotation; }

    /**
     * @brief Set camera target (look at)
     * @param target Target position
     */
    void SetTarget(const glm::vec3& target);

    /**
     * @brief Set perspective projection parameters
     * @param fov Field of view in radians
     * @param aspect_ratio Aspect ratio (width/height)
     * @param near_plane Near clipping plane
     * @param far_plane Far clipping plane
     */
    void SetPerspective(float fov, float aspect_ratio, float near_plane, float far_plane);

    /**
     * @brief Set orthographic projection parameters
     * @param left Left boundary
     * @param right Right boundary
     * @param bottom Bottom boundary
     * @param top Top boundary
     * @param near_plane Near clipping plane
     * @param far_plane Far clipping plane
     */
    void SetOrthographic(float left, float right, float bottom, float top, float near_plane, float far_plane);

    /**
     * @brief Get view matrix
     * @return View matrix
     */
    const glm::mat4& GetViewMatrix() const { return m_view_matrix; }

    /**
     * @brief Get projection matrix
     * @return Projection matrix
     */
    const glm::mat4& GetProjectionMatrix() const { return m_projection_matrix; }

    /**
     * @brief Get view-projection matrix
     * @return Combined view-projection matrix
     */
    glm::mat4 GetViewProjectionMatrix() const { return m_projection_matrix * m_view_matrix; }

    /**
     * @brief Get camera forward vector
     * @return Forward vector
     */
    glm::vec3 GetForward() const;

    /**
     * @brief Get camera right vector
     * @return Right vector
     */
    glm::vec3 GetRight() const;

    /**
     * @brief Get camera up vector
     * @return Up vector
     */
    glm::vec3 GetUp() const;

    /**
     * @brief Update camera matrices
     */
    void UpdateMatrices();

    /**
     * @brief Get projection type
     * @return Projection type
     */
    ProjectionType GetProjectionType() const { return m_projection_type; }

    /**
     * @brief Set projection type
     * @param type New projection type
     */
    void SetProjectionType(ProjectionType type) { m_projection_type = type; UpdateMatrices(); }

    /**
     * @brief Get orthographic bounds
     * @param left Left boundary
     * @param right Right boundary
     * @param bottom Bottom boundary
     * @param top Top boundary
     * @param near_plane Near clipping plane
     * @param far_plane Far clipping plane
     */
    void GetOrthographicBounds(float& left, float& right, float& bottom, float& top, float& near_plane, float& far_plane) const {
        left = m_left;
        right = m_right;
        bottom = m_bottom;
        top = m_top;
        near_plane = m_near_plane;
        far_plane = m_far_plane;
    }

    /**
     * @brief Set whether to use target-based look-at
     * @param use_target True to use target for look-at, false to use rotation
     */
    void SetUseTarget(bool use_target) { m_use_target = use_target; m_matrices_dirty = true; }

    /**
     * @brief Check if camera is using target-based look-at
     * @return True if using target for look-at
     */
    bool IsUsingTarget() const { return m_use_target; }

private:
    ProjectionType m_projection_type;
    glm::vec3 m_position;
    glm::vec3 m_rotation;
    glm::vec3 m_target;
    glm::vec3 m_up;

    // Projection parameters
    float m_fov;
    float m_aspect_ratio;
    float m_near_plane;
    float m_far_plane;

    // Orthographic parameters
    float m_left, m_right, m_bottom, m_top;

    // Matrices
    glm::mat4 m_view_matrix;
    glm::mat4 m_projection_matrix;

    bool m_use_target;
    bool m_matrices_dirty;

    /**
     * @brief Update view matrix
     */
    void UpdateViewMatrix();

    /**
     * @brief Update projection matrix
     */
    void UpdateProjectionMatrix();
};

} // namespace Lupine