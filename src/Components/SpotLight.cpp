#include "lupine/components/SpotLight.h"
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

SpotLight::SpotLight()
    : Component("SpotLight")
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_intensity(1.0f)
    , m_range(10.0f)
    , m_inner_cone_angle(30.0f)
    , m_outer_cone_angle(45.0f)
    , m_enabled(true)
    , m_attenuation_constant(1.0f)
    , m_attenuation_linear(0.09f)
    , m_attenuation_quadratic(0.032f)
    , m_shadow_mode(ShadowMode::Enabled)
    , m_shadow_opacity(0.8f)
    , m_shadow_bias(0.001f)
    , m_shadow_color(0.2f, 0.3f, 0.2f, 1.0f)
    , m_debug_enabled(false)
    , m_debug_cone_vao(nullptr)
    , m_debug_cone_vbo(nullptr)
    , m_debug_cone_ebo(nullptr)
    , m_debug_mesh_initialized(false) {

    // Initialize export variables
    InitializeExportVariables();
}

void SpotLight::SetColor(const glm::vec3& color) {
    glm::vec3 clamped_color = glm::max(color, glm::vec3(0.0f)); // Prevent negative colors
    m_color = glm::vec4(clamped_color, 1.0f);
    SetExportVariable("color", m_color);
}

void SpotLight::SetIntensity(float intensity) {
    m_intensity = std::max(0.0f, intensity);
    SetExportVariable("intensity", m_intensity);
}

void SpotLight::SetRange(float range) {
    m_range = std::max(0.1f, range);
    SetExportVariable("range", m_range);
    
    // Regenerate debug mesh if needed
    if (m_debug_mesh_initialized) {
        m_debug_mesh_initialized = false;
        InitializeDebugMesh();
    }
}

void SpotLight::SetInnerConeAngle(float angle) {
    m_inner_cone_angle = std::clamp(angle, 0.0f, 180.0f);
    if (m_inner_cone_angle > m_outer_cone_angle) {
        m_outer_cone_angle = m_inner_cone_angle;
        SetExportVariable("outer_cone_angle", m_outer_cone_angle);
    }
    SetExportVariable("inner_cone_angle", m_inner_cone_angle);
    
    // Regenerate debug mesh if needed
    if (m_debug_mesh_initialized) {
        m_debug_mesh_initialized = false;
        InitializeDebugMesh();
    }
}

void SpotLight::SetOuterConeAngle(float angle) {
    m_outer_cone_angle = std::clamp(angle, 0.0f, 180.0f);
    if (m_outer_cone_angle < m_inner_cone_angle) {
        m_inner_cone_angle = m_outer_cone_angle;
        SetExportVariable("inner_cone_angle", m_inner_cone_angle);
    }
    SetExportVariable("outer_cone_angle", m_outer_cone_angle);
    
    // Regenerate debug mesh if needed
    if (m_debug_mesh_initialized) {
        m_debug_mesh_initialized = false;
        InitializeDebugMesh();
    }
}

glm::vec3 SpotLight::GetDirection() const {
    if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
        // Use the node's forward direction (negative Z in local space)
        glm::quat rotation = node3d->GetGlobalRotation();
        return glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
    }
    return glm::vec3(0.0f, 0.0f, -1.0f); // Default forward direction
}

void SpotLight::SetAttenuationConstant(float constant) {
    m_attenuation_constant = std::max(0.0f, constant);
    SetExportVariable("attenuation_constant", m_attenuation_constant);
}

void SpotLight::SetAttenuationLinear(float linear) {
    m_attenuation_linear = std::max(0.0f, linear);
    SetExportVariable("attenuation_linear", m_attenuation_linear);
}

void SpotLight::SetAttenuationQuadratic(float quadratic) {
    m_attenuation_quadratic = std::max(0.0f, quadratic);
    SetExportVariable("attenuation_quadratic", m_attenuation_quadratic);
}

void SpotLight::SetEnabled(bool enabled) {
    m_enabled = enabled;
    SetExportVariable("enabled", enabled);
}

void SpotLight::SetShadowMode(ShadowMode mode) {
    m_shadow_mode = mode;
    SetExportVariable("shadow_mode", static_cast<int>(mode));
}

void SpotLight::SetShadowOpacity(float opacity) {
    m_shadow_opacity = std::clamp(opacity, 0.0f, 1.0f);
    SetExportVariable("shadow_opacity", m_shadow_opacity);
}

void SpotLight::SetShadowBias(float bias) {
    m_shadow_bias = std::max(0.0f, bias);
    SetExportVariable("shadow_bias", m_shadow_bias);
}

