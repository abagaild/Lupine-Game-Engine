#include <glad/glad.h>
#include "GizmoRenderer.h"
#include "lupine/rendering/Camera.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Simple vertex shader for gizmo rendering
const char* gizmoVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 uMVP;
uniform vec3 uColor;

out vec3 fragColor;

void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
    fragColor = uColor;
}
)";

// Simple fragment shader for gizmo rendering
const char* gizmoFragmentShader = R"(
#version 330 core
in vec3 fragColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(fragColor, 1.0);
}
)";

GizmoRenderer::GizmoRenderer()
    : m_arrowVAO(0), m_arrowVBO(0), m_arrowEBO(0)
    , m_circleVAO(0), m_circleVBO(0)
    , m_cubeVAO(0), m_cubeVBO(0), m_cubeEBO(0)
    , m_lineVAO(0), m_lineVBO(0)
    , m_shaderProgram(0)
    , m_gizmoSize(1.0f)
    , m_highlightedAxis(GizmoAxis::None)
    , m_colorX(1.0f, 0.0f, 0.0f)      // Red for X
    , m_colorY(0.0f, 1.0f, 0.0f)      // Green for Y
    , m_colorZ(0.0f, 0.0f, 1.0f)      // Blue for Z
    , m_colorHighlight(1.0f, 1.0f, 0.0f)  // Yellow for highlight
    , m_colorSelected(1.0f, 0.5f, 0.0f)   // Orange for selected
{
}

GizmoRenderer::~GizmoRenderer()
{
    Cleanup();
}

