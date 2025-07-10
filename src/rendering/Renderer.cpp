#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/Camera.h"
#include "lupine/rendering/ViewportManager.h"
#include "lupine/rendering/LightingSystem.h"
#include "lupine/rendering/TextRenderer.h"
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
#include <iostream>

// Include proper OpenGL headers
#include <glad/glad.h>

namespace Lupine {

// Static member definitions
bool Renderer::s_initialized = false;
std::unique_ptr<Shader> Renderer::s_default_shader;
std::unique_ptr<Shader> Renderer::s_2d_shader;
std::unique_ptr<Shader> Renderer::s_text_shader;
std::unique_ptr<Shader> Renderer::s_skinned_mesh_shader;
std::unique_ptr<LightingSystem> Renderer::s_lighting_system;
std::vector<RenderCommand> Renderer::s_render_commands;
glm::mat4 Renderer::s_view_matrix;
glm::mat4 Renderer::s_projection_matrix;
RenderingContext Renderer::s_rendering_context = RenderingContext::Runtime;
unsigned int Renderer::s_quad_VAO = 0;
unsigned int Renderer::s_quad_VBO = 0;
unsigned int Renderer::s_white_texture = 0;

// Dynamic quad for texture regions and flipping
unsigned int Renderer::s_dynamic_quad_VAO = 0;
unsigned int Renderer::s_dynamic_quad_VBO = 0;
unsigned int Renderer::s_dynamic_quad_EBO = 0;



Shader::Shader(const std::string& vertex_source, const std::string& fragment_source) {
    // Compile vertex shader
    unsigned int vertex_shader = CompileShader(vertex_source, GL_VERTEX_SHADER);

    // Compile fragment shader
    unsigned int fragment_shader = CompileShader(fragment_source, GL_FRAGMENT_SHADER);

    // Create shader program
    m_program_id = glCreateProgram();
    glAttachShader(m_program_id, vertex_shader);
    glAttachShader(m_program_id, fragment_shader);
    glLinkProgram(m_program_id);

    CheckCompileErrors(m_program_id, "PROGRAM");

    // Delete shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

Shader::~Shader() {
    // TODO: Delete OpenGL program when proper OpenGL is available
    // glDeleteProgram(m_program_id);
}

void Shader::Use() const {
    glUseProgram(m_program_id);
}

void Shader::SetMatrix4(const std::string& name, const glm::mat4& matrix) const {
    int location = glGetUniformLocation(m_program_id, name.c_str());
    glUniformMatrix4fv(location, 1, false, &matrix[0][0]);
}

void Shader::SetVector2(const std::string& name, const glm::vec2& vector) const {
    int location = glGetUniformLocation(m_program_id, name.c_str());
    glUniform2fv(location, 1, &vector[0]);
}

void Shader::SetVector3(const std::string& name, const glm::vec3& vector) const {
    int location = glGetUniformLocation(m_program_id, name.c_str());
    glUniform3fv(location, 1, &vector[0]);
}

void Shader::SetVector4(const std::string& name, const glm::vec4& vector) const {
    int location = glGetUniformLocation(m_program_id, name.c_str());
    glUniform4fv(location, 1, &vector[0]);
}

void Shader::SetFloat(const std::string& name, float value) const {
    int location = glGetUniformLocation(m_program_id, name.c_str());
    glUniform1f(location, value);
}

void Shader::SetInt(const std::string& name, int value) const {
    int location = glGetUniformLocation(m_program_id, name.c_str());
    glUniform1i(location, value);
}

void Shader::SetBool(const std::string& name, bool value) const {
    int location = glGetUniformLocation(m_program_id, name.c_str());
    glUniform1i(location, value ? 1 : 0);
}

unsigned int Shader::CompileShader(const std::string& source, unsigned int type) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    CheckCompileErrors(shader, "SHADER");
    return shader;
}

void Shader::CheckCompileErrors(unsigned int shader, const std::string& type) {
    int success;
    char info_log[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, info_log);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << info_log << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, info_log);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << info_log << std::endl;
        }
    }
}

