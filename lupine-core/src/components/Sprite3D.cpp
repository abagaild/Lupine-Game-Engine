#include "lupine/components/Sprite3D.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/resources/MeshLoader.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/Camera.h"
#include <iostream>
#include <algorithm>
#include <glad/glad.h>

namespace Lupine {

Sprite3D::Sprite3D()
    : Component("Sprite3D")
    , m_texture_path("")
    , m_modulate(1.0f, 1.0f, 1.0f, 1.0f)
    , m_size(1.0f, 1.0f)
    , m_offset(0.0f, 0.0f)
    , m_centered(true)
    , m_flip_h(false)
    , m_flip_v(false)
    , m_region_enabled(false)
    , m_region_rect(0.0f, 0.0f, 1.0f, 1.0f)
    , m_billboard_mode(BillboardMode::Disabled)
    , m_alpha_cut_mode(AlphaCutMode::Disabled)
    , m_alpha_cut_threshold(0.5f)
    , m_transparent(false)
    , m_double_sided(false)
    , m_receives_lighting(true)
    , m_texture_id(0)
    , m_texture_loaded(false)
    , m_vertex_array_object(0)
    , m_vertex_buffer_object(0)
    , m_element_buffer_object(0)
    , m_mesh_initialized(false) {

    // Initialize export variables
    InitializeExportVariables();
}

void Sprite3D::SetTexturePath(const std::string& path) {
    m_texture_path = path;
    SetExportVariable("texture_path", path);
    m_texture_loaded = false; // Force reload
}

void Sprite3D::SetModulate(const glm::vec4& modulate) {
    m_modulate = modulate;
    SetExportVariable("modulate", modulate);
}

void Sprite3D::SetSize(const glm::vec2& size) {
    m_size = glm::max(size, glm::vec2(0.001f)); // Prevent zero/negative size
    SetExportVariable("size", m_size);
    UpdateMeshVertices();
}

void Sprite3D::SetOffset(const glm::vec2& offset) {
    m_offset = offset;
    SetExportVariable("offset", offset);
    UpdateMeshVertices();
}

void Sprite3D::SetCentered(bool centered) {
    m_centered = centered;
    SetExportVariable("centered", centered);
    UpdateMeshVertices();
}

void Sprite3D::SetFlippedH(bool flip_h) {
    m_flip_h = flip_h;
    SetExportVariable("flip_h", flip_h);
    UpdateMeshVertices();
}

void Sprite3D::SetFlippedV(bool flip_v) {
    m_flip_v = flip_v;
    SetExportVariable("flip_v", flip_v);
    UpdateMeshVertices();
}

void Sprite3D::SetBillboardMode(BillboardMode mode) {
    m_billboard_mode = mode;
    SetExportVariable("billboard_mode", static_cast<int>(mode));
}

void Sprite3D::SetAlphaCutMode(AlphaCutMode mode) {
    m_alpha_cut_mode = mode;
    SetExportVariable("alpha_cut_mode", static_cast<int>(mode));
}

void Sprite3D::SetAlphaCutThreshold(float threshold) {
    m_alpha_cut_threshold = std::clamp(threshold, 0.0f, 1.0f);
    SetExportVariable("alpha_cut_threshold", m_alpha_cut_threshold);
}

void Sprite3D::SetTransparent(bool transparent) {
    m_transparent = transparent;
    SetExportVariable("transparent", transparent);
}

void Sprite3D::SetDoubleSided(bool double_sided) {
    m_double_sided = double_sided;
    SetExportVariable("double_sided", double_sided);
}

void Sprite3D::SetReceivesLighting(bool receives_lighting) {
    m_receives_lighting = receives_lighting;
    SetExportVariable("receives_lighting", receives_lighting);
}

void Sprite3D::SetRegionRect(const glm::vec4& region) {
    m_region_rect = region;
    SetExportVariable("region_rect", region);
    UpdateMeshVertices();
}

void Sprite3D::SetRegionEnabled(bool enabled) {
    m_region_enabled = enabled;
    SetExportVariable("region_enabled", enabled);
    UpdateMeshVertices();
}

void Sprite3D::OnReady() {
    UpdateFromExportVariables();
    LoadTexture();
    InitializeMesh();
}

void Sprite3D::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if export variables have changed
    UpdateFromExportVariables();

    // Check rendering context - Sprite3D should only render in 3D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor2D) {
        return; // Don't render 3D sprites in 2D editor view
    }

    // Load texture if needed
    if (!m_texture_path.empty() && !m_texture_loaded) {
        LoadTexture();
    }

    // Initialize mesh if needed
    if (!m_mesh_initialized) {
        InitializeMesh();
    }

    // Render the sprite
    if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
        RenderSprite(node3d);
    }
}