void GizmoRenderer::Initialize()
{
    // Create and compile shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &gizmoVertexShader, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &gizmoFragmentShader, NULL);
    glCompileShader(fragmentShader);

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Create arrow geometry (for move gizmo)
    float arrowVertices[] = {
        // Arrow shaft (cylinder)
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.8f,
        // Arrow head (cone)
        0.0f, 0.0f, 0.8f,
        0.0f, 0.0f, 1.0f,
    };

    glGenVertexArrays(1, &m_arrowVAO);
    glGenBuffers(1, &m_arrowVBO);
    glBindVertexArray(m_arrowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_arrowVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(arrowVertices), arrowVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Create line geometry
    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);
    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void GizmoRenderer::Cleanup()
{
    if (m_arrowVAO) {
        glDeleteVertexArrays(1, &m_arrowVAO);
        glDeleteBuffers(1, &m_arrowVBO);
        if (m_arrowEBO) glDeleteBuffers(1, &m_arrowEBO);
    }
    if (m_circleVAO) {
        glDeleteVertexArrays(1, &m_circleVAO);
        glDeleteBuffers(1, &m_circleVBO);
    }
    if (m_cubeVAO) {
        glDeleteVertexArrays(1, &m_cubeVAO);
        glDeleteBuffers(1, &m_cubeVBO);
        if (m_cubeEBO) glDeleteBuffers(1, &m_cubeEBO);
    }
    if (m_lineVAO) {
        glDeleteVertexArrays(1, &m_lineVAO);
        glDeleteBuffers(1, &m_lineVBO);
    }
    if (m_shaderProgram) {
        glDeleteProgram(m_shaderProgram);
    }
}

void GizmoRenderer::RenderGizmos(Lupine::Camera* camera, Lupine::Node* selectedNode, GizmoType activeGizmo)
{
    if (!camera || !selectedNode || activeGizmo == GizmoType::None) {
        return;
    }

    glm::vec3 nodePosition = GetNodeWorldPosition(selectedNode);
    float gizmoScale = CalculateGizmoScale(camera, nodePosition);
    glm::mat4 viewProjection = camera->GetViewProjectionMatrix();

    // Store the view-projection matrix for use in rendering functions
    m_currentViewProjection = viewProjection;

    // Enable depth testing but disable depth writing for gizmos
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Enable line smoothing
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(2.0f);

    glUseProgram(m_shaderProgram);

    switch (activeGizmo) {
        case GizmoType::Move:
            RenderMoveGizmo(viewProjection, nodePosition, gizmoScale);
            break;
        case GizmoType::Rotate:
            RenderRotateGizmo(viewProjection, nodePosition, gizmoScale);
            break;
        case GizmoType::Scale:
            RenderScaleGizmo(viewProjection, nodePosition, gizmoScale);
            break;
        default:
            break;
    }

    // Restore depth writing
    glDepthMask(GL_TRUE);
    glDisable(GL_LINE_SMOOTH);
    glLineWidth(1.0f);
}

void GizmoRenderer::RenderMoveGizmo(const glm::mat4& viewProjection, const glm::vec3& position, float scale)
{
    // Enhanced move gizmo with larger arrows and better visibility
    float arrowLength = scale * 1.0f;
    float arrowHeadLength = scale * 0.25f;
    float arrowHeadRadius = scale * 0.08f;
    float planeSize = scale * 0.25f;

    // Render X axis (red) - enhanced arrow
    glm::vec3 colorX = (m_highlightedAxis == GizmoAxis::X) ? m_colorHighlight : m_colorX;
    RenderLine(position, position + glm::vec3(arrowLength, 0, 0), colorX);
    RenderArrowHead(position + glm::vec3(arrowLength, 0, 0), glm::vec3(1, 0, 0), arrowHeadLength, arrowHeadRadius, colorX);

    // Render Y axis (green) - enhanced arrow
    glm::vec3 colorY = (m_highlightedAxis == GizmoAxis::Y) ? m_colorHighlight : m_colorY;
    RenderLine(position, position + glm::vec3(0, arrowLength, 0), colorY);
    RenderArrowHead(position + glm::vec3(0, arrowLength, 0), glm::vec3(0, 1, 0), arrowHeadLength, arrowHeadRadius, colorY);

    // Render Z axis (blue) - enhanced arrow
    glm::vec3 colorZ = (m_highlightedAxis == GizmoAxis::Z) ? m_colorHighlight : m_colorZ;
    RenderLine(position, position + glm::vec3(0, 0, arrowLength), colorZ);
    RenderArrowHead(position + glm::vec3(0, 0, arrowLength), glm::vec3(0, 0, 1), arrowHeadLength, arrowHeadRadius, colorZ);

    // Render plane handles for multi-axis movement - smaller and distinct colors
    glm::vec3 colorXY = (m_highlightedAxis == GizmoAxis::XY) ? m_colorHighlight : glm::vec3(1.0f, 1.0f, 0.0f);
    glm::vec3 colorXZ = (m_highlightedAxis == GizmoAxis::XZ) ? m_colorHighlight : glm::vec3(1.0f, 0.0f, 1.0f);
    glm::vec3 colorYZ = (m_highlightedAxis == GizmoAxis::YZ) ? m_colorHighlight : glm::vec3(0.0f, 1.0f, 1.0f);

    RenderPlaneHandle(position, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), planeSize, colorXY);
    RenderPlaneHandle(position, glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), planeSize, colorXZ);
    RenderPlaneHandle(position, glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), planeSize, colorYZ);
}

