#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/Camera.h"
#include "lupine/rendering/ViewportManager.h"
#include "lupine/rendering/LightingSystem.h"
#include "lupine/rendering/TextRenderer.h"
#include "lupine/rendering/GraphicsDeviceFactory.h"
#include "lupine/rendering/GraphicsDevice.h"
#include "lupine/rendering/GraphicsShader.h"
#include "lupine/rendering/GraphicsBuffer.h"
#include "lupine/rendering/GraphicsTexture.h"
#include "lupine/rendering/GraphicsVertexArray.h"
#include "lupine/rendering/VertexLayout.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include "lupine/core/Component.h"
#include "lupine/core/Project.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include "lupine/components/Camera2D.h"
#include "lupine/components/Camera3D.h"
#include "lupine/components/Skybox3D.h"
#include "lupine/resources/MeshLoader.h"
#include "lupine/resources/ResourceManager.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

namespace Lupine {

// Static member definitions
bool Renderer::s_initialized = false;
std::shared_ptr<GraphicsDevice> Renderer::s_graphics_device;
std::unique_ptr<LightingSystem> Renderer::s_lighting_system;
std::vector<RenderCommand> Renderer::s_render_commands;
glm::mat4 Renderer::s_view_matrix;
glm::mat4 Renderer::s_projection_matrix;
RenderingContext Renderer::s_rendering_context = RenderingContext::Runtime;

// Graphics abstraction resources
std::shared_ptr<GraphicsShader> Renderer::s_default_shader;
std::shared_ptr<GraphicsShader> Renderer::s_2d_shader;
std::shared_ptr<GraphicsShader> Renderer::s_text_shader;
std::shared_ptr<GraphicsShader> Renderer::s_skinned_mesh_shader;
std::shared_ptr<GraphicsVertexArray> Renderer::s_quad_vao;
std::shared_ptr<GraphicsBuffer> Renderer::s_quad_vbo;
std::shared_ptr<GraphicsTexture> Renderer::s_white_texture;

// Dynamic quad for texture regions and flipping
std::shared_ptr<GraphicsVertexArray> Renderer::s_dynamic_quad_vao;
std::shared_ptr<GraphicsBuffer> Renderer::s_dynamic_quad_vbo;
std::shared_ptr<GraphicsBuffer> Renderer::s_dynamic_quad_ebo;







bool Renderer::Initialize() {
    if (s_initialized) {
        return true;
    }

    std::cout << "Initializing Renderer..." << std::endl;

    // Create graphics device using the factory
    GraphicsDeviceFactory::DeviceParams params;
    params.preference = GraphicsDeviceFactory::BackendPreference::Auto;
    params.debug_mode = false;
    params.vsync = true;

    s_graphics_device = GraphicsDeviceFactory::CreateDevice(params);
    if (!s_graphics_device) {
        std::cerr << "Failed to create graphics device" << std::endl;
        std::cerr << "This usually means no graphics backends are compiled in or available." << std::endl;
        std::cerr << "Check that LUPINE_OPENGL_BACKEND or LUPINE_WEBGL_BACKEND is defined." << std::endl;
        return false;
    }

    // Initialize the graphics device
    if (!s_graphics_device->Initialize()) {
        std::cerr << "Failed to initialize graphics device" << std::endl;
        std::cerr << "Backend: " << s_graphics_device->GetCapabilities().backend_name << std::endl;
        std::cerr << "This usually means the graphics context is not properly set up." << std::endl;
        return false;
    }

    std::cout << "Graphics device initialized: " << s_graphics_device->GetCapabilities().backend_name << std::endl;

    // Set default render state using graphics device abstraction
    s_graphics_device->SetDepthTest(true);
    s_graphics_device->SetBlending(true);

    // Create default shaders
    std::cout << "Creating default shaders..." << std::endl;
    CreateDefaultShaders();

    // Create quad geometry for 2D rendering
    std::cout << "Creating quad geometry..." << std::endl;
    CreateQuadGeometry();

    // Create dynamic quad geometry for texture regions and flipping
    CreateDynamicQuadGeometry();

    // Create white texture
    CreateWhiteTexture();

    // Initialize lighting system
    std::cout << "Initializing lighting system..." << std::endl;
    s_lighting_system = std::make_unique<LightingSystem>();
    s_lighting_system->Initialize();

    s_initialized = true;
    std::cout << "Renderer initialized successfully!" << std::endl;
    return true;
}

bool Renderer::IsInitialized() {
    return s_initialized;
}

void Renderer::Shutdown() {
    if (!s_initialized) {
        return;
    }

    std::cout << "Shutting down Renderer..." << std::endl;

    if (s_lighting_system) {
        s_lighting_system->Shutdown();
        s_lighting_system.reset();
    }

    s_default_shader.reset();
    s_2d_shader.reset();
    s_render_commands.clear();

    // TODO: Clean up OpenGL resources when proper OpenGL is available

    s_initialized = false;
    std::cout << "Renderer shutdown complete." << std::endl;
}

void Renderer::BeginFrame(Camera* camera) {
    if (camera) {
        camera->UpdateMatrices();
        s_view_matrix = camera->GetViewMatrix();
        s_projection_matrix = camera->GetProjectionMatrix();
    }

    s_render_commands.clear();
}

void Renderer::EndFrame() {
    Flush();
}

void Renderer::Clear(const glm::vec4& color) {
    if (s_graphics_device) {
        s_graphics_device->Clear(color, true, true, false);
    }
}

void Renderer::RenderScene(Scene* scene, Camera* camera, bool clearScreen) {
    if (!scene || !camera) return;

    // Use single-pass rendering
    // Update lighting from scene
    UpdateLighting(scene);

    // Render shadow maps first
    if (s_lighting_system) {
        s_lighting_system->RenderShadowMaps(scene);
    }

    BeginFrame(camera);

    // Clear the screen only if requested (default behavior for standalone rendering)
    if (clearScreen) {
        Clear(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)); // Bright blue background
    }