bool Renderer::Initialize() {
    if (s_initialized) {
        return true;
    }

    std::cout << "Initializing Renderer..." << std::endl;

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    // Disable alpha testing (we'll handle it in shader if needed)
    glDisable(GL_ALPHA_TEST);

    // Create default shaders
    CreateDefaultShaders();

    // Create quad geometry for 2D rendering
    CreateQuadGeometry();

    // Create dynamic quad geometry for texture regions and flipping
    CreateDynamicQuadGeometry();

    // Create white texture
    CreateWhiteTexture();

    // Initialize lighting system
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
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
    command.VAO = mesh.VAO;
    command.vertex_count = static_cast<unsigned int>(mesh.vertices.size());
    command.index_count = static_cast<unsigned int>(mesh.indices.size());
    command.color = color;
    command.texture_id = s_white_texture; // Use default white texture for proper color modulation
    command.use_indices = !mesh.indices.empty();

    Submit(command);
}

void Renderer::RenderMesh(const Mesh& mesh, const glm::mat4& transform, const glm::vec4& color, unsigned int texture_id) {
    RenderCommand command;
    command.model_matrix = transform;
    command.VAO = mesh.VAO;
    command.vertex_count = static_cast<unsigned int>(mesh.vertices.size());
    command.index_count = static_cast<unsigned int>(mesh.indices.size());
    command.color = color;
    command.texture_id = texture_id;
    command.use_indices = !mesh.indices.empty();

    Submit(command);
}

void Renderer::RenderMesh(const Mesh& mesh, const glm::mat4& transform, const glm::vec4& color, unsigned int texture_id, bool use_lighting) {
    RenderCommand command;
    command.model_matrix = transform;
    command.VAO = mesh.VAO;
    command.vertex_count = static_cast<unsigned int>(mesh.vertices.size());
    command.index_count = static_cast<unsigned int>(mesh.indices.size());
    command.color = color;
    command.texture_id = texture_id;
    command.use_indices = !mesh.indices.empty();
    command.use_lighting = use_lighting;

    Submit(command);
}

void Renderer::RenderSkinnedMesh(const Mesh& mesh, const glm::mat4& transform, const glm::vec4& color,
                                unsigned int texture_id, const std::vector<glm::mat4>& bone_transforms,
                                bool use_lighting) {
    RenderCommand command;
    command.model_matrix = transform;
    command.VAO = mesh.VAO;
    command.vertex_count = static_cast<unsigned int>(mesh.vertices.size());
    command.index_count = static_cast<unsigned int>(mesh.indices.size());
    command.color = color;
    command.texture_id = texture_id;
    command.use_indices = !mesh.indices.empty();
    command.use_lighting = use_lighting;
    command.use_skinned_mesh = true;
    command.bone_transforms = bone_transforms;
    command.bone_count = static_cast<unsigned int>(std::min(bone_transforms.size(), static_cast<size_t>(100))); // Limit to 100 bones

    Submit(command);
}

void Renderer::RenderQuad(const glm::mat4& transform, const glm::vec4& color, unsigned int texture_id) {
    RenderCommand command;
    command.model_matrix = transform;
    command.VAO = s_quad_VAO;
    command.vertex_count = 4;
    command.index_count = 6;
    command.color = color;
    command.texture_id = texture_id ? texture_id : s_white_texture;
    command.use_indices = true;

    Submit(command);
}

void Renderer::RenderQuad(const glm::mat4& transform, const glm::vec4& color, unsigned int texture_id,
                         const glm::vec4& texture_region, bool flip_h, bool flip_v) {
    RenderCommand command;
    command.model_matrix = transform;
    command.VAO = s_dynamic_quad_VAO;
    command.vertex_count = 4;
    command.index_count = 6;
    command.color = color;
    command.texture_id = texture_id ? texture_id : s_white_texture;
    command.use_indices = true;
    command.texture_region = texture_region;
    command.flip_h = flip_h;
    command.flip_v = flip_v;
    command.use_texture_region = true;

    Submit(command);
}

void Renderer::RenderRoundedQuad(const glm::mat4& transform, const glm::vec4& color,
                                float corner_radius, const glm::vec2& rect_size, unsigned int texture_id) {
    RenderCommand command;
    command.model_matrix = transform;
    command.VAO = s_quad_VAO;
    command.vertex_count = 4;
    command.index_count = 6;
    command.color = color;
    command.texture_id = texture_id ? texture_id : s_white_texture;
    command.use_indices = true;
    command.corner_radius = corner_radius;
    command.rect_size = rect_size;
    command.use_corner_radius = corner_radius > 0.0f;

    Submit(command);
}