void GizmoRenderer::RenderRotateGizmo(const glm::mat4& viewProjection, const glm::vec3& position, float scale)
{
    // Enhanced rotate gizmo with thicker circles and better visibility
    float circleRadius = scale * 0.9f;
    int segments = 80; // More segments for smoother circles

    // Set thicker lines for rotate gizmo
    glLineWidth(4.0f);

    // Render X rotation circle (red) - YZ plane
    glm::vec3 colorX = (m_highlightedAxis == GizmoAxis::X) ? m_colorHighlight : m_colorX;
    glm::mat4 transformX = glm::translate(glm::mat4(1.0f), position);
    transformX = glm::rotate(transformX, glm::radians(90.0f), glm::vec3(0, 1, 0)); // Rotate to YZ plane
    transformX = glm::scale(transformX, glm::vec3(circleRadius));
    RenderCircle(transformX, colorX, segments);

    // Render Y rotation circle (green) - XZ plane
    glm::vec3 colorY = (m_highlightedAxis == GizmoAxis::Y) ? m_colorHighlight : m_colorY;
    glm::mat4 transformY = glm::translate(glm::mat4(1.0f), position);
    transformY = glm::rotate(transformY, glm::radians(90.0f), glm::vec3(1, 0, 0)); // Rotate to XZ plane
    transformY = glm::scale(transformY, glm::vec3(circleRadius));
    RenderCircle(transformY, colorY, segments);

    // Render Z rotation circle (blue) - XY plane
    glm::vec3 colorZ = (m_highlightedAxis == GizmoAxis::Z) ? m_colorHighlight : m_colorZ;
    glm::mat4 transformZ = glm::translate(glm::mat4(1.0f), position);
    transformZ = glm::scale(transformZ, glm::vec3(circleRadius));
    RenderCircle(transformZ, colorZ, segments);

    // Add rotation indicators (small arrows on circles)
    float indicatorSize = scale * 0.1f;

    // X axis indicator
    glm::vec3 xIndicatorPos = position + glm::vec3(0, circleRadius * 0.7f, 0);
    RenderArrowHead(xIndicatorPos, glm::vec3(0, 0, 1), indicatorSize, indicatorSize * 0.5f, colorX);

    // Y axis indicator
    glm::vec3 yIndicatorPos = position + glm::vec3(circleRadius * 0.7f, 0, 0);
    RenderArrowHead(yIndicatorPos, glm::vec3(0, 0, 1), indicatorSize, indicatorSize * 0.5f, colorY);

    // Z axis indicator
    glm::vec3 zIndicatorPos = position + glm::vec3(circleRadius * 0.7f, 0, 0);
    RenderArrowHead(zIndicatorPos, glm::vec3(0, 1, 0), indicatorSize, indicatorSize * 0.5f, colorZ);

    // Reset line width
    glLineWidth(2.0f);
}

void GizmoRenderer::RenderScaleGizmo(const glm::mat4& viewProjection, const glm::vec3& position, float scale)
{
    // Enhanced scale gizmo with larger cubes and better visibility
    float lineLength = scale * 0.8f;
    float cubeSize = scale * 0.12f;
    float centerCubeSize = scale * 0.15f;

    // Set thicker lines for scale gizmo
    glLineWidth(3.0f);

    // Render X axis (red) with larger cube
    glm::vec3 colorX = (m_highlightedAxis == GizmoAxis::X) ? m_colorHighlight : m_colorX;
    RenderLine(position, position + glm::vec3(lineLength, 0, 0), colorX);
    glm::mat4 transformX = glm::translate(glm::mat4(1.0f), position + glm::vec3(lineLength, 0, 0));
    transformX = glm::scale(transformX, glm::vec3(cubeSize));
    RenderCube(transformX, colorX);

    // Render Y axis (green) with larger cube
    glm::vec3 colorY = (m_highlightedAxis == GizmoAxis::Y) ? m_colorHighlight : m_colorY;
    RenderLine(position, position + glm::vec3(0, lineLength, 0), colorY);
    glm::mat4 transformY = glm::translate(glm::mat4(1.0f), position + glm::vec3(0, lineLength, 0));
    transformY = glm::scale(transformY, glm::vec3(cubeSize));
    RenderCube(transformY, colorY);

    // Render Z axis (blue) with larger cube
    glm::vec3 colorZ = (m_highlightedAxis == GizmoAxis::Z) ? m_colorHighlight : m_colorZ;
    RenderLine(position, position + glm::vec3(0, 0, lineLength), colorZ);
    glm::mat4 transformZ = glm::translate(glm::mat4(1.0f), position + glm::vec3(0, 0, lineLength));
    transformZ = glm::scale(transformZ, glm::vec3(cubeSize));
    RenderCube(transformZ, colorZ);

    // Render center cube for uniform scaling - larger and more prominent
    glm::vec3 colorCenter = (m_highlightedAxis == GizmoAxis::XYZ) ? m_colorHighlight : glm::vec3(0.9f, 0.9f, 0.9f);
    glm::mat4 transformCenter = glm::translate(glm::mat4(1.0f), position);
    transformCenter = glm::scale(transformCenter, glm::vec3(centerCubeSize));
    RenderCube(transformCenter, colorCenter);

    // Add corner cubes for multi-axis scaling
    float cornerCubeSize = scale * 0.08f;
    float cornerOffset = lineLength * 0.3f;

    // XY corner
    if (m_highlightedAxis == GizmoAxis::XY) {
        glm::vec3 colorXY = m_colorHighlight;
        glm::mat4 transformXY = glm::translate(glm::mat4(1.0f), position + glm::vec3(cornerOffset, cornerOffset, 0));
        transformXY = glm::scale(transformXY, glm::vec3(cornerCubeSize));
        RenderCube(transformXY, colorXY);
    }

    // XZ corner
    if (m_highlightedAxis == GizmoAxis::XZ) {
        glm::vec3 colorXZ = m_colorHighlight;
        glm::mat4 transformXZ = glm::translate(glm::mat4(1.0f), position + glm::vec3(cornerOffset, 0, cornerOffset));
        transformXZ = glm::scale(transformXZ, glm::vec3(cornerCubeSize));
        RenderCube(transformXZ, colorXZ);
    }

    // YZ corner
    if (m_highlightedAxis == GizmoAxis::YZ) {
        glm::vec3 colorYZ = m_colorHighlight;
        glm::mat4 transformYZ = glm::translate(glm::mat4(1.0f), position + glm::vec3(0, cornerOffset, cornerOffset));
        transformYZ = glm::scale(transformYZ, glm::vec3(cornerCubeSize));
        RenderCube(transformYZ, colorYZ);
    }

    // Reset line width
    glLineWidth(2.0f);
}