    // Render skyboxes first (as background replacement)
    RenderSkyboxes(scene);

    // Traverse and render the scene tree
    Node* rootNode = scene->GetRootNode();
    if (rootNode) {
        RenderNode(rootNode);
    }

    EndFrame();
}

void Renderer::Submit(const RenderCommand& command) {
    s_render_commands.push_back(command);
}

void Renderer::RenderMesh(const Mesh& mesh, const glm::mat4& transform, const glm::vec4& color) {
    RenderCommand command;
    command.model_matrix = transform;
    // TODO: Convert mesh to use GraphicsVertexArray - for now create a temporary one
    // This is a temporary solution until Mesh structure is updated
    command.vertex_array = nullptr; // Will need to be properly implemented
    command.vertex_count = static_cast<unsigned int>(mesh.vertices.size());
    command.index_count = static_cast<unsigned int>(mesh.indices.size());
    command.color = color;
    command.texture = s_white_texture; // Use default white texture for proper color modulation
    command.use_indices = !mesh.indices.empty();

    Submit(command);
}

void Renderer::RenderMesh(const Mesh& mesh, const glm::mat4& transform, const glm::vec4& color, std::shared_ptr<GraphicsTexture> texture) {
    RenderCommand command;
    command.model_matrix = transform;
    // TODO: Convert mesh to use GraphicsVertexArray - for now create a temporary one
    command.vertex_array = nullptr; // Will need to be properly implemented
    command.vertex_count = static_cast<unsigned int>(mesh.vertices.size());
    command.index_count = static_cast<unsigned int>(mesh.indices.size());
    command.color = color;
    command.texture = texture ? texture : s_white_texture;
    command.use_indices = !mesh.indices.empty();

    Submit(command);
}

void Renderer::RenderMesh(const Mesh& mesh, const glm::mat4& transform, const glm::vec4& color, std::shared_ptr<GraphicsTexture> texture, bool use_lighting) {
    RenderCommand command;
    command.model_matrix = transform;
    // TODO: Convert mesh to use GraphicsVertexArray - for now create a temporary one
    command.vertex_array = nullptr; // Will need to be properly implemented
    command.vertex_count = static_cast<unsigned int>(mesh.vertices.size());
    command.index_count = static_cast<unsigned int>(mesh.indices.size());
    command.color = color;
    command.texture = texture ? texture : s_white_texture;
    command.use_indices = !mesh.indices.empty();
    command.use_lighting = use_lighting;

    Submit(command);
}

void Renderer::RenderSkinnedMesh(const Mesh& mesh, const glm::mat4& transform, const glm::vec4& color,
                                std::shared_ptr<GraphicsTexture> texture, const std::vector<glm::mat4>& bone_transforms,
                                bool use_lighting) {
    RenderCommand command;
    command.model_matrix = transform;
    // TODO: Convert mesh to use GraphicsVertexArray - for now create a temporary one
    command.vertex_array = nullptr; // Will need to be properly implemented
    command.vertex_count = static_cast<unsigned int>(mesh.vertices.size());
    command.index_count = static_cast<unsigned int>(mesh.indices.size());
    command.color = color;
    command.texture = texture ? texture : s_white_texture;
    command.use_indices = !mesh.indices.empty();
    command.use_lighting = use_lighting;
    command.use_skinned_mesh = true;
    command.bone_transforms = bone_transforms;
    command.bone_count = static_cast<unsigned int>(std::min(bone_transforms.size(), static_cast<size_t>(100))); // Limit to 100 bones

    Submit(command);
}

void Renderer::RenderQuad(const glm::mat4& transform, const glm::vec4& color, std::shared_ptr<GraphicsTexture> texture) {
    RenderCommand command;
    command.model_matrix = transform;
    command.vertex_array = s_quad_vao;
    command.vertex_count = 4;
    command.index_count = 6;
    command.color = color;
    command.texture = texture ? texture : s_white_texture;
    command.use_indices = true;

    Submit(command);
}

void Renderer::RenderQuad(const glm::mat4& transform, const glm::vec4& color, std::shared_ptr<GraphicsTexture> texture,
                         const glm::vec4& texture_region, bool flip_h, bool flip_v) {
    RenderCommand command;
    command.model_matrix = transform;
    command.vertex_array = s_dynamic_quad_vao;
    command.vertex_count = 4;
    command.index_count = 6;
    command.color = color;
    command.texture = texture ? texture : s_white_texture;
    command.use_indices = true;
    command.texture_region = texture_region;
    command.flip_h = flip_h;
    command.flip_v = flip_v;
    command.use_texture_region = true;

    Submit(command);
}

