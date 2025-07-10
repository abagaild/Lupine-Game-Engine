#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

namespace Lupine {
    class Camera;
    class Node;
    class Node2D;
    class Node3D;
    class Control;
}

enum class GizmoType {
    None,
    Move,
    Rotate,
    Scale
};

enum class GizmoAxis {
    None,
    X,
    Y,
    Z,
    XY,
    XZ,
    YZ,
    XYZ
};

struct GizmoHitResult {
    bool hit = false;
    GizmoType type = GizmoType::None;
    GizmoAxis axis = GizmoAxis::None;
    float distance = 0.0f;
    glm::vec3 hitPoint = glm::vec3(0.0f);
};

class GizmoRenderer {
public:
    GizmoRenderer();
    ~GizmoRenderer();

    void Initialize();
    void Cleanup();

    // Rendering
    void RenderGizmos(Lupine::Camera* camera, Lupine::Node* selectedNode, GizmoType activeGizmo);
    void RenderMoveGizmo(const glm::mat4& viewProjection, const glm::vec3& position, float scale);
    void RenderRotateGizmo(const glm::mat4& viewProjection, const glm::vec3& position, float scale);
    void RenderScaleGizmo(const glm::mat4& viewProjection, const glm::vec3& position, float scale);

    // Interaction
    GizmoHitResult TestGizmoHit(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                Lupine::Node* selectedNode, GizmoType activeGizmo, Lupine::Camera* camera);
    
    // Utility
    float CalculateGizmoScale(Lupine::Camera* camera, const glm::vec3& position);
    glm::vec3 GetNodeWorldPosition(Lupine::Node* node);
    glm::quat GetNodeWorldRotation(Lupine::Node* node);
    glm::vec3 GetNodeWorldScale(Lupine::Node* node);

    // Settings
    void SetGizmoSize(float size) { m_gizmoSize = size; }
    float GetGizmoSize() const { return m_gizmoSize; }
    
    void SetHighlightedAxis(GizmoAxis axis) { m_highlightedAxis = axis; }
    GizmoAxis GetHighlightedAxis() const { return m_highlightedAxis; }

private:
    // Rendering helpers
    void RenderArrow(const glm::mat4& transform, const glm::vec3& color);
    void RenderArrowHead(const glm::vec3& position, const glm::vec3& direction, float length, float radius, const glm::vec3& color);
    void RenderPlaneHandle(const glm::vec3& position, const glm::vec3& axis1, const glm::vec3& axis2, float size, const glm::vec3& color);
    void RenderLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
    void RenderCircle(const glm::mat4& transform, const glm::vec3& color, int segments = 32);
    void RenderCube(const glm::mat4& transform, const glm::vec3& color);

    // Hit testing helpers
    bool TestArrowHit(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                      const glm::vec3& arrowStart, const glm::vec3& arrowEnd,
                      float radius, float& distance);
    bool TestPlaneHit(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                      const glm::vec3& planeOrigin, const glm::vec3& axis1, const glm::vec3& axis2,
                      float size, float& distance);
    bool TestCircleHit(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                       const glm::vec3& circleCenter, const glm::vec3& circleNormal,
                       float radius, float hitRadius, float& distance);
    bool TestCubeHit(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                     const glm::vec3& cubeCenter, float cubeSize, float& distance);
    bool TestCircleHit(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                       const glm::vec3& center, const glm::vec3& normal, float radius,
                       float& distance);

    // OpenGL resources
    unsigned int m_arrowVAO, m_arrowVBO, m_arrowEBO;
    unsigned int m_circleVAO, m_circleVBO;
    unsigned int m_cubeVAO, m_cubeVBO, m_cubeEBO;
    unsigned int m_lineVAO, m_lineVBO;
    unsigned int m_shaderProgram;

    // Settings
    float m_gizmoSize;
    GizmoAxis m_highlightedAxis;

    // Current rendering state
    glm::mat4 m_currentViewProjection;

    // Colors
    glm::vec3 m_colorX;
    glm::vec3 m_colorY;
    glm::vec3 m_colorZ;
    glm::vec3 m_colorHighlight;
    glm::vec3 m_colorSelected;
};
