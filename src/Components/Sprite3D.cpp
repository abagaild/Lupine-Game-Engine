#include "lupine/components/Sprite3D.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/resources/MeshLoader.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/Camera.h"
#include "lupine/rendering/GraphicsDeviceFactory.h"
#include "lupine/rendering/GraphicsTexture.h"
#include "lupine/rendering/GraphicsVertexArray.h"
#include "lupine/rendering/GraphicsBuffer.h"
#include "lupine/rendering/VertexLayout.h"
#include <iostream>
#include <algorithm>

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
    , m_texture(nullptr)
    , m_texture_loaded(false)
    , m_vertex_array(nullptr)
    , m_vertex_buffer(nullptr)
    , m_index_buffer(nullptr)
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
        m_texture = nullptr;
        return;
    }

    // Initialize ResourceManager if not already done
    if (!ResourceManager::IsInitialized()) {
        ResourceManager::Initialize();
    }

    // Load texture using ResourceManager with unified graphics API
    Texture texture = ResourceManager::LoadTexture(m_texture_path);
    if (texture.IsValid()) {
        m_texture = texture.graphics_texture;
        m_texture_loaded = true;
        std::cout << "Sprite3D: Loaded texture " << m_texture_path << std::endl;
    } else {
        m_texture = nullptr;
        m_texture_loaded = false;
        std::cerr << "Sprite3D: Failed to load texture " << m_texture_path << std::endl;
    }
}

void Sprite3D::InitializeMesh() {
    if (m_mesh_initialized) {
        return;
    }

    // Get graphics device
    auto graphics_device = GraphicsDeviceFactory::GetDevice();
    if (!graphics_device) {
        std::cerr << "Sprite3D: No graphics device available" << std::endl;
        return;
    }

    try {
        // Create vertex array
        m_vertex_array = graphics_device->CreateVertexArray();
        if (!m_vertex_array) {
            std::cerr << "Sprite3D: Failed to create vertex array" << std::endl;
            return;
        }

        // Create vertex buffer (will be updated in UpdateMeshVertices)
        m_vertex_buffer = graphics_device->CreateBuffer(
            BufferType::Vertex,
            BufferUsage::Dynamic,
            4 * 8 * sizeof(float), // 4 vertices, 8 floats each (pos3, normal3, uv2)
            nullptr
        );
        if (!m_vertex_buffer) {
            std::cerr << "Sprite3D: Failed to create vertex buffer" << std::endl;
            return;
        }

        // Create index buffer
        unsigned int indices[] = {
            0, 1, 2,  // First triangle
            2, 3, 0   // Second triangle
        };
        m_index_buffer = graphics_device->CreateBuffer(
            BufferType::Index,
            BufferUsage::Static,
            sizeof(indices),
            indices
        );
        if (!m_index_buffer) {
            std::cerr << "Sprite3D: Failed to create index buffer" << std::endl;
            return;
        }

        // Set up vertex layout
        VertexLayout layout;
        layout.AddAttribute(0, 3, static_cast<uint32_t>(VertexAttributeType::Float), false); // Position
        layout.AddAttribute(1, 3, static_cast<uint32_t>(VertexAttributeType::Float), false); // Normal
        layout.AddAttribute(2, 2, static_cast<uint32_t>(VertexAttributeType::Float), false); // Texture coordinates

        // Set up vertex array with buffers and layout
        m_vertex_array->Bind();
        m_vertex_array->SetVertexBuffer(m_vertex_buffer, layout);
        m_vertex_array->SetIndexBuffer(m_index_buffer);

        m_mesh_initialized = true;
        UpdateMeshVertices();

    } catch (const std::exception& e) {
        std::cerr << "Sprite3D: Exception during mesh initialization: " << e.what() << std::endl;
        // Cleanup on error
        m_vertex_array = nullptr;
        m_vertex_buffer = nullptr;
        m_index_buffer = nullptr;
    }
}

void Sprite3D::UpdateMeshVertices() {
    if (!m_mesh_initialized) {
        return;
    }

    // Calculate sprite bounds (using same approach as AnimatedSprite3D)
    glm::vec2 half_size = m_size * 0.5f;
    glm::vec2 sprite_offset = m_offset;

    if (m_centered) {
        sprite_offset -= half_size;
    }

    // Calculate vertex positions
    float left = sprite_offset.x;
    float right = sprite_offset.x + m_size.x;
    float bottom = sprite_offset.y;
    float top = sprite_offset.y + m_size.y;

    // Calculate texture coordinates
    float tex_left = 0.0f;
    float tex_right = 1.0f;
    float tex_bottom = 0.0f;
    float tex_top = 1.0f;

    if (m_region_enabled) {
        tex_left = m_region_rect.x;
        tex_bottom = m_region_rect.y;
        tex_right = m_region_rect.x + m_region_rect.z;
        tex_top = m_region_rect.y + m_region_rect.w;
    }

    // Apply flipping
    if (m_flip_h) {
        std::swap(tex_left, tex_right);
    }
    if (m_flip_v) {
        std::swap(tex_bottom, tex_top);
    }

    // Define vertices (position, normal, texture coordinates)
    float vertices[] = {
        // Bottom-left
        left, bottom, 0.0f,   0.0f, 0.0f, 1.0f,   tex_left, tex_bottom,
        // Bottom-right
        right, bottom, 0.0f,  0.0f, 0.0f, 1.0f,   tex_right, tex_bottom,
        // Top-right
        right, top, 0.0f,     0.0f, 0.0f, 1.0f,   tex_right, tex_top,
        // Top-left
        left, top, 0.0f,      0.0f, 0.0f, 1.0f,   tex_left, tex_top
    };

    // Update vertex buffer with new vertex data
    if (m_vertex_buffer) {
        m_vertex_buffer->UpdateData(vertices, sizeof(vertices));
    }
}

