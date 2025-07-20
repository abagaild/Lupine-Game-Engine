#include "lupine/components/DirectionalLight.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/resources/MeshLoader.h"
#include "lupine/rendering/Renderer.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <glad/glad.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Lupine {

DirectionalLight::DirectionalLight()
    : Component("DirectionalLight")
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_intensity(1.0f)
    , m_enabled(true)
    , m_shadow_mode(ShadowMode::Enabled)
    , m_shadow_opacity(0.8f)
    , m_shadow_bias(0.001f)
    , m_shadow_cascades(4)
    , m_shadow_distance(100.0f)
    , m_shadow_color(0.2f, 0.2f, 0.3f, 1.0f)
    , m_debug_enabled(false)
    , m_debug_arrow_vao(0)
    , m_debug_arrow_vbo(0)
    , m_debug_arrow_ebo(0)
    , m_debug_mesh_initialized(false) {

    // Initialize export variables
    InitializeExportVariables();
}

void DirectionalLight::SetColor(const glm::vec3& color) {
    glm::vec3 clamped_color = glm::max(color, glm::vec3(0.0f)); // Prevent negative colors
    m_color = glm::vec4(clamped_color, 1.0f);
    SetExportVariable("color", m_color);
}

void DirectionalLight::SetIntensity(float intensity) {
    m_intensity = std::max(0.0f, intensity);
    SetExportVariable("intensity", m_intensity);
}

glm::vec3 DirectionalLight::GetDirection() const {
    if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
        // Use the node's forward direction (negative Z in local space)
        glm::quat rotation = node3d->GetGlobalRotation();
        return glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
    }
    return glm::vec3(0.0f, -1.0f, 0.0f); // Default downward direction
}

void DirectionalLight::SetEnabled(bool enabled) {
    m_enabled = enabled;
    SetExportVariable("enabled", enabled);
}

void DirectionalLight::SetShadowMode(ShadowMode mode) {
    m_shadow_mode = mode;
    SetExportVariable("shadow_mode", static_cast<int>(mode));
}

void DirectionalLight::SetShadowOpacity(float opacity) {
    m_shadow_opacity = std::clamp(opacity, 0.0f, 1.0f);
    SetExportVariable("shadow_opacity", m_shadow_opacity);
}

void DirectionalLight::SetShadowBias(float bias) {
    m_shadow_bias = std::max(0.0f, bias);
    SetExportVariable("shadow_bias", m_shadow_bias);
}

void DirectionalLight::SetShadowCascades(int cascades) {
    m_shadow_cascades = std::clamp(cascades, 1, 8);
    SetExportVariable("shadow_cascades", m_shadow_cascades);
}

void DirectionalLight::SetShadowDistance(float distance) {
    m_shadow_distance = std::max(1.0f, distance);
    SetExportVariable("shadow_distance", m_shadow_distance);
}

void DirectionalLight::SetShadowColor(const glm::vec3& color) {
    glm::vec3 clamped_color = glm::max(color, glm::vec3(0.0f)); // Prevent negative colors
    m_shadow_color = glm::vec4(clamped_color, 1.0f);
    SetExportVariable("shadow_color", m_shadow_color);
}

void DirectionalLight::SetDebugEnabled(bool enabled) {
    m_debug_enabled = enabled;
    SetExportVariable("debug_enabled", enabled);
    
    if (enabled && !m_debug_mesh_initialized) {
        InitializeDebugMesh();
    }
}

glm::vec3 DirectionalLight::GetWorldPosition() const {
    if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
        return node3d->GetGlobalPosition();
    }
    return glm::vec3(0.0f);
}

float DirectionalLight::CalculateLightContribution(const glm::vec3& surface_normal) const {
    glm::vec3 light_dir = -GetDirection(); // Light direction is opposite to the direction vector
    float dot_product = glm::dot(glm::normalize(surface_normal), glm::normalize(light_dir));
    return std::max(0.0f, dot_product);
}

