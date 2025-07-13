#include "lupine/components/OmniLight.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/resources/MeshLoader.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/GraphicsDeviceFactory.h"
#include "lupine/rendering/GraphicsBuffer.h"
#include "lupine/rendering/GraphicsVertexArray.h"
#include "lupine/rendering/VertexLayout.h"
#include <iostream>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Lupine {

OmniLight::OmniLight()
    : Component("OmniLight")
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_intensity(1.0f)
    , m_range(10.0f)
    , m_enabled(true)
    , m_attenuation_constant(1.0f)
    , m_attenuation_linear(0.09f)
    , m_attenuation_quadratic(0.032f)
    , m_shadow_mode(ShadowMode::Enabled)
    , m_shadow_opacity(0.8f)
    , m_shadow_bias(0.001f)
    , m_shadow_color(0.3f, 0.2f, 0.2f, 1.0f)
    , m_debug_enabled(false)
    , m_debug_sphere_vao(nullptr)
    , m_debug_sphere_vbo(nullptr)
    , m_debug_sphere_ebo(nullptr)
    , m_debug_mesh_initialized(false) {

    // Initialize export variables
    InitializeExportVariables();
}

void OmniLight::SetColor(const glm::vec3& color) {
    glm::vec3 clamped_color = glm::max(color, glm::vec3(0.0f)); // Prevent negative colors
    m_color = glm::vec4(clamped_color, 1.0f);
    SetExportVariable("color", m_color);
}

void OmniLight::SetIntensity(float intensity) {
    m_intensity = std::max(0.0f, intensity);
    SetExportVariable("intensity", m_intensity);
}

void OmniLight::SetRange(float range) {
    m_range = std::max(0.1f, range);
    SetExportVariable("range", m_range);
    
    // Regenerate debug mesh if needed
    if (m_debug_mesh_initialized) {
        m_debug_mesh_initialized = false;
        InitializeDebugMesh();
    }
}

void OmniLight::SetAttenuationConstant(float constant) {
    m_attenuation_constant = std::max(0.0f, constant);
    SetExportVariable("attenuation_constant", m_attenuation_constant);
}

void OmniLight::SetAttenuationLinear(float linear) {
    m_attenuation_linear = std::max(0.0f, linear);
    SetExportVariable("attenuation_linear", m_attenuation_linear);
}

void OmniLight::SetAttenuationQuadratic(float quadratic) {
    m_attenuation_quadratic = std::max(0.0f, quadratic);
    SetExportVariable("attenuation_quadratic", m_attenuation_quadratic);
}

void OmniLight::SetEnabled(bool enabled) {
    m_enabled = enabled;
    SetExportVariable("enabled", enabled);
}

void OmniLight::SetShadowMode(ShadowMode mode) {
    m_shadow_mode = mode;
    SetExportVariable("shadow_mode", static_cast<int>(mode));
}

void OmniLight::SetShadowOpacity(float opacity) {
    m_shadow_opacity = std::clamp(opacity, 0.0f, 1.0f);
    SetExportVariable("shadow_opacity", m_shadow_opacity);
}

void OmniLight::SetShadowBias(float bias) {
    m_shadow_bias = std::max(0.0f, bias);
    SetExportVariable("shadow_bias", m_shadow_bias);
}

void OmniLight::SetShadowColor(const glm::vec3& color) {
    glm::vec3 clamped_color = glm::max(color, glm::vec3(0.0f)); // Prevent negative colors
    m_shadow_color = glm::vec4(clamped_color, 1.0f);
    SetExportVariable("shadow_color", m_shadow_color);
}

void OmniLight::SetDebugEnabled(bool enabled) {
    m_debug_enabled = enabled;
    SetExportVariable("debug_enabled", enabled);
    
    if (enabled && !m_debug_mesh_initialized) {
        InitializeDebugMesh();
    }
}

glm::vec3 OmniLight::GetWorldPosition() const {
    if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
        return node3d->GetGlobalPosition();
    }
    return glm::vec3(0.0f);
}

float OmniLight::CalculateAttenuation(float distance) const {
    if (distance >= m_range) {
        return 0.0f;
    }
    
    float attenuation = 1.0f / (m_attenuation_constant + 
                               m_attenuation_linear * distance + 
                               m_attenuation_quadratic * distance * distance);
    
    return std::clamp(attenuation, 0.0f, 1.0f);
}