void Renderer::RenderTextGlyph(const glm::mat4& transform, const glm::vec4& color, unsigned int texture_id) {
    // Debug output for text rendering
    static int glyph_count = 0;
    if (++glyph_count % 60 == 0) { // Debug every 60 glyphs to avoid spam
        std::cout << "RenderTextGlyph: texture_id=" << texture_id
                  << ", color=(" << color.r << "," << color.g << "," << color.b << "," << color.a << ")" << std::endl;
    }

    // Set up dynamic quad with correct UV coordinates for text glyphs
    SetupTextQuad();

    RenderCommand command;
    command.model_matrix = transform;
    command.VAO = s_dynamic_quad_VAO;
    command.vertex_count = 4;
    command.index_count = 6;
    command.color = color;
    command.texture_id = texture_id;
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
        RenderTextGlyph(transform, color, glyph.texture_id);

        // Advance to next character position with proper scaling
        x += (glyph.advance >> 6) * dpi_scale_factor * scale; // Convert from 1/64th pixels to pixels and scale
    }
}

void Renderer::CreateDefaultShaders() {
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

        // Lighting uniforms
        uniform vec3 uAmbientLight;
        uniform int uLightCount;
        uniform bool uUseLighting;
        uniform vec3 uCameraPos;

        // Fog uniforms
        uniform bool uFogEnabled;
        uniform vec3 uFogColor;
        uniform float uFogDensity;
        uniform float uFogStart;
        uniform float uFogEnd;
        uniform float uFogHeightFalloff;

        struct Light {
            vec3 position;
            float intensity;
            vec3 direction;
            float range;
            vec3 color;
            int type; // 0=Directional, 1=Point, 2=Spot
            float attenuation_constant;
            float attenuation_linear;
            float attenuation_quadratic;
            int casts_shadows; // 1 if this light casts shadows, 0 otherwise
            float inner_cone_angle; // Cosine of inner cone angle
            float outer_cone_angle; // Cosine of outer cone angle
            int shadow_map_index; // Index into shadow map array (-1 if no shadows)
            float shadow_bias; // Shadow bias to prevent acne
            float shadow_opacity; // Shadow opacity (0.0 to 1.0)
            vec3 shadow_color; // Shadow color (RGB)
        };

        uniform Light uLights[32];
        uniform sampler2DArray u_shadow_maps;
        uniform mat4 uLightSpaceMatrices[32];

        // Test single light uniforms for debugging
        uniform vec3 uTestLightColor;
        uniform float uTestLightIntensity;
        uniform bool uUseTestLight;

        float calculateShadow(Light light, vec3 fragPos, vec3 normal) {
            if (light.casts_shadows == 0 || light.shadow_map_index < 0) {
                return 0.0; // No shadow
            }

            // Transform fragment position to light space
            vec4 fragPosLightSpace = uLightSpaceMatrices[light.shadow_map_index] * vec4(fragPos, 1.0);

            // Perform perspective divide
            vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

            // Transform to [0,1] range
            projCoords = projCoords * 0.5 + 0.5;

            // Check if fragment is outside light's frustum
            if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 ||
                projCoords.y < 0.0 || projCoords.y > 1.0) {
                return 0.0; // No shadow
            }

            // PCF (Percentage Closer Filtering) for smooth shadows
            float shadow = 0.0;
            float currentDepth = projCoords.z;

            // Enhanced bias calculation to prevent shadow acne and peter-panning
            vec3 lightDir;
            if (light.type == 0) { // Directional light
                lightDir = normalize(-light.direction);
            } else if (light.type == 1) { // Point light
                lightDir = normalize(light.position - fragPos);
            } else { // Spot light
                lightDir = normalize(-light.direction);
            }

            // Improved adaptive bias with slope-scale bias and light-type specific adjustments
            float cosTheta = clamp(dot(normal, lightDir), 0.0, 1.0);
            float baseBias = light.shadow_bias;

            // Adjust bias based on light type
            if (light.type == 0) { // Directional light
                baseBias *= 1.5; // Directional lights need slightly more bias
            } else if (light.type == 1) { // Point light
                baseBias *= 2.0; // Point lights need more bias due to perspective projection
            } else { // Spot light
                baseBias *= 1.8; // Spot lights need moderate bias adjustment
            }

            float slopeBias = baseBias * sqrt(1.0 - cosTheta * cosTheta) / max(cosTheta, 0.1);
            float finalBias = clamp(baseBias + slopeBias, baseBias * 0.1, baseBias * 8.0);

            // Enhanced PCF sampling with Poisson disk pattern for better quality
            vec2 texelSize = 1.0 / textureSize(u_shadow_maps, 0).xy;

            // Enhanced Poisson disk samples for better quality
            vec2 poissonDisk[25] = vec2[](
                vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
                vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
                vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
                vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
                vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
                vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
                vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
                vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790),
                vec2(-0.65563464, 0.61860425), vec2(0.72781330, -0.31148052),
                vec2(-0.42543951, -0.81647956), vec2(0.13965822, 0.56789321),
                vec2(0.82134157, 0.34567891), vec2(-0.23456789, 0.87654321),
                vec2(0.56789012, -0.12345678), vec2(-0.78901234, -0.23456789),
                vec2(0.01234567, 0.98765432)
            );

            // Adaptive PCF sample count based on distance
            float distanceToLight = length(light.position - fragPos);
            int pcfSamples = light.type == 0 ? 25 : // Directional lights get full quality
                           (distanceToLight < light.range * 0.3) ? 25 : // Close objects get full quality
                           (distanceToLight < light.range * 0.6) ? 16 : // Medium distance gets good quality
                           9; // Far objects get basic quality

            float filterRadius = mix(1.5, 3.0, light.shadow_opacity); // Adaptive filter radius

            for (int i = 0; i < pcfSamples; ++i) {
                vec2 offset = poissonDisk[i] * texelSize * filterRadius;
                float pcfDepth = texture(u_shadow_maps, vec3(projCoords.xy + offset, float(light.shadow_map_index))).r;
                shadow += (currentDepth - finalBias > pcfDepth) ? 1.0 : 0.0;
            }
            shadow /= float(pcfSamples);

            // Apply shadow opacity
            shadow *= light.shadow_opacity;

            // Return shadow factor for color mixing
            return shadow;
        }

        vec3 calculateDirectionalLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
            vec3 lightDir = normalize(-light.direction);

            // Diffuse lighting
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = light.color * light.intensity * diff;

            // Specular lighting (Blinn-Phong)
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
            vec3 specular = light.color * light.intensity * spec * 0.3; // Reduced specular intensity

            // Shadow calculation
            float shadowFactor = calculateShadow(light, fragPos, normal);

            // Combine diffuse and specular
            vec3 lightContribution = diffuse + specular;

            // Apply shadows with proper color mixing
            vec3 shadowTint = light.shadow_color * 0.2;
            vec3 finalLighting = mix(lightContribution, shadowTint, shadowFactor * light.shadow_opacity);

            return finalLighting;
        }

        vec3 calculatePointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
            vec3 lightDir = normalize(light.position - fragPos);
            float distance = length(light.position - fragPos);

            // Check if fragment is within light range
            if (distance > light.range) {
                return vec3(0.0);
            }

            // Attenuation with smooth falloff near range limit
            float attenuation = 1.0 / (light.attenuation_constant +
                                     light.attenuation_linear * distance +
                                     light.attenuation_quadratic * distance * distance);

            // Smooth range falloff to prevent hard cutoff
            float rangeFalloff = 1.0 - smoothstep(light.range * 0.8, light.range, distance);
            attenuation *= rangeFalloff;

            // Diffuse lighting
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = light.color * light.intensity * diff * attenuation;

            // Specular lighting (Blinn-Phong)
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
            vec3 specular = light.color * light.intensity * spec * attenuation * 0.3;

            // Shadow calculation
            float shadowFactor = calculateShadow(light, fragPos, normal);

            // Combine diffuse and specular
            vec3 lightContribution = diffuse + specular;

            // Apply shadows with proper color mixing
            vec3 shadowTint = light.shadow_color * 0.2;
            vec3 finalLighting = mix(lightContribution, shadowTint * attenuation, shadowFactor * light.shadow_opacity);

            return finalLighting;
        }

        vec3 calculateSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
            vec3 lightDir = normalize(light.position - fragPos);
            float distance = length(light.position - fragPos);

            // Check if fragment is within light range
            if (distance > light.range) {
                return vec3(0.0);
            }

            // Check if fragment is within spotlight cone
            float theta = dot(lightDir, normalize(-light.direction));

            if (theta < light.outer_cone_angle) {
                return vec3(0.0); // Outside cone
            }

            // Smooth falloff between inner and outer cone with improved curve
            float epsilon = light.inner_cone_angle - light.outer_cone_angle;
            float intensity = clamp((theta - light.outer_cone_angle) / epsilon, 0.0, 1.0);
            intensity = smoothstep(0.0, 1.0, intensity); // Smoother cone falloff

            // Attenuation with smooth range falloff
            float attenuation = 1.0 / (light.attenuation_constant +
                                     light.attenuation_linear * distance +
                                     light.attenuation_quadratic * distance * distance);

            // Smooth range falloff to prevent hard cutoff
            float rangeFalloff = 1.0 - smoothstep(light.range * 0.8, light.range, distance);
            attenuation *= rangeFalloff;

            // Diffuse lighting
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = light.color * light.intensity * diff * attenuation * intensity;

            // Specular lighting (Blinn-Phong)
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
            vec3 specular = light.color * light.intensity * spec * attenuation * intensity * 0.3;

            // Shadow calculation
            float shadowFactor = calculateShadow(light, fragPos, normal);

            // Combine diffuse and specular
            vec3 lightContribution = diffuse + specular;

            // Apply shadows with proper color mixing
            vec3 shadowTint = light.shadow_color * 0.2;
            vec3 finalLighting = mix(lightContribution, shadowTint * attenuation * intensity, shadowFactor * light.shadow_opacity);

            return finalLighting;
        }

        float calculateFogFactor(vec3 fragPos) {
            if (!uFogEnabled) return 0.0;

            // Calculate distance from camera to fragment
            float distance = length(uCameraPos - fragPos);

            // Height-based fog calculation
            float height = fragPos.y - uCameraPos.y;
            float heightFactor = exp(-max(0.0, -height) * uFogHeightFalloff);

            // Distance-based fog calculation
            float fogFactor = 0.0;
            if (distance > uFogStart) {
                // Linear fog between start and end
                fogFactor = (distance - uFogStart) / (uFogEnd - uFogStart);
                fogFactor = clamp(fogFactor, 0.0, 1.0);

                // Apply exponential fog for more realistic falloff
                fogFactor = 1.0 - exp(-uFogDensity * distance * 0.01);
                fogFactor = clamp(fogFactor, 0.0, 1.0);
            }

            return fogFactor * heightFactor;
        }

        void main() {
            // Sample texture
            vec4 texColor = texture(texture1, TexCoord);

            vec3 result;

            if (uUseLighting) {
                // Normalize normal and calculate view direction
                vec3 norm = normalize(Normal);
                vec3 viewDir = normalize(uCameraPos - FragPos);

                // Start with ambient lighting
                result = uAmbientLight;

                // Add contribution from each light
                for (int i = 0; i < uLightCount && i < 32; ++i) {
                    Light light = uLights[i];

                    if (light.type == 0) {
                        // Directional light
                        result += calculateDirectionalLight(light, norm, FragPos, viewDir);
                    } else if (light.type == 1) {
                        // Point light
                        result += calculatePointLight(light, norm, FragPos, viewDir);
                    } else if (light.type == 2) {
                        // Spot light
                        result += calculateSpotLight(light, norm, FragPos, viewDir);
                    }
                }

                // Apply lighting to texture and color with improved color mixing
                result = result * texColor.rgb * color.rgb;

                // Ensure we don't exceed maximum brightness
                result = min(result, vec3(2.0));
            } else {
                // Unlit rendering - just use texture and color
                result = texColor.rgb * color.rgb;
            }

            // Apply fog if enabled
            if (uFogEnabled) {
                float fogFactor = calculateFogFactor(FragPos);
                result = mix(result, uFogColor, fogFactor);
            }

            FragColor = vec4(result, texColor.a * color.a);

            // Discard fully transparent pixels
            if (FragColor.a < 0.01) {
                discard;
            }
        }
    )";

    s_default_shader = std::make_unique<Shader>(vertex_3d, fragment_3d);

    // 2D shader with corner radius support
    std::string vertex_2d = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec2 TexCoord;
        out vec2 FragPos;

        void main() {
            TexCoord = aTexCoord;
            FragPos = aPos;
            gl_Position = projection * view * model * vec4(aPos, 0.0, 1.0);
        }
    )";

    std::string fragment_2d = R"(
        #version 330 core
        out vec4 FragColor;

        in vec2 TexCoord;
        in vec2 FragPos;

        uniform vec4 color;
        uniform sampler2D texture1;
        uniform float cornerRadius;
        uniform vec2 rectSize;
        uniform bool useCornerRadius;

        float roundedBoxSDF(vec2 centerPos, vec2 size, float radius) {
            return length(max(abs(centerPos) - size + radius, 0.0)) - radius;
        }

        void main() {
            vec4 texColor = texture(texture1, TexCoord);
            vec4 finalColor = texColor * color;

            // Apply corner radius if enabled
            if (useCornerRadius && cornerRadius > 0.0) {
                vec2 centerPos = FragPos - rectSize * 0.5;
                vec2 halfSize = rectSize * 0.5;
                float distance = roundedBoxSDF(centerPos, halfSize, cornerRadius);

                // Smooth anti-aliasing
                float alpha = 1.0 - smoothstep(-1.0, 1.0, distance);
                finalColor.a *= alpha;
            }

            FragColor = finalColor;

            // Discard fully transparent pixels for better alpha testing
            if (FragColor.a < 0.01) {
                discard;
            }
        }
    )";

    s_2d_shader = std::make_unique<Shader>(vertex_2d, fragment_2d);

    // Text shader for high-quality text rendering
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
            // Sample the glyph texture
            vec4 texColor = texture(texture1, TexCoord);

            // SDL_TTF renders glyphs as white pixels with varying alpha
            // Use the alpha channel directly for text rendering
            float textAlpha = texColor.a;

            // Apply the text color with the glyph alpha
            FragColor = vec4(color.rgb, textAlpha * color.a);

            // Discard fully transparent pixels for better alpha testing
            if (FragColor.a < 0.01) {
                discard;
            }
        }
    )";

    s_text_shader = std::make_unique<Shader>(vertex_text, fragment_text);

    // Skinned mesh shader for bone-based animation
    std::string vertex_skinned = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;
        layout (location = 3) in ivec4 aBoneIds;
        layout (location = 4) in vec4 aBoneWeights;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform mat4 boneTransforms[100]; // Support up to 100 bones

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;

        void main() {
            // Calculate bone transformation
            mat4 boneTransform = mat4(0.0);
            for(int i = 0; i < 4; i++) {
                if(aBoneIds[i] >= 0 && aBoneIds[i] < 100) {
                    boneTransform += boneTransforms[aBoneIds[i]] * aBoneWeights[i];
                }
            }

            // If no bone weights, use identity matrix
            if(boneTransform == mat4(0.0)) {
                boneTransform = mat4(1.0);
            }

            // Apply bone transformation to position and normal
            vec4 localPos = boneTransform * vec4(aPos, 1.0);
            vec3 localNormal = mat3(boneTransform) * aNormal;

            FragPos = vec3(model * localPos);
            Normal = mat3(transpose(inverse(model))) * localNormal;
            TexCoord = aTexCoord;

            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    // Use the same fragment shader as the default 3D shader
    s_skinned_mesh_shader = std::make_unique<Shader>(vertex_skinned, fragment_3d);
}