void GizmoRenderer::RenderArrowHead(const glm::vec3& position, const glm::vec3& direction, float length, float radius, const glm::vec3& color)
{
    // Create a simple cone for the arrow head
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);

    // Align the cone with the direction vector
    glm::vec3 up = glm::vec3(0, 1, 0);
    if (glm::abs(glm::dot(direction, up)) > 0.99f) {
        up = glm::vec3(1, 0, 0); // Use different up vector if direction is parallel to Y
    }

    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    up = glm::normalize(glm::cross(right, direction));

    glm::mat3 rotation = glm::mat3(right, up, -direction);
    transform = transform * glm::mat4(rotation);
    transform = glm::scale(transform, glm::vec3(radius, radius, length));

    // Use the cube geometry as a simple arrow head for now
    RenderCube(transform, color);
}

void GizmoRenderer::RenderPlaneHandle(const glm::vec3& position, const glm::vec3& axis1, const glm::vec3& axis2, float size, const glm::vec3& color)
{
    // Render a small square plane handle
    glm::vec3 corner1 = position + axis1 * size * 0.2f + axis2 * size * 0.2f;
    glm::vec3 corner2 = position + axis1 * size + axis2 * size * 0.2f;
    glm::vec3 corner3 = position + axis1 * size + axis2 * size;
    glm::vec3 corner4 = position + axis1 * size * 0.2f + axis2 * size;

    // Render as lines forming a square
    RenderLine(corner1, corner2, color);
    RenderLine(corner2, corner3, color);
    RenderLine(corner3, corner4, color);
    RenderLine(corner4, corner1, color);
}

void GizmoRenderer::RenderLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color)
{
    float vertices[] = {
        start.x, start.y, start.z,
        end.x, end.y, end.z
    };

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    // Set uniforms - use the current view-projection matrix
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uMVP"), 1, GL_FALSE, glm::value_ptr(m_currentViewProjection));
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "uColor"), 1, glm::value_ptr(color));

    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