void SpotLight::SetShadowColor(const glm::vec3& color) {
    glm::vec3 clamped_color = glm::max(color, glm::vec3(0.0f)); // Prevent negative colors
    m_shadow_color = glm::vec4(clamped_color, 1.0f);
    SetExportVariable("shadow_color", m_shadow_color);
}

void SpotLight::SetDebugEnabled(bool enabled) {
    m_debug_enabled = enabled;
    SetExportVariable("debug_enabled", enabled);
    
    if (enabled && !m_debug_mesh_initialized) {
        InitializeDebugMesh();
    }
}

glm::vec3 SpotLight::GetWorldPosition() const {
    if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
        return node3d->GetGlobalPosition();
    }
    return glm::vec3(0.0f);
}

float SpotLight::CalculateAttenuation(float distance) const {
    if (distance >= m_range) {
        return 0.0f;
    }
    
    float attenuation = 1.0f / (m_attenuation_constant + 
                               m_attenuation_linear * distance + 
                               m_attenuation_quadratic * distance * distance);
    
    return std::clamp(attenuation, 0.0f, 1.0f);
}

float SpotLight::CalculateAngularAttenuation(const glm::vec3& direction) const {
    glm::vec3 light_dir = GetDirection();
    float cos_angle = glm::dot(glm::normalize(direction), glm::normalize(light_dir));
    
    float inner_cos = cos(glm::radians(m_inner_cone_angle * 0.5f));
    float outer_cos = cos(glm::radians(m_outer_cone_angle * 0.5f));
    
    if (cos_angle > inner_cos) {
        return 1.0f; // Full intensity
    } else if (cos_angle > outer_cos) {
        // Smooth falloff between inner and outer cone
        return (cos_angle - outer_cos) / (inner_cos - outer_cos);
    } else {
        return 0.0f; // Outside cone
    }
}

bool SpotLight::IsPointInCone(const glm::vec3& world_position) const {
    glm::vec3 light_pos = GetWorldPosition();
    glm::vec3 to_point = world_position - light_pos;
    float distance = glm::length(to_point);
    
    if (distance >= m_range) {
        return false;
    }
    
    glm::vec3 direction = to_point / distance;
    return CalculateAngularAttenuation(direction) > 0.0f;
}

void SpotLight::OnReady() {
    UpdateFromExportVariables();
    if (m_debug_enabled) {
        InitializeDebugMesh();
    }
}

void SpotLight::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if export variables have changed
    UpdateFromExportVariables();
}