void Renderer::RenderRoundedQuad(const glm::mat4& transform, const glm::vec4& color,
                                float corner_radius, const glm::vec2& rect_size, std::shared_ptr<GraphicsTexture> texture) {
    RenderCommand command;
    command.model_matrix = transform;
    command.vertex_array = s_quad_vao;
    command.vertex_count = 4;
    command.index_count = 6;
    command.color = color;
    command.texture = texture ? texture : s_white_texture;
    command.use_indices = true;
    command.corner_radius = corner_radius;
    command.rect_size = rect_size;
    command.use_corner_radius = corner_radius > 0.0f;

    Submit(command);
}

void Renderer::RenderTextGlyph(const glm::mat4& transform, const glm::vec4& color, std::shared_ptr<GraphicsTexture> texture) {
    // Debug output for text rendering
    static int glyph_count = 0;
    if (++glyph_count % 60 == 0) { // Debug every 60 glyphs to avoid spam
        std::cout << "RenderTextGlyph: texture=" << (texture ? "valid" : "null")
                  << ", color=(" << color.r << "," << color.g << "," << color.b << "," << color.a << ")" << std::endl;
    }

    // Set up dynamic quad with correct UV coordinates for text glyphs
    SetupTextQuad();

    RenderCommand command;
    command.model_matrix = transform;
    command.vertex_array = s_dynamic_quad_vao;
    command.vertex_count = 4;
    command.index_count = 6;
    command.color = color;
    command.texture = texture ? texture : s_white_texture;
    command.use_indices = true;
    command.use_texture_region = false;
    command.use_text_shader = true; // Use specialized text shader
    command.flip_h = false;
    command.flip_v = false;
    command.reset_dynamic_quad = true; // Reset dynamic quad to default state after text rendering

    Submit(command);
}

void Renderer::RenderText(const std::string& text, const glm::vec2& position, float scale,
                         const glm::vec4& color, const std::string& font_path, int font_size) {
    // Initialize ResourceManager if not already done
    if (!ResourceManager::IsInitialized()) {
        ResourceManager::Initialize();
    }

    // Load font
    Font font = ResourceManager::LoadFont(font_path, font_size);
    if (!font.IsValid()) {
        return;
    }

    // Generate glyph atlas
    auto glyphs = ResourceManager::GenerateGlyphAtlas(font);
    if (glyphs.empty()) {
        return;
    }

    // Render each character
    float x = position.x;
    float y = position.y;

    for (char c : text) {
        if (c == '\n') {
            x = position.x;
            y += font_size * scale;
            continue;
        }

        auto glyph_it = glyphs.find(c);
        if (glyph_it == glyphs.end()) {
            continue; // Skip characters not in atlas
        }

        const Glyph& glyph = glyph_it->second;

        // Calculate glyph position using proper baseline alignment with context-aware scaling
        float dpi_scale_factor = TextRenderer::GetContextAwareScaleFactor(font);
        float glyph_x = x + (glyph.bearing.x * dpi_scale_factor * scale);
        // Position glyph relative to a consistent baseline
        // bearing.y (maxy) is the distance from baseline to top of glyph (positive upward)
        // baseline_to_bottom (miny) is the distance from baseline to bottom of glyph (negative for descenders)
        // In OpenGL coordinate system (Y up), position the texture's bottom-left corner at baseline + miny
        // For descenders, add a small adjustment to better align the main body
        float descender_adjustment = 0.0f;
        if (glyph.baseline_to_bottom < -2) { // Significant descender
            descender_adjustment = -glyph.baseline_to_bottom * 0.25f * dpi_scale_factor * scale; // 25% of descender depth
        }
        float glyph_y = y + (glyph.baseline_to_bottom * dpi_scale_factor * scale) + descender_adjustment;

        // Round positions to integers for pixel-perfect rendering
        glyph_x = std::round(glyph_x);
        glyph_y = std::round(glyph_y);

        // Create transform matrix for this glyph with proper scaling
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(glyph_x, glyph_y, 0.0f));
        transform = glm::scale(transform, glm::vec3(glyph.size.x * dpi_scale_factor * scale, glyph.size.y * dpi_scale_factor * scale, 1.0f));

        // Render glyph quad with high-quality text shader
        // TODO: Convert glyph.texture_id to GraphicsTexture - for now use white texture
        RenderTextGlyph(transform, color, GetWhiteTexture());

        // Advance to next character position with proper scaling
        x += (glyph.advance >> 6) * dpi_scale_factor * scale; // Convert from 1/64th pixels to pixels and scale
    }
}