float GizmoRenderer::CalculateGizmoScale(Lupine::Camera* camera, const glm::vec3& position)
{
    // Fixed screen-space size for gizmos - they should not scale with zoom
    const float FIXED_GIZMO_SIZE = 80.0f; // Size in pixels

    if (camera->GetProjectionType() == Lupine::ProjectionType::Orthographic) {
        // For orthographic projection (2D view), calculate world-space size from screen-space size
        float left, right, bottom, top, near_plane, far_plane;
        camera->GetOrthographicBounds(left, right, bottom, top, near_plane, far_plane);

        // Calculate world units per pixel
        float ortho_width = right - left;
        // Assume viewport width of 1920 pixels (this could be made dynamic)
        float world_units_per_pixel = ortho_width / 1920.0f;

        // Convert fixed pixel size to world space
        float world_space_size = FIXED_GIZMO_SIZE * world_units_per_pixel;
        return world_space_size * m_gizmoSize;
    }

    // For perspective projection (3D view), calculate screen-space scale
    glm::vec4 projected = camera->GetViewProjectionMatrix() * glm::vec4(position, 1.0f);

    // Avoid division by zero or negative w
    if (projected.w <= 0.0f) {
        return 50.0f * m_gizmoSize; // Default fallback scale
    }

    // Calculate distance from camera to maintain constant screen size
    float distance_to_camera = glm::length(camera->GetPosition() - position);

    // Use a fixed screen-space size calculation
    // This ensures gizmos appear the same size regardless of camera distance
    float screen_scale = distance_to_camera * 0.05f; // Adjust this factor to change gizmo size
    return screen_scale * m_gizmoSize;
}

glm::vec3 GizmoRenderer::GetNodeWorldPosition(Lupine::Node* node)
{
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node)) {
        glm::vec2 pos2d = node2d->GetGlobalPosition();
        return glm::vec3(pos2d.x, pos2d.y, 0.0f);
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node)) {
        return node3d->GetGlobalPosition();
    } else if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
        glm::vec2 pos2d = control->GetPosition();
        return glm::vec3(pos2d.x, pos2d.y, 0.0f);
    }
    return glm::vec3(0.0f);
}

glm::quat GizmoRenderer::GetNodeWorldRotation(Lupine::Node* node)
{
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node)) {
        float rotation = node2d->GetGlobalRotation();
        return glm::angleAxis(rotation, glm::vec3(0, 0, 1));
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node)) {
        return node3d->GetGlobalRotation();
    }
    return glm::quat(1, 0, 0, 0); // Identity quaternion
}

glm::vec3 GizmoRenderer::GetNodeWorldScale(Lupine::Node* node)
{
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node)) {
        glm::vec2 scale2d = node2d->GetGlobalScale();
        return glm::vec3(scale2d.x, scale2d.y, 1.0f);
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node)) {
        return node3d->GetGlobalScale();
    } else if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
        glm::vec2 size = control->GetSize();
        return glm::vec3(size.x, size.y, 1.0f);
    }
    return glm::vec3(1.0f);
}