void SpotLight::InitializeExportVariables() {
    // Create enum options for shadow mode
    std::vector<std::string> shadowModeOptions = {
        "Disabled", "Enabled"
    };

    AddExportVariable("color", m_color, "Light color (RGB)", ExportVariableType::Color);
    AddExportVariable("intensity", m_intensity, "Light intensity multiplier", ExportVariableType::Float);
    AddExportVariable("range", m_range, "Maximum light range", ExportVariableType::Float);
    AddExportVariable("inner_cone_angle", m_inner_cone_angle, "Inner cone angle (degrees)", ExportVariableType::Float);
    AddExportVariable("outer_cone_angle", m_outer_cone_angle, "Outer cone angle (degrees)", ExportVariableType::Float);
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

void SpotLight::UpdateExportVariables() {
    SetExportVariable("color", m_color);
    SetExportVariable("intensity", m_intensity);
    SetExportVariable("range", m_range);
    SetExportVariable("inner_cone_angle", m_inner_cone_angle);
    SetExportVariable("outer_cone_angle", m_outer_cone_angle);
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

void SpotLight::UpdateFromExportVariables() {
    m_color = GetExportVariableValue<glm::vec4>("color", m_color);
    m_intensity = GetExportVariableValue<float>("intensity", m_intensity);

    float new_range = GetExportVariableValue<float>("range", m_range);
    float new_inner_angle = GetExportVariableValue<float>("inner_cone_angle", m_inner_cone_angle);
    float new_outer_angle = GetExportVariableValue<float>("outer_cone_angle", m_outer_cone_angle);

    bool mesh_needs_update = (new_range != m_range || new_inner_angle != m_inner_cone_angle || new_outer_angle != m_outer_cone_angle);

    m_range = new_range;
    m_inner_cone_angle = new_inner_angle;
    m_outer_cone_angle = new_outer_angle;

    if (mesh_needs_update && m_debug_mesh_initialized) {
        m_debug_mesh_initialized = false;
        InitializeDebugMesh();
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

void SpotLight::InitializeDebugMesh() {
    if (m_debug_mesh_initialized) {
        // Clean up existing mesh - shared_ptr will handle cleanup automatically
        m_debug_cone_vao.reset();
        m_debug_cone_vbo.reset();
        m_debug_cone_ebo.reset();
    }

    GenerateConeMesh(m_range, glm::radians(m_outer_cone_angle));
    m_debug_mesh_initialized = true;
}

void SpotLight::RenderDebugVisualization(Node3D* node3d) {
    if (!node3d || !m_debug_mesh_initialized || !m_debug_cone_vao) {
        return;
    }

    // Get node's world transform
    glm::mat4 transform = node3d->GetGlobalTransform();

    // Create a temporary mesh structure for rendering
    Mesh temp_mesh;
    temp_mesh.VAO = m_debug_cone_vao;
    temp_mesh.vertices.resize(1); // Just for size info
    temp_mesh.indices.resize(1);  // Just for size info

    // Render wireframe cone with light color
    glm::vec4 debug_color = glm::vec4(m_color.r * m_intensity, m_color.g * m_intensity, m_color.b * m_intensity, 0.3f); // Semi-transparent
    Renderer::RenderMesh(temp_mesh, transform, debug_color);
}

void SpotLight::GenerateConeMesh(float range, float angle) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    int segments = 16;
    float radius = range * tan(angle * 0.5f);

    // Apex of cone (at origin)
    vertices.insert(vertices.end(), {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f});

    // Base circle vertices
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * M_PI * i / segments;
        float x = radius * cos(theta);
        float y = radius * sin(theta);
        float z = -range;

        // Position
        vertices.insert(vertices.end(), {x, y, z});

        // Normal (pointing outward from cone surface)
        glm::vec3 normal = glm::normalize(glm::vec3(x, y, range * tan(angle * 0.5f)));
        vertices.insert(vertices.end(), {normal.x, normal.y, normal.z});

        // Texture coordinates
        vertices.insert(vertices.end(), {(float)i / segments, 1.0f});
    }

    // Generate indices for cone sides
    for (int i = 0; i < segments; ++i) {
        int base1 = 1 + i;
        int base2 = 1 + ((i + 1) % (segments + 1));

        // Triangle from apex to base edge
        indices.insert(indices.end(), {0u, static_cast<unsigned int>(base1), static_cast<unsigned int>(base2)});
    }

    // Generate indices for base (optional, for closed cone)
    int center_base = vertices.size() / 8; // New vertex for base center
    vertices.insert(vertices.end(), {0.0f, 0.0f, -range, 0.0f, 0.0f, -1.0f, 0.5f, 0.5f});

    for (int i = 0; i < segments; ++i) {
        int base1 = 1 + i;
        int base2 = 1 + ((i + 1) % (segments + 1));

        // Triangle for base
        indices.insert(indices.end(), {static_cast<unsigned int>(center_base), static_cast<unsigned int>(base2), static_cast<unsigned int>(base1)});
    }

    // Create graphics objects using unified graphics system
    auto graphics_device = GraphicsDeviceFactory::GetDevice();
    if (!graphics_device) {
        std::cerr << "SpotLight: No graphics device available for debug mesh creation" << std::endl;
        return;
    }

    // Create vertex buffer
    m_debug_cone_vbo = graphics_device->CreateBuffer(
        BufferType::Vertex,
        BufferUsage::Static,
        vertices.size() * sizeof(float),
        vertices.data()
    );

    // Create index buffer
    m_debug_cone_ebo = graphics_device->CreateBuffer(
        BufferType::Index,
        BufferUsage::Static,
        indices.size() * sizeof(unsigned int),
        indices.data()
    );

    // Create vertex array
    m_debug_cone_vao = graphics_device->CreateVertexArray();

    if (!m_debug_cone_vao || !m_debug_cone_vbo || !m_debug_cone_ebo) {
        std::cerr << "SpotLight: Failed to create debug mesh graphics objects" << std::endl;
        return;
    }

    // Set up vertex layout
    VertexLayout layout;
    layout.AddAttribute(0, 3, static_cast<uint32_t>(VertexAttributeType::Float), false); // Position (location 0)
    layout.AddAttribute(1, 3, static_cast<uint32_t>(VertexAttributeType::Float), false); // Normal (location 1)
    layout.AddAttribute(2, 2, static_cast<uint32_t>(VertexAttributeType::Float), false); // Texture coordinates (location 2)

    // Configure vertex array
    m_debug_cone_vao->SetVertexBuffer(m_debug_cone_vbo, layout);
    m_debug_cone_vao->SetIndexBuffer(m_debug_cone_ebo);
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(SpotLight, "Light", "Spot light with cone-shaped illumination and attenuation")