void Renderer::CreateDefaultShaders() {
    if (!s_graphics_device) {
        std::cerr << "Cannot create shaders: Graphics device not initialized" << std::endl;
        return;
    }

    // Default 3D shader
    std::string vertex_3d = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord;

            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    std::string fragment_3d = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoord;

        uniform vec4 color;
        uniform sampler2D texture1;

        // Basic lighting
        uniform vec3 uAmbientLight;
        uniform bool uUseLighting;

        void main() {
            vec4 texColor = texture(texture1, TexCoord);
            vec4 finalColor = texColor * color;

            if (uUseLighting) {
                vec3 ambient = uAmbientLight;
                finalColor.rgb *= ambient;
            }

            FragColor = finalColor;
        }
    )";

    // Create default 3D shader using graphics device
    s_default_shader = s_graphics_device->CreateShader(vertex_3d, fragment_3d);
    if (!s_default_shader) {
        std::cerr << "Failed to create default 3D shader" << std::endl;
        return;
    }
    // 2D shader for UI and sprites
    std::string vertex_2d = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec2 TexCoord;

        void main() {
            TexCoord = aTexCoord;
            gl_Position = projection * view * model * vec4(aPos, 0.0, 1.0);
        }
    )";

    std::string fragment_2d = R"(
        #version 330 core
        out vec4 FragColor;

        in vec2 TexCoord;

        uniform vec4 color;
        uniform sampler2D texture1;

        void main() {
            vec4 texColor = texture(texture1, TexCoord);
            FragColor = texColor * color;

            if (FragColor.a < 0.01) {
                discard;
            }
        }
    )";

    // Create 2D shader using graphics device
    s_2d_shader = s_graphics_device->CreateShader(vertex_2d, fragment_2d);
    if (!s_2d_shader) {
        std::cerr << "Failed to create 2D shader" << std::endl;
        return;
    }

    // Text shader for text rendering
    std::string vertex_text = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec2 TexCoord;

        void main() {
            TexCoord = aTexCoord;
            gl_Position = projection * view * model * vec4(aPos, 0.0, 1.0);
        }
    )";

    std::string fragment_text = R"(
        #version 330 core
        out vec4 FragColor;

        in vec2 TexCoord;

        uniform vec4 color;
        uniform sampler2D texture1;

        void main() {
            float alpha = texture(texture1, TexCoord).r;
            FragColor = vec4(color.rgb, color.a * alpha);

            if (FragColor.a < 0.01) {
                discard;
            }
        }
    )";

    // Create text shader using graphics device
    s_text_shader = s_graphics_device->CreateShader(vertex_text, fragment_text);
    if (!s_text_shader) {
        std::cerr << "Failed to create text shader" << std::endl;
        return;
    }

    // Skinned mesh shader (simplified version of 3D shader)
    std::string vertex_skinned = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;
        layout (location = 3) in ivec4 aBoneIds;
        layout (location = 4) in vec4 aWeights;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform mat4 boneMatrices[100];

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;

        void main() {
            mat4 boneTransform = boneMatrices[aBoneIds[0]] * aWeights[0];
            boneTransform += boneMatrices[aBoneIds[1]] * aWeights[1];
            boneTransform += boneMatrices[aBoneIds[2]] * aWeights[2];
            boneTransform += boneMatrices[aBoneIds[3]] * aWeights[3];

            vec4 localPosition = boneTransform * vec4(aPos, 1.0);
            FragPos = vec3(model * localPosition);
            Normal = mat3(transpose(inverse(model * boneTransform))) * aNormal;
            TexCoord = aTexCoord;

            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    // Create skinned mesh shader using graphics device (reuse 3D fragment shader)
    s_skinned_mesh_shader = s_graphics_device->CreateShader(vertex_skinned, fragment_3d);
    if (!s_skinned_mesh_shader) {
        std::cerr << "Failed to create skinned mesh shader" << std::endl;
        return;
    }

    std::cout << "Default shaders created successfully" << std::endl;
}