GizmoHitResult GizmoRenderer::TestGizmoHit(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                           Lupine::Node* selectedNode, GizmoType activeGizmo, Lupine::Camera* camera)
{
    GizmoHitResult result;

    if (!selectedNode || activeGizmo == GizmoType::None || !camera) {
        return result;
    }

    glm::vec3 nodePosition = GetNodeWorldPosition(selectedNode);
    float gizmoScale = CalculateGizmoScale(camera, nodePosition);
    float hitRadius = 0.15f * gizmoScale; // Larger hit radius for better usability

    if (activeGizmo == GizmoType::Move) {
        float distance;

        // Test plane handles first (they have priority for easier multi-axis movement)
        float planeSize = gizmoScale * 0.25f;

        // Test XY plane
        if (TestPlaneHit(rayOrigin, rayDirection, nodePosition, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), planeSize, distance)) {
            result.hit = true;
            result.type = GizmoType::Move;
            result.axis = GizmoAxis::XY;
            result.distance = distance;
            return result;
        }

        // Test XZ plane
        if (TestPlaneHit(rayOrigin, rayDirection, nodePosition, glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), planeSize, distance)) {
            result.hit = true;
            result.type = GizmoType::Move;
            result.axis = GizmoAxis::XZ;
            result.distance = distance;
            return result;
        }

        // Test YZ plane
        if (TestPlaneHit(rayOrigin, rayDirection, nodePosition, glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), planeSize, distance)) {
            result.hit = true;
            result.type = GizmoType::Move;
            result.axis = GizmoAxis::YZ;
            result.distance = distance;
            return result;
        }

        // Test individual axes
        // Test X axis
        if (TestArrowHit(rayOrigin, rayDirection, nodePosition,
                        nodePosition + glm::vec3(gizmoScale * 1.0f, 0, 0), hitRadius, distance)) {
            result.hit = true;
            result.type = GizmoType::Move;
            result.axis = GizmoAxis::X;
            result.distance = distance;
            return result;
        }

        // Test Y axis
        if (TestArrowHit(rayOrigin, rayDirection, nodePosition,
                        nodePosition + glm::vec3(0, gizmoScale * 1.0f, 0), hitRadius, distance)) {
            result.hit = true;
            result.type = GizmoType::Move;
            result.axis = GizmoAxis::Y;
            result.distance = distance;
            return result;
        }

        // Test Z axis
        if (TestArrowHit(rayOrigin, rayDirection, nodePosition,
                        nodePosition + glm::vec3(0, 0, gizmoScale * 1.0f), hitRadius, distance)) {
            result.hit = true;
            result.type = GizmoType::Move;
            result.axis = GizmoAxis::Z;
            result.distance = distance;
            return result;
        }
    }
    else if (activeGizmo == GizmoType::Rotate) {
        float distance;
        float circleRadius = gizmoScale * 0.9f;
        float circleHitRadius = hitRadius * 1.5f; // Larger hit radius for circles

        // Test X rotation circle (YZ plane)
        if (TestCircleHit(rayOrigin, rayDirection, nodePosition, glm::vec3(1, 0, 0), circleRadius, circleHitRadius, distance)) {
            result.hit = true;
            result.type = GizmoType::Rotate;
            result.axis = GizmoAxis::X;
            result.distance = distance;
            return result;
        }

        // Test Y rotation circle (XZ plane)
        if (TestCircleHit(rayOrigin, rayDirection, nodePosition, glm::vec3(0, 1, 0), circleRadius, circleHitRadius, distance)) {
            result.hit = true;
            result.type = GizmoType::Rotate;
            result.axis = GizmoAxis::Y;
            result.distance = distance;
            return result;
        }

        // Test Z rotation circle (XY plane)
        if (TestCircleHit(rayOrigin, rayDirection, nodePosition, glm::vec3(0, 0, 1), circleRadius, circleHitRadius, distance)) {
            result.hit = true;
            result.type = GizmoType::Rotate;
            result.axis = GizmoAxis::Z;
            result.distance = distance;
            return result;
        }
    }
    else if (activeGizmo == GizmoType::Scale) {
        float distance;
        float cubeSize = gizmoScale * 0.12f;
        float cubeHitRadius = hitRadius * 2.0f; // Larger hit radius for cubes

        // Test center cube for uniform scaling
        if (TestCubeHit(rayOrigin, rayDirection, nodePosition, gizmoScale * 0.15f, distance)) {
            result.hit = true;
            result.type = GizmoType::Scale;
            result.axis = GizmoAxis::XYZ;
            result.distance = distance;
            return result;
        }

        // Test individual axis cubes
        // Test X axis cube
        glm::vec3 xCubePos = nodePosition + glm::vec3(gizmoScale * 0.8f, 0, 0);
        if (TestCubeHit(rayOrigin, rayDirection, xCubePos, cubeSize, distance)) {
            result.hit = true;
            result.type = GizmoType::Scale;
            result.axis = GizmoAxis::X;
            result.distance = distance;
            return result;
        }

        // Test Y axis cube
        glm::vec3 yCubePos = nodePosition + glm::vec3(0, gizmoScale * 0.8f, 0);
        if (TestCubeHit(rayOrigin, rayDirection, yCubePos, cubeSize, distance)) {
            result.hit = true;
            result.type = GizmoType::Scale;
            result.axis = GizmoAxis::Y;
            result.distance = distance;
            return result;
        }

        // Test Z axis cube
        glm::vec3 zCubePos = nodePosition + glm::vec3(0, 0, gizmoScale * 0.8f);
        if (TestCubeHit(rayOrigin, rayDirection, zCubePos, cubeSize, distance)) {
            result.hit = true;
            result.type = GizmoType::Scale;
            result.axis = GizmoAxis::Z;
            result.distance = distance;
            return result;
        }
    }

    return result;
}