void Renderer::CreateQuadGeometry() {
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

    unsigned int VBO, EBO;

    glGenVertexArrays(1, &s_quad_VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(s_quad_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    s_quad_VBO = VBO;
}

void Renderer::CreateDynamicQuadGeometry() {
    // Create VAO, VBO, and EBO for dynamic quad
    glGenVertexArrays(1, &s_dynamic_quad_VAO);
    glGenBuffers(1, &s_dynamic_quad_VBO);
    glGenBuffers(1, &s_dynamic_quad_EBO);

    glBindVertexArray(s_dynamic_quad_VAO);

    // Default quad vertices for dynamic rendering (UV coordinates flipped for images)
    float default_vertices[] = {
        // positions   // texture coords
        -0.5f, -0.5f,  0.0f, 1.0f,  // bottom-left -> top-left in texture
         0.5f, -0.5f,  1.0f, 1.0f,  // bottom-right -> top-right in texture
         0.5f,  0.5f,  1.0f, 0.0f,  // top-right -> bottom-right in texture
        -0.5f,  0.5f,  0.0f, 0.0f   // top-left -> bottom-left in texture
    };

    // Bind VBO with default data (will be updated dynamically for texture regions)
    glBindBuffer(GL_ARRAY_BUFFER, s_dynamic_quad_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(default_vertices), default_vertices, GL_DYNAMIC_DRAW);

    // Indices for quad (always the same)
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_dynamic_quad_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
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

    // Update VBO with new vertex data
    glBindBuffer(GL_ARRAY_BUFFER, s_dynamic_quad_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
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

    // Update VBO with default vertex data
    glBindBuffer(GL_ARRAY_BUFFER, s_dynamic_quad_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(default_vertices), default_vertices);
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

    // Update VBO with text-specific vertex data
    glBindBuffer(GL_ARRAY_BUFFER, s_dynamic_quad_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(text_vertices), text_vertices);
}

void Renderer::CreateWhiteTexture() {
    glGenTextures(1, &s_white_texture);
    glBindTexture(GL_TEXTURE_2D, s_white_texture);

    // Create a 1x1 white texture
    unsigned char white_pixel[] = { 255, 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
        Shader* shader = s_default_shader.get();

        if (command.use_skinned_mesh) {
            // Use skinned mesh shader for bone-based animation
            shader = s_skinned_mesh_shader.get();
            // For 3D rendering, enable depth testing
            glEnable(GL_DEPTH_TEST);
        } else if (command.use_text_shader) {
            // Use specialized text shader for high-quality text rendering
            shader = s_text_shader.get();
            glDisable(GL_DEPTH_TEST);
            // Enable proper alpha blending for text
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else if (command.VAO == s_quad_VAO || command.VAO == s_dynamic_quad_VAO) {
            shader = s_2d_shader.get();
            // For 2D rendering, disable depth testing entirely
            glDisable(GL_DEPTH_TEST);
            // Enable proper alpha blending for 2D UI elements
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            // For 3D rendering, enable depth testing
            glEnable(GL_DEPTH_TEST);
        }

        shader->Use();

        // Bind lighting uniforms for 3D shaders
        if ((shader == s_default_shader.get() || shader == s_skinned_mesh_shader.get()) && s_lighting_system) {
            s_lighting_system->BindLightingUniforms(shader->GetProgramID());
            // Set lighting toggle uniform (use int since OpenGL treats booleans as integers)
            shader->SetInt("uUseLighting", command.use_lighting ? 1 : 0);
        }

        // Set bone transforms for skinned mesh shader
        if (shader == s_skinned_mesh_shader.get() && command.use_skinned_mesh) {
            // Set bone transformation matrices
            for (unsigned int i = 0; i < command.bone_count && i < 100; i++) {
                std::string uniform_name = "boneTransforms[" + std::to_string(i) + "]";
                shader->SetMatrix4(uniform_name, command.bone_transforms[i]);
            }
        }

        // Update dynamic quad if needed
        if (command.VAO == s_dynamic_quad_VAO && command.use_texture_region) {
            UpdateDynamicQuad(command.texture_region, command.flip_h, command.flip_v);
        }

        // Set uniforms
        shader->SetMatrix4("model", command.model_matrix);
        shader->SetVector4("color", command.color);

        // Set uniforms - use the global matrices set by the camera system
        shader->SetMatrix4("view", s_view_matrix);
        shader->SetMatrix4("projection", s_projection_matrix);

        // Set corner radius uniforms for 2D shader
        if (shader == s_2d_shader.get()) {
            shader->SetFloat("cornerRadius", command.corner_radius);
            shader->SetVector2("rectSize", command.rect_size);
            shader->SetBool("useCornerRadius", command.use_corner_radius);
        }

        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, command.texture_id);
        shader->SetInt("texture1", 0);

        // Bind VAO and draw
        glBindVertexArray(command.VAO);

        if (command.use_indices && command.index_count > 0) {
            glDrawElements(GL_TRIANGLES, command.index_count, GL_UNSIGNED_INT, 0);
        } else if (command.vertex_count > 0) {
            glDrawArrays(GL_TRIANGLES, 0, command.vertex_count);
        }

        // Reset dynamic quad if needed (for text rendering after sprite rendering)
        if (command.reset_dynamic_quad && command.VAO == s_dynamic_quad_VAO) {
            ResetDynamicQuad();
        }

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR && loop_count % 120 == 0) {
            std::cout << "[ERROR] OpenGL error after draw call: " << error << std::endl;
        }
    }

    // Restore depth testing
    glEnable(GL_DEPTH_TEST);

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

unsigned int Renderer::GetWhiteTexture() {
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