void Renderer::CreateQuadGeometry() {
    if (!s_graphics_device) {
        std::cerr << "Cannot create quad geometry: Graphics device not initialized" << std::endl;
        return;
    }

    // Quad vertices (position + texture coordinates)
    // UV coordinates flipped vertically to match image coordinate system
    float vertices[] = {
        // positions   // texture coords
        -0.5f, -0.5f,  0.0f, 1.0f,  // bottom-left -> top-left in texture
         0.5f, -0.5f,  1.0f, 1.0f,  // bottom-right -> top-right in texture
         0.5f,  0.5f,  1.0f, 0.0f,  // top-right -> bottom-right in texture
        -0.5f,  0.5f,  0.0f, 0.0f   // top-left -> bottom-left in texture
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    // Create vertex buffer
    s_quad_vbo = s_graphics_device->CreateBuffer(
        BufferType::Vertex,
        BufferUsage::Static,
        sizeof(vertices),
        vertices
    );

    // Create index buffer
    auto index_buffer = s_graphics_device->CreateBuffer(
        BufferType::Index,
        BufferUsage::Static,
        sizeof(indices),
        indices
    );

    // Create vertex array
    s_quad_vao = s_graphics_device->CreateVertexArray();

    // Define vertex layout
    VertexLayout layout;
    layout.AddAttribute(0, 2, static_cast<uint32_t>(VertexAttributeType::Float), false);  // Position (2 floats)
    layout.AddAttribute(1, 2, static_cast<uint32_t>(VertexAttributeType::Float), false);  // Texture coordinates (2 floats)

    // Set up vertex array
    s_quad_vao->SetVertexBuffer(s_quad_vbo, layout);
    s_quad_vao->SetIndexBuffer(index_buffer);
}

void Renderer::CreateDynamicQuadGeometry() {
    if (!s_graphics_device) {
        std::cerr << "Cannot create dynamic quad geometry: Graphics device not initialized" << std::endl;
        return;
    }

    // Default quad vertices for dynamic rendering (UV coordinates flipped for images)
    float default_vertices[] = {
        // positions   // texture coords
        -0.5f, -0.5f,  0.0f, 1.0f,  // bottom-left -> top-left in texture
         0.5f, -0.5f,  1.0f, 1.0f,  // bottom-right -> top-right in texture
         0.5f,  0.5f,  1.0f, 0.0f,  // top-right -> bottom-right in texture
        -0.5f,  0.5f,  0.0f, 0.0f   // top-left -> bottom-left in texture
    };

    // Indices for quad (always the same)
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    // Create dynamic vertex buffer (will be updated for texture regions)
    s_dynamic_quad_vbo = s_graphics_device->CreateBuffer(
        BufferType::Vertex,
        BufferUsage::Dynamic,
        sizeof(default_vertices),
        default_vertices
    );

    // Create index buffer
    s_dynamic_quad_ebo = s_graphics_device->CreateBuffer(
        BufferType::Index,
        BufferUsage::Static,
        sizeof(indices),
        indices
    );

    // Create vertex array
    s_dynamic_quad_vao = s_graphics_device->CreateVertexArray();

    // Define vertex layout
    VertexLayout layout;
    layout.AddAttribute(0, 2, static_cast<uint32_t>(VertexAttributeType::Float), false);  // Position (2 floats)
    layout.AddAttribute(1, 2, static_cast<uint32_t>(VertexAttributeType::Float), false);  // Texture coordinates (2 floats)

    // Set up vertex array
    s_dynamic_quad_vao->SetVertexBuffer(s_dynamic_quad_vbo, layout);
    s_dynamic_quad_vao->SetIndexBuffer(s_dynamic_quad_ebo);
}

void Renderer::UpdateDynamicQuad(const glm::vec4& texture_region, bool flip_h, bool flip_v) {
    // Calculate UV coordinates based on texture region and flipping
    float u_min = texture_region.x;
    float v_min = texture_region.y;
    float u_max = texture_region.x + texture_region.z;
    float v_max = texture_region.y + texture_region.w;

    // Apply flipping
    if (flip_h) {
        std::swap(u_min, u_max);
    }
    if (flip_v) {
        std::swap(v_min, v_max);
    }

    // Quad vertices (position + texture coordinates)
    // UV coordinates flipped vertically to match image coordinate system
    float vertices[] = {
        // positions   // texture coords
        -0.5f, -0.5f,  u_min, v_min,  // bottom-left -> bottom of texture region
         0.5f, -0.5f,  u_max, v_min,  // bottom-right -> bottom of texture region
         0.5f,  0.5f,  u_max, v_max,  // top-right -> top of texture region
        -0.5f,  0.5f,  u_min, v_max   // top-left -> top of texture region
    };

    // Update VBO with new vertex data using graphics abstraction
    if (s_dynamic_quad_vbo) {
        s_dynamic_quad_vbo->UpdateData(0, sizeof(vertices), vertices);
    }
}

void Renderer::ResetDynamicQuad() {
    // Reset dynamic quad to default state (full texture, no flipping)
    float default_vertices[] = {
        // positions   // texture coords
        -0.5f, -0.5f,  0.0f, 1.0f,  // bottom-left -> top-left in texture
         0.5f, -0.5f,  1.0f, 1.0f,  // bottom-right -> top-right in texture
         0.5f,  0.5f,  1.0f, 0.0f,  // top-right -> bottom-right in texture
        -0.5f,  0.5f,  0.0f, 0.0f   // top-left -> bottom-left in texture
    };

    // Update VBO with default vertex data using graphics abstraction
    if (s_dynamic_quad_vbo) {
        s_dynamic_quad_vbo->UpdateData(0, sizeof(default_vertices), default_vertices);
    }
}

void Renderer::SetupTextQuad() {
    // Set up dynamic quad with correct UV coordinates for text glyphs
    // Text glyphs from SDL_TTF are rendered with Y=0 at top, Y=1 at bottom
    // But OpenGL texture coordinates have Y=0 at bottom, Y=1 at top
    // So we need to flip the V coordinate for text glyphs
    float text_vertices[] = {
        // positions   // texture coords (flipped V for text glyphs)
        -0.5f, -0.5f,  0.0f, 1.0f,  // bottom-left -> top-left in texture
         0.5f, -0.5f,  1.0f, 1.0f,  // bottom-right -> top-right in texture
         0.5f,  0.5f,  1.0f, 0.0f,  // top-right -> bottom-right in texture
        -0.5f,  0.5f,  0.0f, 0.0f   // top-left -> bottom-left in texture
    };

    // Update VBO with text-specific vertex data using graphics abstraction
    if (s_dynamic_quad_vbo) {
        s_dynamic_quad_vbo->UpdateData(0, sizeof(text_vertices), text_vertices);
    }
}

void Renderer::CreateWhiteTexture() {
    if (!s_graphics_device) {
        std::cerr << "Cannot create white texture: Graphics device not initialized" << std::endl;
        return;
    }

    // Create a 1x1 white texture
    unsigned char white_pixel[] = { 255, 255, 255, 255 };

    s_white_texture = s_graphics_device->CreateTexture2D(
        1, 1,
        TextureFormat::RGBA8,
        white_pixel
    );
}

void Renderer::Flush() {

    if (s_render_commands.empty()) {
        return;
    }

    // Sort render commands by shader type, texture, etc. for batching
    // For now, just render them in order

    static int flush_frame_counter = 0;
    bool should_debug = (++flush_frame_counter % 120 == 0); // Debug every 120 frames to see matrices

    int loop_count = 0;
    for (const auto& command : s_render_commands) {
        loop_count++;


        // Bind appropriate shader based on command type
        std::shared_ptr<GraphicsShader> shader = s_default_shader;

        if (command.use_skinned_mesh) {
            // Use skinned mesh shader for bone-based animation
            shader = s_skinned_mesh_shader;
            // For 3D rendering, enable depth testing
            if (s_graphics_device) {
                s_graphics_device->SetDepthTest(true);
            }
        } else if (command.use_text_shader) {
            // Use specialized text shader for high-quality text rendering
            shader = s_text_shader;
            if (s_graphics_device) {
                s_graphics_device->SetDepthTest(false);
                s_graphics_device->SetBlending(true);
            }
        } else if (command.vertex_array == s_quad_vao || command.vertex_array == s_dynamic_quad_vao) {
            shader = s_2d_shader;
            // For 2D rendering, disable depth testing entirely
            if (s_graphics_device) {
                s_graphics_device->SetDepthTest(false);
                s_graphics_device->SetBlending(true);
            }
        } else {
            // For 3D rendering, enable depth testing
            if (s_graphics_device) {
                s_graphics_device->SetDepthTest(true);
            }
        }

        if (shader) {
            shader->Use();

            // Bind lighting uniforms for 3D shaders
            if ((shader == s_default_shader || shader == s_skinned_mesh_shader) && s_lighting_system) {
                // TODO: Update lighting system to work with GraphicsShader
                // s_lighting_system->BindLightingUniforms(shader->GetNativeHandle());
                // Set lighting toggle uniform (use int since OpenGL treats booleans as integers)
                shader->SetInt("uUseLighting", command.use_lighting ? 1 : 0);
            }

            // Set bone transforms for skinned mesh shader
            if (shader == s_skinned_mesh_shader && command.use_skinned_mesh) {
                // Set bone transformation matrices
                for (unsigned int i = 0; i < command.bone_count && i < 100; i++) {
                    std::string uniform_name = "boneTransforms[" + std::to_string(i) + "]";
                    shader->SetMat4(uniform_name, command.bone_transforms[i]);
                }
            }

            // Update dynamic quad if needed (TODO: Remove direct OpenGL calls)
            if (command.vertex_array == s_dynamic_quad_vao && command.use_texture_region) {
                // TODO: Update this to use graphics abstraction
                // UpdateDynamicQuad(command.texture_region, command.flip_h, command.flip_v);
            }
        }

        if (shader) {
            // Set uniforms
            shader->SetMat4("model", command.model_matrix);
            shader->SetVec4("color", command.color);

            // Set uniforms - use the global matrices set by the camera system
            shader->SetMat4("view", s_view_matrix);
            shader->SetMat4("projection", s_projection_matrix);

            // Set corner radius uniforms for 2D shader
            if (shader == s_2d_shader) {
                shader->SetFloat("cornerRadius", command.corner_radius);
                shader->SetVec2("rectSize", command.rect_size);
                shader->SetBool("useCornerRadius", command.use_corner_radius);
            }

            // Bind texture
            if (command.texture && s_graphics_device) {
                // TODO: Implement texture binding through graphics abstraction
                // For now, skip texture binding
                shader->SetInt("texture1", 0);
            }

            // Bind vertex array and draw
            if (command.vertex_array && s_graphics_device) {
                command.vertex_array->Bind();

                if (command.use_indices && command.index_count > 0) {
                    s_graphics_device->DrawIndexed(PrimitiveType::Triangles, command.index_count);
                } else if (command.vertex_count > 0) {
                    s_graphics_device->Draw(PrimitiveType::Triangles, command.vertex_count);
                }

                command.vertex_array->Unbind();
            }

            // Reset dynamic quad if needed (for text rendering after sprite rendering)
            if (command.reset_dynamic_quad && command.vertex_array == s_dynamic_quad_vao) {
                // TODO: Update this to use graphics abstraction
                // ResetDynamicQuad();
            }
        }

        // TODO: Check for graphics errors through graphics abstraction
        // For now, skip error checking
    }

    // Restore depth testing
    if (s_graphics_device) {
        s_graphics_device->SetDepthTest(true);
    }

    s_render_commands.clear();
}

void Renderer::RenderNode(Node* node) {
    if (!node || !node->IsActive() || !node->IsVisible()) {
        return;
    }

    // Update all components of this node
    // Components handle their own rendering in OnUpdate
    const auto& components = node->GetAllComponents();
    for (const auto& component : components) {
        if (component && component->IsActive()) {
            // Components like Sprite2D and PrimitiveMesh render themselves in OnUpdate
            component->OnUpdate(0.0f); // Delta time not needed for rendering
        }
    }

    // Recursively render all children
    const auto& children = node->GetChildren();
    for (const auto& child : children) {
        RenderNode(child.get());
    }
}

void Renderer::SetRenderingContext(RenderingContext context) {
    s_rendering_context = context;
}

RenderingContext Renderer::GetRenderingContext() {
    return s_rendering_context;
}

std::shared_ptr<GraphicsTexture> Renderer::GetWhiteTexture() {
    return s_white_texture;
}

void Renderer::RenderSceneWithCameras(Scene* scene, bool clearScreen) {
    if (!scene) return;

    // Update lighting from scene
    UpdateLighting(scene);

    // Render shadow maps first
    if (s_lighting_system) {
        s_lighting_system->RenderShadowMaps(scene);
    }

    // Find active camera components
    auto [camera2d, camera3d] = FindActiveCameras(scene);



    // If no camera components found, use default rendering with proper projections
    if (!camera2d && !camera3d) {
        BeginFrame(nullptr);

        if (clearScreen) {
            Clear(glm::vec4(0.12f, 0.12f, 0.14f, 1.0f));
        }

        ViewportManager::ScreenBounds bounds = ViewportManager::GetCurrentBounds();
        glm::mat4 identity_view = glm::mat4(1.0f);

        // Render 3D content with default perspective projection
        glm::mat4 default_3d_projection = glm::perspective(
            glm::radians(45.0f),
            bounds.aspect_ratio,
            0.1f,
            1000.0f
        );
        s_view_matrix = glm::lookAt(
            glm::vec3(5.0f, 5.0f, 5.0f),   // Camera position (isometric-like view)
            glm::vec3(0.0f, 0.0f, 0.0f),   // Look at origin
            glm::vec3(0.0f, 1.0f, 0.0f)    // Up vector
        );
        s_projection_matrix = default_3d_projection;

        // Render skyboxes first (as background replacement)
        RenderSkyboxes(scene);

        RenderNodesByType(scene, nullptr, "3D");
        Flush(); // Flush 3D render commands

        // Render 2D content with default orthographic projection (matching scene view)
        glm::mat4 default_2d_projection = ViewportManager::Create2DWorldProjection(bounds);
        s_view_matrix = identity_view;
        s_projection_matrix = default_2d_projection;
        RenderNodesByType(scene, nullptr, "2D");
        Flush(); // Flush 2D render commands

        // Render Control content (world coordinates but no camera transform)
        glm::mat4 control_projection = ViewportManager::Create2DWorldProjection(bounds);
        s_view_matrix = identity_view;
        s_projection_matrix = control_projection;
        RenderNodesByType(scene, nullptr, "Control");
        Flush(); // Flush Control render commands

        // Note: We don't call EndFrame() here since we've already flushed all render commands
        return;
    }

    BeginFrame(nullptr);

    if (clearScreen) {
        Clear(glm::vec4(0.12f, 0.12f, 0.14f, 1.0f));
    }

    // Render in order: 3D -> 2D -> Control

    // 1. Render 3D content with Camera3D (or fallback)
    if (camera3d && camera3d->IsEnabled()) {
        // Update camera component to ensure proper projection setup
        camera3d->UpdateCamera();
        camera3d->GetCamera()->UpdateMatrices();
        s_view_matrix = camera3d->GetCamera()->GetViewMatrix();
        s_projection_matrix = camera3d->GetCamera()->GetProjectionMatrix();

        // Render skyboxes first (as background replacement)
        RenderSkyboxes(scene);

        RenderNodesByType(scene, camera3d->GetCamera(), "3D");
        Flush(); // Flush 3D render commands with 3D camera matrices
    } else {
        // Fallback: render 3D content with default perspective projection
        ViewportManager::ScreenBounds bounds = ViewportManager::GetCurrentBounds();
        glm::mat4 default_3d_projection = glm::perspective(
            glm::radians(45.0f),
            bounds.aspect_ratio,
            0.1f,
            1000.0f
        );
        s_view_matrix = glm::lookAt(
            glm::vec3(5.0f, 5.0f, 5.0f),   // Camera position (isometric-like view)
            glm::vec3(0.0f, 0.0f, 0.0f),   // Look at origin
            glm::vec3(0.0f, 1.0f, 0.0f)    // Up vector
        );
        s_projection_matrix = default_3d_projection;

        // Render skyboxes first (as background replacement)
        RenderSkyboxes(scene);

        RenderNodesByType(scene, nullptr, "3D");
        Flush(); // Flush 3D render commands with default 3D matrices
    }

    // 2. Render 2D content with Camera2D (or fallback)
    if (camera2d && camera2d->IsEnabled()) {
        // Update camera component to ensure proper projection setup
        camera2d->UpdateCamera();
        camera2d->GetCamera()->UpdateMatrices();
        s_view_matrix = camera2d->GetCamera()->GetViewMatrix();
        s_projection_matrix = camera2d->GetCamera()->GetProjectionMatrix();
        RenderNodesByType(scene, camera2d->GetCamera(), "2D");
        Flush(); // Flush 2D render commands with 2D camera matrices
    } else {
        // Fallback: render 2D content with default orthographic projection
        ViewportManager::ScreenBounds bounds = ViewportManager::GetCurrentBounds();
        glm::mat4 default_2d_projection = ViewportManager::Create2DWorldProjection(bounds);
        glm::mat4 identity_view = glm::mat4(1.0f);
        s_view_matrix = identity_view;
        s_projection_matrix = default_2d_projection;
        RenderNodesByType(scene, nullptr, "2D");
        Flush(); // Flush 2D render commands with default 2D matrices
    }

    // 3. Render Control content (world coordinates but no camera transform)
    glm::mat4 identity_view = glm::mat4(1.0f);
    ViewportManager::ScreenBounds bounds = ViewportManager::GetCurrentBounds();
    glm::mat4 control_projection = ViewportManager::Create2DWorldProjection(bounds);
    s_view_matrix = identity_view;
    s_projection_matrix = control_projection;
    RenderNodesByType(scene, nullptr, "Control");
    Flush(); // Flush Control render commands with world space projection but no camera transform

    // Note: We don't call EndFrame() here since we've already flushed all render commands
}

void Renderer::RenderSceneWithCameras(Scene* scene, Project* project, bool clearScreen) {
    if (!scene) return;

    // Update viewport manager with project settings
    ViewportManager::UpdateFromProject(project);

    // Use the standard method with updated viewport
    RenderSceneWithCameras(scene, clearScreen);
}

void Renderer::RenderNodesByType(Scene* scene, Camera* camera, const std::string& node_type) {
    (void)camera; // Suppress unused parameter warning
    if (!scene) return;

    // Start from root node and recursively render nodes of the specified type
    Node* rootNode = scene->GetRootNode();
    if (rootNode) {
        RenderNodesByTypeRecursive(rootNode, node_type);
    }
}

void Renderer::RenderNodesByTypeRecursive(Node* node, const std::string& node_type) {
    if (!node || !node->IsActive() || !node->IsVisible()) return;

    bool should_render = false;

    if (node_type == "3D" && dynamic_cast<Node3D*>(node)) {
        should_render = true;
    } else if (node_type == "2D" && dynamic_cast<Node2D*>(node)) {
        should_render = true;
    } else if (node_type == "Control" && dynamic_cast<Control*>(node)) {
        // Control nodes with world_space=false should render in the Control pass (screen space)
        Control* control = dynamic_cast<Control*>(node);
        if (control && !control->GetWorldSpace()) {
            should_render = true;
        }
    } else if (node_type == "2D" && dynamic_cast<Control*>(node)) {
        // Control nodes with world_space=true should render in the 2D pass
        Control* control = dynamic_cast<Control*>(node);
        if (control && control->GetWorldSpace()) {
            should_render = true;
        }
    }

    if (should_render) {
        // Render components of this node
        const auto& components = node->GetAllComponents();
        for (const auto& component : components) {
            if (component && component->IsActive()) {
                // Skip camera components and skybox components during regular rendering
                if (dynamic_cast<Camera2D*>(component.get()) || dynamic_cast<Camera3D*>(component.get()) ||
                    component->GetTypeName() == "Skybox3D") {
                    continue;
                }
                component->OnUpdate(0.0f); // Components render themselves in OnUpdate
            }
        }
    }

    // Recursively render all children regardless of parent type
    // This ensures that child nodes of different types are still rendered
    const auto& children = node->GetChildren();
    for (const auto& child : children) {
        RenderNodesByTypeRecursive(child.get(), node_type);
    }
}

std::pair<Camera2D*, Camera3D*> Renderer::FindActiveCameras(Scene* scene) {
    if (!scene) return {nullptr, nullptr};

    Camera2D* active_camera2d = nullptr;
    Camera3D* active_camera3d = nullptr;
    Camera2D* first_enabled_camera2d = nullptr;
    Camera3D* first_enabled_camera3d = nullptr;

    std::vector<Node*> all_nodes = scene->GetAllNodes();

    for (Node* node : all_nodes) {
        if (!node || !node->IsActive()) continue;

        // Look for Camera2D components
        if (auto* camera2d = node->GetComponent<Camera2D>()) {
            if (camera2d->IsEnabled()) {
                // Store first enabled camera as fallback
                if (!first_enabled_camera2d) {
                    first_enabled_camera2d = camera2d;
                }
                // Use current camera if explicitly marked
                if (camera2d->IsCurrent()) {
                    active_camera2d = camera2d;
                }
            }
        }

        // Look for Camera3D components
        if (auto* camera3d = node->GetComponent<Camera3D>()) {
            if (camera3d->IsEnabled()) {
                // Store first enabled camera as fallback
                if (!first_enabled_camera3d) {
                    first_enabled_camera3d = camera3d;
                }
                // Use current camera if explicitly marked
                if (camera3d->IsCurrent()) {
                    active_camera3d = camera3d;
                }
            }
        }
    }

    // If no camera is explicitly marked as current, use the first enabled camera
    if (!active_camera2d && first_enabled_camera2d) {
        active_camera2d = first_enabled_camera2d;
    }
    if (!active_camera3d && first_enabled_camera3d) {
        active_camera3d = first_enabled_camera3d;
    }

    return {active_camera2d, active_camera3d};
}

LightingSystem* Renderer::GetLightingSystem() {
    return s_lighting_system.get();
}

const glm::mat4& Renderer::GetViewMatrix() {
    return s_view_matrix;
}

const glm::mat4& Renderer::GetProjectionMatrix() {
    return s_projection_matrix;
}

void Renderer::RenderSkyboxes(Scene* scene) {
    if (!scene) return;


    // Find and render all skybox components in the scene
    RenderSkyboxesRecursive(scene->GetRootNode());
}

void Renderer::RenderSkyboxesRecursive(Node* node) {
    if (!node || !node->IsActive() || !node->IsVisible()) return;

    // Only process 3D nodes for skyboxes
    if (dynamic_cast<Node3D*>(node)) {
        const auto& components = node->GetAllComponents();
        for (const auto& component : components) {
            if (component && component->IsActive()) {
                // Check if this is a Skybox3D component
                if (component->GetTypeName() == "Skybox3D") {
                    // Cast to Skybox3D and call Render method
                    if (auto* skybox = dynamic_cast<Skybox3D*>(component.get())) {
                        // Update lighting system with skybox ambient contribution
                        skybox->UpdateLighting(s_lighting_system.get());
                        // Render the skybox
                        skybox->Render();
                    }
                }
            }
        }
    }

    // Recursively check all children
    const auto& children = node->GetChildren();
    for (const auto& child : children) {
        RenderSkyboxesRecursive(child.get());
    }
}

void Renderer::UpdateLighting(Scene* scene) {
    if (s_lighting_system) {
        s_lighting_system->UpdateLights(scene);
    }
}



} // namespace Lupine