bool GizmoRenderer::TestArrowHit(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                 const glm::vec3& arrowStart, const glm::vec3& arrowEnd,
                                 float radius, float& distance)
{
    // Ray-cylinder intersection test for arrow shaft
    glm::vec3 arrowDir = glm::normalize(arrowEnd - arrowStart);
    glm::vec3 toRayOrigin = rayOrigin - arrowStart;

    // Project ray direction and toRayOrigin onto plane perpendicular to arrow
    glm::vec3 rayDirPerp = rayDirection - glm::dot(rayDirection, arrowDir) * arrowDir;
    glm::vec3 toRayOriginPerp = toRayOrigin - glm::dot(toRayOrigin, arrowDir) * arrowDir;

    // Solve quadratic equation for ray-cylinder intersection
    float a = glm::dot(rayDirPerp, rayDirPerp);
    float b = 2.0f * glm::dot(rayDirPerp, toRayOriginPerp);
    float c = glm::dot(toRayOriginPerp, toRayOriginPerp) - radius * radius;

    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0) {
        return false; // No intersection
    }

    float t1 = (-b - sqrt(discriminant)) / (2 * a);
    float t2 = (-b + sqrt(discriminant)) / (2 * a);

    // Check if intersection points are within the arrow length
    for (float t : {t1, t2}) {
        if (t >= 0) { // Ray parameter must be positive
            glm::vec3 hitPoint = rayOrigin + t * rayDirection;
            glm::vec3 toHit = hitPoint - arrowStart;
            float projLength = glm::dot(toHit, arrowDir);

            if (projLength >= 0 && projLength <= glm::length(arrowEnd - arrowStart)) {
                distance = t;
                return true;
            }
        }
    }

    return false;
}

bool GizmoRenderer::TestPlaneHit(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                const glm::vec3& planeOrigin, const glm::vec3& axis1, const glm::vec3& axis2,
                                float size, float& distance)
{
    // Calculate plane normal
    glm::vec3 normal = glm::normalize(glm::cross(axis1, axis2));

    // Check if ray is parallel to plane
    float denominator = glm::dot(normal, rayDirection);
    if (glm::abs(denominator) < 1e-6f) {
        return false; // Ray is parallel to plane
    }

    // Calculate intersection distance
    glm::vec3 toPlane = planeOrigin - rayOrigin;
    distance = glm::dot(toPlane, normal) / denominator;

    if (distance < 0) {
        return false; // Intersection is behind ray origin
    }

    // Calculate intersection point
    glm::vec3 intersectionPoint = rayOrigin + rayDirection * distance;
    glm::vec3 localPoint = intersectionPoint - planeOrigin;

    // Project onto plane axes to get local coordinates
    float u = glm::dot(localPoint, axis1);
    float v = glm::dot(localPoint, axis2);

    // Check if intersection is within the plane handle bounds
    float minBound = size * 0.2f;
    float maxBound = size;

    return (u >= minBound && u <= maxBound && v >= minBound && v <= maxBound);
}

bool GizmoRenderer::TestCircleHit(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                  const glm::vec3& center, const glm::vec3& normal, float radius,
                                  float& distance)
{
    // Calculate plane intersection first
    float denominator = glm::dot(normal, rayDirection);
    if (glm::abs(denominator) < 1e-6f) {
        return false; // Ray is parallel to plane
    }

    glm::vec3 toPlane = center - rayOrigin;
    distance = glm::dot(toPlane, normal) / denominator;

    if (distance < 0) {
        return false; // Intersection is behind ray origin
    }

    // Calculate intersection point on plane
    glm::vec3 intersectionPoint = rayOrigin + rayDirection * distance;

    // Check if intersection is within circle radius
    float distanceFromCenter = glm::length(intersectionPoint - center);
    float tolerance = radius * 0.1f; // 10% tolerance for easier selection

    return (distanceFromCenter >= radius - tolerance && distanceFromCenter <= radius + tolerance);
}