bool OmniLight::IsPointInRange(const glm::vec3& world_position) const {
    glm::vec3 light_pos = GetWorldPosition();
    float distance = glm::length(world_position - light_pos);
    return distance <= m_range;
}

void OmniLight::OnReady() {
    UpdateFromExportVariables();
    if (m_debug_enabled) {
        InitializeDebugMesh();
    }
}

void OmniLight::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if export variables have changed
    UpdateFromExportVariables();
}

void OmniLight::InitializeExportVariables() {
    // Create enum options for shadow mode
    std::vector<std::string> shadowModeOptions = {
        "Disabled", "Enabled"
    };

    AddExportVariable("color", m_color, "Light color (RGB)", ExportVariableType::Color);
    AddExportVariable("intensity", m_intensity, "Light intensity multiplier", ExportVariableType::Float);
    AddExportVariable("range", m_range, "Maximum light range", ExportVariableType::Float);
    AddExportVariable("enabled", m_enabled, "Enable/disable light", ExportVariableType::Bool);
    AddExportVariable("attenuation_constant", m_attenuation_constant, "Constant attenuation factor", ExportVariableType::Float);
    AddExportVariable("attenuation_linear", m_attenuation_linear, "Linear attenuation factor", ExportVariableType::Float);
    AddExportVariable("attenuation_quadratic", m_attenuation_quadratic, "Quadratic attenuation factor", ExportVariableType::Float);
    AddEnumExportVariable("shadow_mode", static_cast<int>(m_shadow_mode), "Shadow mode", shadowModeOptions);
    AddExportVariable("shadow_opacity", m_shadow_opacity, "Shadow opacity (0.0 to 1.0)", ExportVariableType::Float);
    AddExportVariable("shadow_bias", m_shadow_bias, "Shadow bias to prevent acne", ExportVariableType::Float);
    AddExportVariable("shadow_color", m_shadow_color, "Shadow color (RGB)", ExportVariableType::Color);
    AddExportVariable("debug_enabled", m_debug_enabled, "Show debug visualization", ExportVariableType::Bool);
}

void OmniLight::UpdateExportVariables() {
    SetExportVariable("color", m_color);
    SetExportVariable("intensity", m_intensity);
    SetExportVariable("range", m_range);
    SetExportVariable("enabled", m_enabled);
    SetExportVariable("attenuation_constant", m_attenuation_constant);
    SetExportVariable("attenuation_linear", m_attenuation_linear);
    SetExportVariable("attenuation_quadratic", m_attenuation_quadratic);
    SetExportVariable("shadow_mode", static_cast<int>(m_shadow_mode));
    SetExportVariable("shadow_opacity", m_shadow_opacity);
    SetExportVariable("shadow_bias", m_shadow_bias);
    SetExportVariable("shadow_color", m_shadow_color);
    SetExportVariable("debug_enabled", m_debug_enabled);
}

void OmniLight::UpdateFromExportVariables() {
    m_color = GetExportVariableValue<glm::vec4>("color", m_color);
    m_intensity = GetExportVariableValue<float>("intensity", m_intensity);
    
    float new_range = GetExportVariableValue<float>("range", m_range);
    if (new_range != m_range) {
        m_range = new_range;
        if (m_debug_mesh_initialized) {
            m_debug_mesh_initialized = false;
            InitializeDebugMesh();
        }
    }
    
    m_enabled = GetExportVariableValue<bool>("enabled", m_enabled);
    m_attenuation_constant = GetExportVariableValue<float>("attenuation_constant", m_attenuation_constant);
    m_attenuation_linear = GetExportVariableValue<float>("attenuation_linear", m_attenuation_linear);
    m_attenuation_quadratic = GetExportVariableValue<float>("attenuation_quadratic", m_attenuation_quadratic);
    m_shadow_mode = static_cast<ShadowMode>(GetExportVariableValue<int>("shadow_mode", static_cast<int>(m_shadow_mode)));
    m_shadow_opacity = GetExportVariableValue<float>("shadow_opacity", m_shadow_opacity);
    m_shadow_bias = GetExportVariableValue<float>("shadow_bias", m_shadow_bias);
    m_shadow_color = GetExportVariableValue<glm::vec4>("shadow_color", m_shadow_color);
    
    bool new_debug_enabled = GetExportVariableValue<bool>("debug_enabled", m_debug_enabled);
    if (new_debug_enabled != m_debug_enabled) {
        m_debug_enabled = new_debug_enabled;
        if (m_debug_enabled && !m_debug_mesh_initialized) {
            InitializeDebugMesh();
        }
    }
}