void DirectionalLight::OnReady() {
    UpdateFromExportVariables();
    if (m_debug_enabled) {
        InitializeDebugMesh();
    }
}

void DirectionalLight::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if export variables have changed
    UpdateFromExportVariables();
}

void DirectionalLight::InitializeExportVariables() {
    // Create enum options for shadow mode
    std::vector<std::string> shadowModeOptions = {
        "Disabled", "Enabled"
    };

    AddExportVariable("color", m_color, "Light color (RGB)", ExportVariableType::Color);
    AddExportVariable("intensity", m_intensity, "Light intensity multiplier", ExportVariableType::Float);
    AddExportVariable("enabled", m_enabled, "Enable/disable light", ExportVariableType::Bool);
    AddEnumExportVariable("shadow_mode", static_cast<int>(m_shadow_mode), "Shadow mode", shadowModeOptions);
    AddExportVariable("shadow_opacity", m_shadow_opacity, "Shadow opacity (0.0 to 1.0)", ExportVariableType::Float);
    AddExportVariable("shadow_bias", m_shadow_bias, "Shadow bias to prevent acne", ExportVariableType::Float);
    AddExportVariable("shadow_cascades", m_shadow_cascades, "Number of shadow cascades", ExportVariableType::Int);
    AddExportVariable("shadow_distance", m_shadow_distance, "Maximum shadow distance", ExportVariableType::Float);
    AddExportVariable("shadow_color", m_shadow_color, "Shadow color (RGB)", ExportVariableType::Color);
    AddExportVariable("debug_enabled", m_debug_enabled, "Show debug visualization", ExportVariableType::Bool);
}

void DirectionalLight::UpdateExportVariables() {
    SetExportVariable("color", m_color);
    SetExportVariable("intensity", m_intensity);
    SetExportVariable("enabled", m_enabled);
    SetExportVariable("shadow_mode", static_cast<int>(m_shadow_mode));
    SetExportVariable("shadow_opacity", m_shadow_opacity);
    SetExportVariable("shadow_bias", m_shadow_bias);
    SetExportVariable("shadow_cascades", m_shadow_cascades);
    SetExportVariable("shadow_distance", m_shadow_distance);
    SetExportVariable("shadow_color", m_shadow_color);
    SetExportVariable("debug_enabled", m_debug_enabled);
}

void DirectionalLight::UpdateFromExportVariables() {
    m_color = GetExportVariableValue<glm::vec4>("color", m_color);
    m_intensity = GetExportVariableValue<float>("intensity", m_intensity);
    m_enabled = GetExportVariableValue<bool>("enabled", m_enabled);
    m_shadow_mode = static_cast<ShadowMode>(GetExportVariableValue<int>("shadow_mode", static_cast<int>(m_shadow_mode)));
    m_shadow_opacity = GetExportVariableValue<float>("shadow_opacity", m_shadow_opacity);
    m_shadow_bias = GetExportVariableValue<float>("shadow_bias", m_shadow_bias);
    m_shadow_cascades = GetExportVariableValue<int>("shadow_cascades", m_shadow_cascades);
    m_shadow_distance = GetExportVariableValue<float>("shadow_distance", m_shadow_distance);
    m_shadow_color = GetExportVariableValue<glm::vec4>("shadow_color", m_shadow_color);
    
    bool new_debug_enabled = GetExportVariableValue<bool>("debug_enabled", m_debug_enabled);
    if (new_debug_enabled != m_debug_enabled) {
        m_debug_enabled = new_debug_enabled;
        if (m_debug_enabled && !m_debug_mesh_initialized) {
            InitializeDebugMesh();
        }
    }
}

void DirectionalLight::InitializeDebugMesh() {
    if (m_debug_mesh_initialized) {
        // Clean up existing mesh
        if (m_debug_arrow_vao != 0) {
            glDeleteVertexArrays(1, &m_debug_arrow_vao);
            glDeleteBuffers(1, &m_debug_arrow_vbo);
            glDeleteBuffers(1, &m_debug_arrow_ebo);
        }
    }

    GenerateArrowMesh(2.0f);
    m_debug_mesh_initialized = true;
}