void Sprite3D::InitializeExportVariables() {
    AddExportVariable("texture_path", m_texture_path, "Path to texture file", ExportVariableType::FilePath);
    AddExportVariable("modulate", m_modulate, "Color modulation (RGBA)", ExportVariableType::Color);
    AddExportVariable("size", m_size, "Sprite size in world units", ExportVariableType::Vec2);
    AddExportVariable("offset", m_offset, "Sprite offset from node position", ExportVariableType::Vec2);
    AddExportVariable("centered", m_centered, "Center sprite on node position", ExportVariableType::Bool);
    AddExportVariable("flip_h", m_flip_h, "Flip sprite horizontally", ExportVariableType::Bool);
    AddExportVariable("flip_v", m_flip_v, "Flip sprite vertically", ExportVariableType::Bool);
    std::vector<std::string> billboardModeOptions = {
        "Disabled", "Enabled", "Y-Billboard", "Particles"
    };
    AddEnumExportVariable("billboard_mode", static_cast<int>(m_billboard_mode), "Billboard mode", billboardModeOptions);

    std::vector<std::string> alphaCutModeOptions = {
        "Disabled", "Discard", "OpaquePrepass"
    };
    AddEnumExportVariable("alpha_cut_mode", static_cast<int>(m_alpha_cut_mode), "Alpha cut mode", alphaCutModeOptions);
    AddExportVariable("alpha_cut_threshold", m_alpha_cut_threshold, "Alpha cut threshold (0.0 to 1.0)", ExportVariableType::Float);
    AddExportVariable("transparent", m_transparent, "Enable transparency", ExportVariableType::Bool);
    AddExportVariable("double_sided", m_double_sided, "Render on both sides", ExportVariableType::Bool);
    AddExportVariable("receives_lighting", m_receives_lighting, "Enable lighting calculations", ExportVariableType::Bool);
    AddExportVariable("region_enabled", m_region_enabled, "Use texture region", ExportVariableType::Bool);
    AddExportVariable("region_rect", m_region_rect, "Texture region (x, y, width, height)", ExportVariableType::Vec4);
}

void Sprite3D::UpdateExportVariables() {
    SetExportVariable("texture_path", m_texture_path);
    SetExportVariable("modulate", m_modulate);
    SetExportVariable("size", m_size);
    SetExportVariable("offset", m_offset);
    SetExportVariable("centered", m_centered);
    SetExportVariable("flip_h", m_flip_h);
    SetExportVariable("flip_v", m_flip_v);
    SetExportVariable("billboard_mode", static_cast<int>(m_billboard_mode));
    SetExportVariable("alpha_cut_mode", static_cast<int>(m_alpha_cut_mode));
    SetExportVariable("alpha_cut_threshold", m_alpha_cut_threshold);
    SetExportVariable("transparent", m_transparent);
    SetExportVariable("double_sided", m_double_sided);
    SetExportVariable("receives_lighting", m_receives_lighting);
    SetExportVariable("region_enabled", m_region_enabled);
    SetExportVariable("region_rect", m_region_rect);
}

void Sprite3D::UpdateFromExportVariables() {
    std::string new_texture_path = GetExportVariableValue<std::string>("texture_path", m_texture_path);
    if (new_texture_path != m_texture_path) {
        m_texture_path = new_texture_path;
        m_texture_loaded = false;
        LoadTexture();
    }

    m_modulate = GetExportVariableValue<glm::vec4>("modulate", m_modulate);
    
    glm::vec2 new_size = GetExportVariableValue<glm::vec2>("size", m_size);
    glm::vec2 new_offset = GetExportVariableValue<glm::vec2>("offset", m_offset);
    bool new_centered = GetExportVariableValue<bool>("centered", m_centered);
    bool new_flip_h = GetExportVariableValue<bool>("flip_h", m_flip_h);
    bool new_flip_v = GetExportVariableValue<bool>("flip_v", m_flip_v);
    bool new_region_enabled = GetExportVariableValue<bool>("region_enabled", m_region_enabled);
    glm::vec4 new_region_rect = GetExportVariableValue<glm::vec4>("region_rect", m_region_rect);

    bool mesh_needs_update = (new_size != m_size || new_offset != m_offset || 
                             new_centered != m_centered || new_flip_h != m_flip_h || 
                             new_flip_v != m_flip_v || new_region_enabled != m_region_enabled ||
                             new_region_rect != m_region_rect);

    m_size = new_size;
    m_offset = new_offset;
    m_centered = new_centered;
    m_flip_h = new_flip_h;
    m_flip_v = new_flip_v;
    m_region_enabled = new_region_enabled;
    m_region_rect = new_region_rect;

    if (mesh_needs_update) {
        UpdateMeshVertices();
    }

    m_billboard_mode = static_cast<BillboardMode>(GetExportVariableValue<int>("billboard_mode", static_cast<int>(m_billboard_mode)));
    m_alpha_cut_mode = static_cast<AlphaCutMode>(GetExportVariableValue<int>("alpha_cut_mode", static_cast<int>(m_alpha_cut_mode)));
    m_alpha_cut_threshold = GetExportVariableValue<float>("alpha_cut_threshold", m_alpha_cut_threshold);
    m_transparent = GetExportVariableValue<bool>("transparent", m_transparent);
    m_double_sided = GetExportVariableValue<bool>("double_sided", m_double_sided);
    m_receives_lighting = GetExportVariableValue<bool>("receives_lighting", m_receives_lighting);
}