void Sprite3D::RenderSprite(Node3D* node3d) {
    if (!node3d) {
        std::cerr << "Sprite3D: No Node3D owner for rendering" << std::endl;
        return;
    }

    if (!m_mesh_initialized) {
        std::cerr << "Sprite3D: Mesh not initialized for rendering" << std::endl;
        return;
    }

    // Debug: Log rendering attempt
    static bool debug_logged = false;
    if (!debug_logged) {
        std::cout << "Sprite3D: Attempting to render sprite" << std::endl;
        std::cout << "Sprite3D: Texture loaded: " << m_texture_loaded << std::endl;
        std::cout << "Sprite3D: Texture path: " << m_texture_path << std::endl;
        debug_logged = true;
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

    // Create a temporary mesh structure for rendering with unified graphics API
    Mesh temp_mesh;
    temp_mesh.VAO = m_vertex_array;
    temp_mesh.vertices.resize(4); // Just for size info
    temp_mesh.indices.resize(6);  // Just for size info

    // Debug: Check texture and modulate values
    static bool debug_render_logged = false;
    if (!debug_render_logged) {
        std::cout << "Sprite3D: Modulate color: " << m_modulate.r << ", " << m_modulate.g << ", " << m_modulate.b << ", " << m_modulate.a << std::endl;
        if (m_texture_loaded && m_texture && m_texture->IsValid()) {
            std::cout << "Sprite3D: Rendering with texture, handle: " << m_texture->GetNativeHandle() << std::endl;
        } else {
            std::cout << "Sprite3D: Rendering with white texture" << std::endl;
        }
        debug_render_logged = true;
    }

    // Render the mesh with texture if available
    if (m_texture_loaded && m_texture && m_texture->IsValid()) {
        Renderer::RenderMesh(temp_mesh, final_transform, m_modulate, m_texture, m_receives_lighting);
    } else {
        // Render without texture (white quad with modulate color) - use white texture with lighting control
        Renderer::RenderMesh(temp_mesh, final_transform, m_modulate, Renderer::GetWhiteTexture(), m_receives_lighting);
    }
}

glm::mat4 Sprite3D::CalculateBillboardTransform(const glm::mat4& node_transform) const {
    // Extract position from node transform
    glm::vec3 position = glm::vec3(node_transform[3]);

    // Extract scale from node transform
    glm::vec3 scale = glm::vec3(
        glm::length(glm::vec3(node_transform[0])),
        glm::length(glm::vec3(node_transform[1])),
        glm::length(glm::vec3(node_transform[2]))
    );

    // Get camera information from renderer
    glm::mat4 view_matrix = Renderer::GetViewMatrix();
    glm::mat4 inv_view = glm::inverse(view_matrix);

    // Extract camera vectors from inverse view matrix
    glm::vec3 camera_right = glm::normalize(glm::vec3(inv_view[0]));
    glm::vec3 camera_up = glm::normalize(glm::vec3(inv_view[1]));
    glm::vec3 camera_forward = -glm::normalize(glm::vec3(inv_view[2]));

    glm::mat4 billboard_transform = glm::mat4(1.0f);

    switch (m_billboard_mode) {
        case BillboardMode::Enabled: {
            // Full billboard - always face camera
            billboard_transform[0] = glm::vec4(camera_right * scale.x, 0.0f);
            billboard_transform[1] = glm::vec4(camera_up * scale.y, 0.0f);
            billboard_transform[2] = glm::vec4(camera_forward * scale.z, 0.0f);
            billboard_transform[3] = glm::vec4(position, 1.0f);
            break;
        }
        case BillboardMode::YBillboard: {
            // Y-axis billboard - only rotate around Y axis to face camera
            glm::vec3 to_camera = glm::normalize(glm::vec3(inv_view[3]) - position);
            to_camera.y = 0.0f; // Remove Y component
            to_camera = glm::normalize(to_camera);

            glm::vec3 right = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), to_camera);
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 forward = to_camera;

            billboard_transform[0] = glm::vec4(right * scale.x, 0.0f);
            billboard_transform[1] = glm::vec4(up * scale.y, 0.0f);
            billboard_transform[2] = glm::vec4(forward * scale.z, 0.0f);
            billboard_transform[3] = glm::vec4(position, 1.0f);
            break;
        }
        case BillboardMode::ParticlesBillboard: {
            // Special billboard mode for particles (same as full billboard for now)
            billboard_transform[0] = glm::vec4(camera_right * scale.x, 0.0f);
            billboard_transform[1] = glm::vec4(camera_up * scale.y, 0.0f);
            billboard_transform[2] = glm::vec4(camera_forward * scale.z, 0.0f);
            billboard_transform[3] = glm::vec4(position, 1.0f);
            break;
        }
        default:
            return node_transform;
    }

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