void OmniLight::InitializeDebugMesh() {
    if (m_debug_mesh_initialized) {
        // Clean up existing mesh - shared_ptr will handle cleanup automatically
        m_debug_sphere_vao.reset();
        m_debug_sphere_vbo.reset();
        m_debug_sphere_ebo.reset();
    }

    GenerateSphereMesh(m_range, 16);
    m_debug_mesh_initialized = true;
}

void OmniLight::RenderDebugVisualization(Node3D* node3d) {
    if (!node3d || !m_debug_mesh_initialized || !m_debug_sphere_vao) {
        return;
    }

    // Get node's world transform
    glm::mat4 transform = node3d->GetGlobalTransform();

    // Create a temporary mesh structure for rendering
    Mesh temp_mesh;
    temp_mesh.VAO = m_debug_sphere_vao;
    temp_mesh.vertices.resize(1); // Just for size info
    temp_mesh.indices.resize(1);  // Just for size info

    // Render wireframe sphere with light color
    glm::vec4 debug_color = glm::vec4(m_color.r * m_intensity, m_color.g * m_intensity, m_color.b * m_intensity, 0.3f); // Semi-transparent
    Renderer::RenderMesh(temp_mesh, transform, debug_color);
}

void OmniLight::GenerateSphereMesh(float radius, int segments) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Generate sphere vertices
    for (int i = 0; i <= segments; ++i) {
        float lat = M_PI * (-0.5f + (float)i / segments);
        float y = radius * sin(lat);
        float r = radius * cos(lat);

        for (int j = 0; j <= segments; ++j) {
            float lon = 2 * M_PI * (float)j / segments;
            float x = r * cos(lon);
            float z = r * sin(lon);

            // Position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // Normal (same as position for unit sphere, scaled by radius)
            vertices.push_back(x / radius);
            vertices.push_back(y / radius);
            vertices.push_back(z / radius);

            // Texture coordinates (not used for debug, but needed for vertex format)
            vertices.push_back((float)j / segments);
            vertices.push_back((float)i / segments);
        }
    }

    // Generate sphere indices
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < segments; ++j) {
            int first = i * (segments + 1) + j;
            int second = first + segments + 1;

            // First triangle
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            // Second triangle
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    // Create graphics objects using unified graphics system
    auto graphics_device = GraphicsDeviceFactory::GetDevice();
    if (!graphics_device) {
        std::cerr << "OmniLight: No graphics device available for debug mesh creation" << std::endl;
        return;
    }

    // Create vertex buffer
    m_debug_sphere_vbo = graphics_device->CreateBuffer(
        BufferType::Vertex,
        BufferUsage::Static,
        vertices.size() * sizeof(float),
        vertices.data()
    );

    // Create index buffer
    m_debug_sphere_ebo = graphics_device->CreateBuffer(
        BufferType::Index,
        BufferUsage::Static,
        indices.size() * sizeof(unsigned int),
        indices.data()
    );

    // Create vertex array
    m_debug_sphere_vao = graphics_device->CreateVertexArray();

    if (!m_debug_sphere_vao || !m_debug_sphere_vbo || !m_debug_sphere_ebo) {
        std::cerr << "OmniLight: Failed to create debug mesh graphics objects" << std::endl;
        return;
    }

    // Set up vertex layout
    VertexLayout layout;
    layout.AddAttribute(0, 3, static_cast<uint32_t>(VertexAttributeType::Float), false); // Position (location 0)
    layout.AddAttribute(1, 3, static_cast<uint32_t>(VertexAttributeType::Float), false); // Normal (location 1)
    layout.AddAttribute(2, 2, static_cast<uint32_t>(VertexAttributeType::Float), false); // Texture coordinates (location 2)

    // Configure vertex array
    m_debug_sphere_vao->SetVertexBuffer(m_debug_sphere_vbo, layout);
    m_debug_sphere_vao->SetIndexBuffer(m_debug_sphere_ebo);
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(OmniLight, "Light", "Omnidirectional point light with attenuation and shadows")