void GizmoRenderer::RenderCircle(const glm::mat4& transform, const glm::vec3& color, int segments)
{
    if (segments < 3) segments = 32; // Minimum segments for a circle

    // Generate circle vertices
    std::vector<float> vertices;
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle);
        float y = sin(angle);
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(0.0f);
    }

    // Create/update circle VAO if needed
    if (m_circleVAO == 0) {
        glGenVertexArrays(1, &m_circleVAO);
        glGenBuffers(1, &m_circleVBO);

        glBindVertexArray(m_circleVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_circleVBO);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    // Update buffer data
    glBindVertexArray(m_circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_circleVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    // Set uniforms - combine view-projection with local transform
    glm::mat4 mvp = m_currentViewProjection * transform;
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "uColor"), 1, glm::value_ptr(color));

    // Render as line loop
    glDrawArrays(GL_LINE_LOOP, 0, segments);
    glBindVertexArray(0);
}

void GizmoRenderer::RenderCube(const glm::mat4& transform, const glm::vec3& color)
{
    // Define cube vertices (unit cube centered at origin)
    static const float cubeVertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        // Back face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
    };

    // Define cube edges (wireframe)
    static const unsigned int cubeIndices[] = {
        // Front face
        0, 1, 1, 2, 2, 3, 3, 0,
        // Back face
        4, 5, 5, 6, 6, 7, 7, 4,
        // Connecting edges
        0, 4, 1, 5, 2, 6, 3, 7
    };

    // Create/update cube VAO if needed
    if (m_cubeVAO == 0) {
        glGenVertexArrays(1, &m_cubeVAO);
        glGenBuffers(1, &m_cubeVBO);
        glGenBuffers(1, &m_cubeEBO);

        glBindVertexArray(m_cubeVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    // Render cube wireframe
    glBindVertexArray(m_cubeVAO);

    // Set uniforms - combine view-projection with local transform
    glm::mat4 mvp = m_currentViewProjection * transform;
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "uColor"), 1, glm::value_ptr(color));

    // Render as lines
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

bool GizmoRenderer::TestCircleHit(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                  const glm::vec3& circleCenter, const glm::vec3& circleNormal,
                                  float radius, float hitRadius, float& distance)
{
    // Ray-plane intersection first
    float denom = glm::dot(circleNormal, rayDirection);
    if (glm::abs(denom) < 1e-6f) {
        return false; // Ray is parallel to plane
    }

    float t = glm::dot(circleCenter - rayOrigin, circleNormal) / denom;
    if (t < 0) {
        return false; // Intersection behind ray origin
    }

    // Get intersection point on plane
    glm::vec3 hitPoint = rayOrigin + t * rayDirection;

    // Check if hit point is within circle ring
    float distanceFromCenter = glm::length(hitPoint - circleCenter);
    if (distanceFromCenter >= radius - hitRadius && distanceFromCenter <= radius + hitRadius) {
        distance = t;
        return true;
    }

    return false;
}

bool GizmoRenderer::TestCubeHit(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                const glm::vec3& cubeCenter, float cubeSize, float& distance)
{
    // Create axis-aligned bounding box around cube
    glm::vec3 minBounds = cubeCenter - glm::vec3(cubeSize * 0.5f);
    glm::vec3 maxBounds = cubeCenter + glm::vec3(cubeSize * 0.5f);

    // Ray-AABB intersection test
    float tNear = -std::numeric_limits<float>::max();
    float tFar = std::numeric_limits<float>::max();

    for (int i = 0; i < 3; ++i) {
        if (glm::abs(rayDirection[i]) < 1e-8f) {
            // Ray is parallel to the slab
            if (rayOrigin[i] < minBounds[i] || rayOrigin[i] > maxBounds[i]) {
                return false; // Ray is outside the slab
            }
        } else {
            // Calculate intersection distances
            float t1 = (minBounds[i] - rayOrigin[i]) / rayDirection[i];
            float t2 = (maxBounds[i] - rayOrigin[i]) / rayDirection[i];

            if (t1 > t2) {
                std::swap(t1, t2);
            }

            tNear = glm::max(tNear, t1);
            tFar = glm::min(tFar, t2);

            if (tNear > tFar) {
                return false; // No intersection
            }
        }
    }

    // Check if intersection is in front of ray origin
    if (tFar >= 0.0f) {
        distance = tNear >= 0.0f ? tNear : tFar;
        return true;
    }

    return false;
}