void Sprite3D::LoadTexture() {
    if (m_texture_path.empty()) {
        m_texture_loaded = false;
        m_texture_id = 0;
        return;
    }

    // Initialize ResourceManager if not already done
    if (!ResourceManager::IsInitialized()) {
        ResourceManager::Initialize();
    }

    // Load texture using ResourceManager
    Texture texture = ResourceManager::LoadTexture(m_texture_path);
    if (texture.IsValid()) {
        m_texture_id = texture.id;
        m_texture_loaded = true;
        std::cout << "Sprite3D: Loaded texture " << m_texture_path << std::endl;
    } else {
        m_texture_id = 0;
        m_texture_loaded = false;
        std::cerr << "Sprite3D: Failed to load texture " << m_texture_path << std::endl;
    }
}

void Sprite3D::InitializeMesh() {
    if (m_mesh_initialized) {
        return;
    }

    // Generate OpenGL objects
    glGenVertexArrays(1, &m_vertex_array_object);
    glGenBuffers(1, &m_vertex_buffer_object);
    glGenBuffers(1, &m_element_buffer_object);

    // Bind VAO
    glBindVertexArray(m_vertex_array_object);

    // Bind and setup VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_object);

    // Bind and setup EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_element_buffer_object);

    // Define indices for quad (two triangles)
    unsigned int indices[] = {
        0, 1, 2,  // First triangle
        2, 3, 0   // Second triangle
    };
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

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

    m_mesh_initialized = true;
    UpdateMeshVertices();
}

void Sprite3D::UpdateMeshVertices() {
    if (!m_mesh_initialized) {
        return;
    }

    // Calculate sprite bounds
    glm::vec4 bounds = GetLocalBounds();
    float left = bounds.x;
    float top = bounds.y;
    float right = bounds.x + bounds.z;
    float bottom = bounds.y + bounds.w;

    // Calculate texture coordinates
    glm::vec2 uv_min(0.0f, 0.0f);
    glm::vec2 uv_max(1.0f, 1.0f);

    if (m_region_enabled) {
        uv_min = glm::vec2(m_region_rect.x, m_region_rect.y);
        uv_max = glm::vec2(m_region_rect.x + m_region_rect.z, m_region_rect.y + m_region_rect.w);
    }

    // Apply flipping
    if (m_flip_h) {
        std::swap(uv_min.x, uv_max.x);
    }
    if (m_flip_v) {
        std::swap(uv_min.y, uv_max.y);
    }

    // Define vertices (position, normal, texture coordinates)
    float vertices[] = {
        // Bottom-left
        left, bottom, 0.0f,   0.0f, 0.0f, 1.0f,   uv_min.x, uv_max.y,
        // Bottom-right
        right, bottom, 0.0f,  0.0f, 0.0f, 1.0f,   uv_max.x, uv_max.y,
        // Top-right
        right, top, 0.0f,     0.0f, 0.0f, 1.0f,   uv_max.x, uv_min.y,
        // Top-left
        left, top, 0.0f,      0.0f, 0.0f, 1.0f,   uv_min.x, uv_min.y
    };

    // Update VBO with new vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Sprite3D::RenderSprite(Node3D* node3d) {
    if (!node3d || !m_mesh_initialized) {
        return;
    }

    // Get node's world transform
    glm::mat4 node_transform = node3d->GetGlobalTransform();

    // Calculate final transform based on billboard mode
    glm::mat4 final_transform;
    if (m_billboard_mode != BillboardMode::Disabled) {
        final_transform = CalculateBillboardTransform(node_transform);
    } else {
        final_transform = node_transform;
    }

    // Create a temporary mesh structure for rendering
    Mesh temp_mesh;
    temp_mesh.VAO = m_vertex_array_object;
    temp_mesh.vertices.resize(4); // Just for size info
    temp_mesh.indices.resize(6);  // Just for size info

    // Render the mesh with texture if available
    // TODO: Convert m_texture_id to GraphicsTexture - for now use white texture
    Renderer::RenderMesh(temp_mesh, final_transform, m_modulate, Renderer::GetWhiteTexture(), m_receives_lighting);
}

glm::mat4 Sprite3D::CalculateBillboardTransform(const glm::mat4& node_transform) const {
    // Extract position from node transform
    glm::vec3 position = glm::vec3(node_transform[3]);

    // Get current camera (this is a simplified approach)
    // In a real implementation, you'd get the active camera from the scene
    glm::mat4 billboard_transform = glm::mat4(1.0f);
    billboard_transform[3] = glm::vec4(position, 1.0f);

    // For now, just return the position-only transform
    // TODO: Implement proper billboard calculation based on camera direction
    return billboard_transform;
}

glm::vec4 Sprite3D::GetLocalBounds() const {
    glm::vec2 half_size = m_size * 0.5f;
    glm::vec2 sprite_offset = m_offset;

    if (m_centered) {
        sprite_offset -= half_size;
    }

    return glm::vec4(sprite_offset.x, sprite_offset.y, m_size.x, m_size.y);
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(Sprite3D, "3D", "3D sprite component for rendering 2D textures in 3D space")