void DirectionalLight::RenderDebugVisualization(Node3D* node3d) {
    if (!node3d || !m_debug_mesh_initialized || m_debug_arrow_vao == 0) {
        return;
    }

    // Get node's world transform
    glm::mat4 transform = node3d->GetGlobalTransform();

    // Create a temporary mesh structure for rendering
    Mesh temp_mesh;
    temp_mesh.VAO = m_debug_arrow_vao;
    temp_mesh.vertices.resize(1); // Just for size info
    temp_mesh.indices.resize(1);  // Just for size info

    // Render arrow with light color
    glm::vec4 debug_color = glm::vec4(m_color.r * m_intensity, m_color.g * m_intensity, m_color.b * m_intensity, 1.0f);
    Renderer::RenderMesh(temp_mesh, transform, debug_color);
}

void DirectionalLight::GenerateArrowMesh(float length) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float shaft_radius = length * 0.05f;
    float head_radius = length * 0.15f;
    float head_length = length * 0.3f;
    float shaft_length = length - head_length;

    // Arrow shaft (cylinder)
    int segments = 8;
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = shaft_radius * cos(angle);
        float y = shaft_radius * sin(angle);

        // Bottom of shaft
        vertices.insert(vertices.end(), {x, y, 0.0f, x, y, 0.0f, 0.0f, 0.0f});
        // Top of shaft
        vertices.insert(vertices.end(), {x, y, -shaft_length, x, y, 0.0f, 1.0f, 0.0f});
    }

    // Arrow head (cone)
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = head_radius * cos(angle);
        float y = head_radius * sin(angle);

        // Base of head
        vertices.insert(vertices.end(), {x, y, -shaft_length, x, y, 0.0f, 0.0f, 1.0f});
    }

    // Tip of arrow
    vertices.insert(vertices.end(), {0.0f, 0.0f, -length, 0.0f, 0.0f, -1.0f, 0.5f, 1.0f});

    // Generate indices for shaft
    for (int i = 0; i < segments; ++i) {
        int bottom1 = i * 2;
        int top1 = i * 2 + 1;
        int bottom2 = ((i + 1) % (segments + 1)) * 2;
        int top2 = ((i + 1) % (segments + 1)) * 2 + 1;

        // Shaft quad (two triangles)
        indices.insert(indices.end(), {static_cast<unsigned int>(bottom1), static_cast<unsigned int>(top1), static_cast<unsigned int>(bottom2)});
        indices.insert(indices.end(), {static_cast<unsigned int>(top1), static_cast<unsigned int>(top2), static_cast<unsigned int>(bottom2)});
    }

    // Generate indices for arrow head
    int head_base_start = (segments + 1) * 2;
    int tip_index = head_base_start + segments + 1;

    for (int i = 0; i < segments; ++i) {
        int base1 = head_base_start + i;
        int base2 = head_base_start + ((i + 1) % (segments + 1));

        // Head triangle
        indices.insert(indices.end(), {static_cast<unsigned int>(base1), static_cast<unsigned int>(tip_index), static_cast<unsigned int>(base2)});
    }

    // Generate OpenGL objects
    glGenVertexArrays(1, &m_debug_arrow_vao);
    glGenBuffers(1, &m_debug_arrow_vbo);
    glGenBuffers(1, &m_debug_arrow_ebo);

    // Bind VAO
    glBindVertexArray(m_debug_arrow_vao);

    // Bind and setup VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_debug_arrow_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Bind and setup EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_debug_arrow_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Setup vertex attributes
    // Position (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture coordinates (location = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Unbind VAO
    glBindVertexArray(0);
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(DirectionalLight, "Light", "Directional light for uniform lighting like sunlight")
