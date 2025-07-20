#include "lupine/components/Skybox3D.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/LightingSystem.h"
#include "lupine/rendering/GraphicsShader.h"
#include "lupine/rendering/GraphicsDevice.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/nodes/Node3D.h"
#include <glad/glad.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Lupine {

// Static member definitions
std::shared_ptr<GraphicsShader> Skybox3D::s_skybox_shader = nullptr;
std::shared_ptr<GraphicsShader> Skybox3D::s_procedural_sky_shader = nullptr;
bool Skybox3D::s_shaders_initialized = false;

Skybox3D::Skybox3D()
    : Component("Skybox3D")
    , m_skybox_mode(SkyboxMode::SolidColor)
    , m_background_color(0.5f, 0.7f, 1.0f, 1.0f) // Light blue sky
    , m_texture_path("")
    , m_texture_rotation(0.0f)
    , m_exposure(1.0f)
    , m_sun_enabled(true)
    , m_sun_color(1.0f, 0.95f, 0.8f, 1.0f) // Warm sun color
    , m_sun_intensity(1.0f)
    , m_shadow_color(0.3f, 0.4f, 0.6f, 1.0f) // Cool shadow color
    , m_sun_rotation(-30.0f, 45.0f) // Default sun position (pitch=-30°, yaw=45°)
    , m_fog_enabled(false)
    , m_fog_color(0.7f, 0.8f, 0.9f, 1.0f) // Light blue-gray fog
    , m_fog_density(0.02f)
    , m_fog_start(10.0f)
    , m_fog_end(100.0f)
    , m_fog_height_falloff(0.1f)
    , m_skybox_VAO(0)
    , m_skybox_VBO(0)
    , m_texture_id(0)
    , m_texture_loaded(false)
    , m_mesh_initialized(false) {

    // Initialize export variables
    InitializeExportVariables();
}

Skybox3D::~Skybox3D() {
    CleanupResources();
}

void Skybox3D::SetSkyboxMode(SkyboxMode mode) {
    m_skybox_mode = mode;
    SetExportVariable("skybox_mode", SkyboxModeToInt(mode));
    
    // Load texture if switching to texture mode
    if (mode == SkyboxMode::PanoramicImage || mode == SkyboxMode::Cubemap) {
        LoadTexture();
    }
}

void Skybox3D::SetBackgroundColor(const glm::vec4& color) {
    m_background_color = color;
    SetExportVariable("background_color", color);
}

void Skybox3D::SetTexturePath(const std::string& path) {
    m_texture_path = path;
    SetExportVariable("texture_path", path);
    m_texture_loaded = false;
    LoadTexture();
}

void Skybox3D::SetSunEnabled(bool enabled) {
    m_sun_enabled = enabled;
    SetExportVariable("sun_enabled", enabled);
}

void Skybox3D::SetSunColor(const glm::vec3& color) {
    m_sun_color = glm::vec4(color, 1.0f);
    SetExportVariable("sun_color", m_sun_color);
}

void Skybox3D::SetSunIntensity(float intensity) {
    m_sun_intensity = std::max(0.0f, intensity);
    SetExportVariable("sun_intensity", m_sun_intensity);
}

void Skybox3D::SetShadowColor(const glm::vec3& color) {
    m_shadow_color = glm::vec4(color, 1.0f);
    SetExportVariable("shadow_color", m_shadow_color);
}

void Skybox3D::SetSunRotation(const glm::vec2& rotation) {
    m_sun_rotation = rotation;
    SetExportVariable("sun_rotation", rotation);
}

glm::vec3 Skybox3D::GetSunDirection() const {
    // Calculate sun direction from pitch and yaw angles
    float pitch = glm::radians(m_sun_rotation.x);
    float yaw = glm::radians(m_sun_rotation.y);

    // Convert spherical coordinates to cartesian
    glm::vec3 direction;
    direction.x = cos(pitch) * sin(yaw);
    direction.y = sin(pitch);
    direction.z = cos(pitch) * cos(yaw);

    return glm::normalize(direction);
}

void Skybox3D::SetTextureRotation(float rotation) {
    m_texture_rotation = rotation;
    SetExportVariable("texture_rotation", rotation);
}

void Skybox3D::SetExposure(float exposure) {
    m_exposure = std::max(0.0f, exposure);
    SetExportVariable("exposure", m_exposure);
}

void Skybox3D::SetFogEnabled(bool enabled) {
    m_fog_enabled = enabled;
    SetExportVariable("fog_enabled", enabled);
}

void Skybox3D::SetFogColor(const glm::vec3& color) {
    glm::vec3 clamped_color = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
    m_fog_color = glm::vec4(clamped_color, 1.0f);
    SetExportVariable("fog_color", m_fog_color);
}

void Skybox3D::SetFogDensity(float density) {
    m_fog_density = std::max(0.0f, density);
    SetExportVariable("fog_density", m_fog_density);
}

void Skybox3D::SetFogStart(float start) {
    m_fog_start = std::max(0.0f, start);
    SetExportVariable("fog_start", m_fog_start);
}

void Skybox3D::SetFogEnd(float end) {
    m_fog_end = std::max(m_fog_start + 0.1f, end);
    SetExportVariable("fog_end", m_fog_end);
}

void Skybox3D::SetFogHeightFalloff(float falloff) {
    m_fog_height_falloff = std::max(0.0f, falloff);
    SetExportVariable("fog_height_falloff", m_fog_height_falloff);
}

void Skybox3D::OnReady() {
    InitializeMesh();
    InitializeShaders();
    LoadTexture();
}

void Skybox3D::OnUpdate(float delta_time) {
    (void)delta_time;

    // Update from export variables if they've changed
    UpdateFromExportVariables();

    if (!m_mesh_initialized) {
        InitializeMesh();
    }

    if (!s_shaders_initialized) {
        InitializeShaders();
    }
}

void Skybox3D::Render() {


    // Render skybox based on current mode
    switch (m_skybox_mode) {
        case SkyboxMode::SolidColor:
            RenderSolidColor();
            break;
        case SkyboxMode::PanoramicImage:
        case SkyboxMode::Cubemap:
            RenderPanoramicTexture();
            break;
        case SkyboxMode::ProceduralSky:
            RenderProceduralSky();
            break;
    }
}

void Skybox3D::InitializeExportVariables() {
    // Create enum options for skybox mode
    std::vector<std::string> skyboxModeOptions = {
        "Solid Color", "Panoramic Image", "Cubemap", "Procedural Sky"
    };

    AddEnumExportVariable("skybox_mode", SkyboxModeToInt(m_skybox_mode), "Skybox rendering mode", skyboxModeOptions);
    AddExportVariable("background_color", m_background_color, "Background color (RGBA)", ExportVariableType::Color);
    AddExportVariable("texture_path", m_texture_path, "Panoramic texture file path", ExportVariableType::FilePath);
    AddExportVariable("texture_rotation", m_texture_rotation, "Texture rotation in degrees", ExportVariableType::Float);
    AddExportVariable("exposure", m_exposure, "HDR exposure multiplier", ExportVariableType::Float);
    
    AddExportVariable("sun_enabled", m_sun_enabled, "Enable sun lighting", ExportVariableType::Bool);
    AddExportVariable("sun_color", m_sun_color, "Sun color (RGB)", ExportVariableType::Color);
    AddExportVariable("sun_intensity", m_sun_intensity, "Sun intensity multiplier", ExportVariableType::Float);
    AddExportVariable("shadow_color", m_shadow_color, "Shadow color (RGB)", ExportVariableType::Color);

    // Fog properties
    AddExportVariable("fog_enabled", m_fog_enabled, "Enable distance fog", ExportVariableType::Bool);
    AddExportVariable("fog_color", m_fog_color, "Fog color (RGB)", ExportVariableType::Color);
    AddExportVariable("fog_density", m_fog_density, "Fog density factor", ExportVariableType::Float);
    AddExportVariable("fog_start", m_fog_start, "Distance where fog starts", ExportVariableType::Float);
    AddExportVariable("fog_end", m_fog_end, "Distance where fog reaches maximum", ExportVariableType::Float);
    AddExportVariable("fog_height_falloff", m_fog_height_falloff, "Height-based fog falloff", ExportVariableType::Float);
}

void Skybox3D::UpdateExportVariables() {
    SetExportVariable("skybox_mode", SkyboxModeToInt(m_skybox_mode));
    SetExportVariable("background_color", m_background_color);
    SetExportVariable("texture_path", m_texture_path);
    SetExportVariable("texture_rotation", m_texture_rotation);
    SetExportVariable("exposure", m_exposure);
    SetExportVariable("sun_enabled", m_sun_enabled);
    SetExportVariable("sun_color", m_sun_color);
    SetExportVariable("sun_intensity", m_sun_intensity);
    SetExportVariable("shadow_color", m_shadow_color);
    SetExportVariable("sun_rotation", m_sun_rotation);

    // Fog properties
    SetExportVariable("fog_enabled", m_fog_enabled);
    SetExportVariable("fog_color", m_fog_color);
    SetExportVariable("fog_density", m_fog_density);
    SetExportVariable("fog_start", m_fog_start);
    SetExportVariable("fog_end", m_fog_end);
    SetExportVariable("fog_height_falloff", m_fog_height_falloff);
}

void Skybox3D::UpdateFromExportVariables() {
    // Check if skybox mode changed
    int mode_int = GetExportVariableValue<int>("skybox_mode", SkyboxModeToInt(m_skybox_mode));
    SkyboxMode new_mode = IntToSkyboxMode(mode_int);
    if (new_mode != m_skybox_mode) {
        m_skybox_mode = new_mode;
        if (new_mode == SkyboxMode::PanoramicImage || new_mode == SkyboxMode::Cubemap) {
            LoadTexture();
        }
    }

    // Check if texture path changed
    std::string new_texture_path = GetExportVariableValue<std::string>("texture_path", m_texture_path);
    if (new_texture_path != m_texture_path) {
        m_texture_path = new_texture_path;
        m_texture_loaded = false;
        LoadTexture();
    }

    // Update other properties
    m_background_color = GetExportVariableValue<glm::vec4>("background_color", m_background_color);
    m_texture_rotation = GetExportVariableValue<float>("texture_rotation", m_texture_rotation);
    m_exposure = GetExportVariableValue<float>("exposure", m_exposure);
    m_sun_enabled = GetExportVariableValue<bool>("sun_enabled", m_sun_enabled);
    m_sun_color = GetExportVariableValue<glm::vec4>("sun_color", m_sun_color);
    m_sun_intensity = GetExportVariableValue<float>("sun_intensity", m_sun_intensity);
    m_shadow_color = GetExportVariableValue<glm::vec4>("shadow_color", m_shadow_color);
    m_sun_rotation = GetExportVariableValue<glm::vec2>("sun_rotation", m_sun_rotation);

    // Fog properties
    m_fog_enabled = GetExportVariableValue<bool>("fog_enabled", m_fog_enabled);
    m_fog_color = GetExportVariableValue<glm::vec4>("fog_color", m_fog_color);
    m_fog_density = GetExportVariableValue<float>("fog_density", m_fog_density);
    m_fog_start = GetExportVariableValue<float>("fog_start", m_fog_start);
    m_fog_end = GetExportVariableValue<float>("fog_end", m_fog_end);
    m_fog_height_falloff = GetExportVariableValue<float>("fog_height_falloff", m_fog_height_falloff);
}

int Skybox3D::SkyboxModeToInt(SkyboxMode mode) const {
    return static_cast<int>(mode);
}

Skybox3D::SkyboxMode Skybox3D::IntToSkyboxMode(int value) const {
    if (value >= 0 && value <= static_cast<int>(SkyboxMode::ProceduralSky)) {
        return static_cast<SkyboxMode>(value);
    }
    return SkyboxMode::SolidColor;
}

void Skybox3D::InitializeMesh() {
    if (m_mesh_initialized) {
        return;
    }

    // Create skybox cube vertices - simple cube with correct winding order
    // For skybox, we want to see the inside faces, so we use clockwise winding
    float skyboxVertices[] = {
        // Front face (Z+)
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        // Back face (Z-)
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        // Left face (X-)
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,

        // Right face (X+)
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,

        // Top face (Y+)
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,

        // Bottom face (Y-)
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f
    };

    // Generate and bind VAO
    glGenVertexArrays(1, &m_skybox_VAO);
    glGenBuffers(1, &m_skybox_VBO);

    glBindVertexArray(m_skybox_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_skybox_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
    m_mesh_initialized = true;
}

void Skybox3D::LoadTexture() {
    if (m_texture_path.empty() ||
        (m_skybox_mode != SkyboxMode::PanoramicImage && m_skybox_mode != SkyboxMode::Cubemap)) {
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
        std::cout << "Skybox3D: Loaded texture " << m_texture_path << std::endl;
    } else {
        m_texture_id = 0;
        m_texture_loaded = false;
        std::cerr << "Skybox3D: Failed to load texture " << m_texture_path << std::endl;
    }
}

void Skybox3D::CleanupResources() {
    if (m_skybox_VAO != 0) {
        glDeleteVertexArrays(1, &m_skybox_VAO);
        m_skybox_VAO = 0;
    }
    if (m_skybox_VBO != 0) {
        glDeleteBuffers(1, &m_skybox_VBO);
        m_skybox_VBO = 0;
    }
    m_mesh_initialized = false;
}

void Skybox3D::SetFogUniforms() {
    if (!s_skybox_shader) return;

    // Set fog parameters
    s_skybox_shader->SetInt("fogEnabled", m_fog_enabled ? 1 : 0);
    s_skybox_shader->SetVec3("fogColor", m_fog_color);
    s_skybox_shader->SetFloat("fogDensity", m_fog_density);
    s_skybox_shader->SetFloat("fogStart", m_fog_start);
    s_skybox_shader->SetFloat("fogEnd", m_fog_end);
    s_skybox_shader->SetFloat("fogHeightFalloff", m_fog_height_falloff);

    // Set camera position for fog calculations
    glm::mat4 view_matrix = Renderer::GetViewMatrix();
    glm::mat4 inverse_view = glm::inverse(view_matrix);
    glm::vec3 camera_pos = glm::vec3(inverse_view[3]);
    s_skybox_shader->SetVec3("cameraPosition", camera_pos);
}

void Skybox3D::InitializeShaders() {
    if (s_shaders_initialized) {
        return;
    }



    // Skybox vertex shader
    std::string skybox_vertex = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        out vec3 TexCoords;

        uniform mat4 projection;
        uniform mat4 view;

        void main() {
            TexCoords = aPos;
            vec4 pos = projection * view * vec4(aPos, 1.0);
            gl_Position = pos.xyww; // Set z to w for maximum depth
        }
    )";

    // Skybox fragment shader
    std::string skybox_fragment = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 TexCoords;

        uniform int skyboxMode; // 0=solid, 1=panoramic, 2=cubemap, 3=procedural
        uniform vec4 backgroundColor;
        uniform sampler2D panoramicTexture;
        uniform samplerCube cubemapTexture;
        uniform float textureRotation;
        uniform float exposure;

        // Procedural sky uniforms
        uniform bool sunEnabled;
        uniform vec3 sunDirection;
        uniform vec3 sunColor;
        uniform float sunIntensity;
        uniform vec3 shadowColor;

        // Fog uniforms
        uniform bool fogEnabled;
        uniform vec3 fogColor;
        uniform float fogDensity;
        uniform float fogStart;
        uniform float fogEnd;
        uniform float fogHeightFalloff;
        uniform vec3 cameraPosition;

        vec2 SampleSphericalMap(vec3 v) {
            vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
            uv *= vec2(0.1591, 0.3183); // 1/(2*PI), 1/PI
            uv += 0.5;
            return uv;
        }

        vec3 ProceduralSky(vec3 direction) {
            float sunDot = dot(direction, normalize(sunDirection));
            float sunFactor = pow(max(sunDot, 0.0), 32.0);

            // Sky gradient
            float skyFactor = max(direction.y, 0.0);
            vec3 skyColor = mix(shadowColor, sunColor, skyFactor);

            // Add sun disk
            if (sunEnabled) {
                skyColor += sunColor * sunFactor * sunIntensity;
            }

            return skyColor;
        }

        float CalculateFogFactor(vec3 direction) {
            if (!fogEnabled) return 0.0;

            // Calculate distance to the point on the skybox
            // For skybox, we simulate infinite distance with a large value
            float distance = 1000.0; // Large distance for skybox

            // Height-based fog calculation
            float height = direction.y;
            float heightFactor = exp(-max(0.0, -height) * fogHeightFalloff);

            // Distance-based fog calculation
            float fogFactor = 0.0;
            if (distance > fogStart) {
                fogFactor = (distance - fogStart) / (fogEnd - fogStart);
                fogFactor = clamp(fogFactor, 0.0, 1.0);

                // Apply exponential fog for more realistic falloff
                fogFactor = 1.0 - exp(-fogDensity * distance * 0.01);
            }

            return fogFactor * heightFactor;
        }

        void main() {
            vec3 direction = normalize(TexCoords);
            vec4 skyColor;

            if (skyboxMode == 0) {
                // Solid color
                skyColor = backgroundColor;
            } else if (skyboxMode == 1) {
                // Panoramic texture
                vec2 uv = SampleSphericalMap(direction);
                // Apply rotation
                float rad = radians(textureRotation);
                uv.x += rad / (2.0 * 3.14159);
                skyColor = texture(panoramicTexture, uv) * exposure;
            } else if (skyboxMode == 2) {
                // Cubemap texture
                skyColor = texture(cubemapTexture, direction) * exposure;
            } else if (skyboxMode == 3) {
                // Procedural sky
                vec3 proceduralColor = ProceduralSky(direction);
                skyColor = vec4(proceduralColor, 1.0);
            } else {
                skyColor = backgroundColor;
            }

            // Apply fog if enabled
            if (fogEnabled) {
                float fogFactor = CalculateFogFactor(direction);
                skyColor.rgb = mix(skyColor.rgb, fogColor, fogFactor);
            }

            FragColor = skyColor;
        }
    )";

    auto graphics_device = Renderer::GetGraphicsDevice();
    if (graphics_device) {
        s_skybox_shader = graphics_device->CreateShader(skybox_vertex, skybox_fragment);
    }



    s_shaders_initialized = true;
}

void Skybox3D::RenderSolidColor() {

    if (!s_skybox_shader || !m_mesh_initialized) return;

    // Disable depth testing and writing for skybox
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    s_skybox_shader->Use();

    // Set uniforms
    s_skybox_shader->SetInt("skyboxMode", 0);
    s_skybox_shader->SetVec4("backgroundColor", m_background_color);

    // Set sun parameters (even for solid color mode)
    s_skybox_shader->SetInt("sunEnabled", m_sun_enabled ? 1 : 0);
    s_skybox_shader->SetVec3("sunDirection", GetSunDirection());
    s_skybox_shader->SetVec3("sunColor", m_sun_color);
    s_skybox_shader->SetFloat("sunIntensity", m_sun_intensity);
    s_skybox_shader->SetVec3("shadowColor", m_shadow_color);

    // Set fog parameters
    SetFogUniforms();

    // Set matrices (remove translation from view matrix)
    glm::mat4 view = Renderer::GetViewMatrix();
    view = glm::mat4(glm::mat3(view)); // Remove translation
    s_skybox_shader->SetMat4("view", view);
    s_skybox_shader->SetMat4("projection", Renderer::GetProjectionMatrix());

    // Render skybox cube
    glBindVertexArray(m_skybox_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // Re-enable depth testing and writing
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void Skybox3D::RenderPanoramicTexture() {
    if (!s_skybox_shader || !m_mesh_initialized) return;

    // Disable depth testing and writing for skybox
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    s_skybox_shader->Use();

    // Set uniforms
    if (m_skybox_mode == SkyboxMode::PanoramicImage) {
        s_skybox_shader->SetInt("skyboxMode", 1);
        if (m_texture_loaded && m_texture_id != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_texture_id);
            s_skybox_shader->SetInt("panoramicTexture", 0);
        }
    } else if (m_skybox_mode == SkyboxMode::Cubemap) {
        s_skybox_shader->SetInt("skyboxMode", 2);
        if (m_texture_loaded && m_texture_id != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture_id);
            s_skybox_shader->SetInt("cubemapTexture", 0);
        }
    }

    s_skybox_shader->SetFloat("textureRotation", m_texture_rotation);
    s_skybox_shader->SetFloat("exposure", m_exposure);

    // Set sun parameters
    s_skybox_shader->SetInt("sunEnabled", m_sun_enabled ? 1 : 0);
    s_skybox_shader->SetVec3("sunDirection", GetSunDirection());
    s_skybox_shader->SetVec3("sunColor", m_sun_color);
    s_skybox_shader->SetFloat("sunIntensity", m_sun_intensity);
    s_skybox_shader->SetVec3("shadowColor", m_shadow_color);

    // Set fog parameters
    SetFogUniforms();

    // Set matrices (remove translation from view matrix)
    glm::mat4 view = Renderer::GetViewMatrix();
    view = glm::mat4(glm::mat3(view)); // Remove translation
    s_skybox_shader->SetMat4("view", view);
    s_skybox_shader->SetMat4("projection", Renderer::GetProjectionMatrix());

    // Render skybox cube
    glBindVertexArray(m_skybox_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // Re-enable depth testing and writing
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void Skybox3D::RenderProceduralSky() {
    if (!s_skybox_shader || !m_mesh_initialized) return;

    // Disable depth testing and writing for skybox
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    s_skybox_shader->Use();

    // Set uniforms
    s_skybox_shader->SetInt("skyboxMode", 3);
    s_skybox_shader->SetInt("sunEnabled", m_sun_enabled ? 1 : 0);
    s_skybox_shader->SetVec3("sunDirection", GetSunDirection());
    s_skybox_shader->SetVec3("sunColor", m_sun_color);
    s_skybox_shader->SetFloat("sunIntensity", m_sun_intensity);
    s_skybox_shader->SetVec3("shadowColor", m_shadow_color);

    // Set fog parameters
    SetFogUniforms();

    // Set matrices (remove translation from view matrix)
    glm::mat4 view = Renderer::GetViewMatrix();
    view = glm::mat4(glm::mat3(view)); // Remove translation
    s_skybox_shader->SetMat4("view", view);
    s_skybox_shader->SetMat4("projection", Renderer::GetProjectionMatrix());

    // Render skybox cube
    glBindVertexArray(m_skybox_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // Re-enable depth testing and writing
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void Skybox3D::UpdateLighting(LightingSystem* lighting_system) {
    if (!lighting_system) return;

    // Calculate ambient light contribution based on skybox mode
    glm::vec3 ambient_contribution(0.0f);

    switch (m_skybox_mode) {
        case SkyboxMode::SolidColor:
            // Use background color as ambient light with better color balance
            ambient_contribution = glm::vec3(m_background_color) * 0.25f;
            // Add slight color temperature variation
            ambient_contribution.r *= 1.05f; // Slightly warmer
            ambient_contribution.b *= 0.95f; // Slightly less blue
            break;
        case SkyboxMode::ProceduralSky:
            // Use shadow color as ambient base, influenced by sun
            ambient_contribution = m_shadow_color * 0.4f;
            if (m_sun_enabled) {
                // More sophisticated sun contribution based on sun direction
                glm::vec3 sun_dir = GetSunDirection();
                float sun_height = glm::clamp(sun_dir.y, 0.0f, 1.0f); // How high the sun is
                float sun_contribution = sun_height * m_sun_intensity * 0.15f;
                glm::vec3 sun_color_rgb = glm::vec3(m_sun_color.r, m_sun_color.g, m_sun_color.b);
                ambient_contribution += sun_color_rgb * sun_contribution;

                // Add atmospheric scattering effect
                glm::vec3 sky_tint = glm::mix(glm::vec3(0.4f, 0.6f, 1.0f), sun_color_rgb, sun_height * 0.3f);
                ambient_contribution = glm::mix(ambient_contribution, sky_tint * 0.2f, 0.3f);
            }
            break;
        case SkyboxMode::PanoramicImage:
        case SkyboxMode::Cubemap:
            // For texture-based skyboxes, sample average color if possible
            // For now, use improved neutral ambient with exposure consideration
            float exposure_factor = glm::clamp(m_exposure, 0.5f, 2.0f);
            ambient_contribution = glm::vec3(0.18f, 0.20f, 0.25f) * exposure_factor * 0.8f;

            // Add slight variation based on texture rotation for more dynamic lighting
            float rotation_factor = sin(glm::radians(m_texture_rotation) * 0.1f) * 0.05f + 1.0f;
            ambient_contribution *= rotation_factor;
            break;
    }

    // Enhanced clamping with better range
    ambient_contribution = glm::clamp(ambient_contribution, glm::vec3(0.02f), glm::vec3(0.8f));

    // Set ambient light in lighting system
    lighting_system->SetAmbientLight(ambient_contribution);

    // Set fog parameters in lighting system
    lighting_system->SetFogParameters(m_fog_enabled, m_fog_color, m_fog_density,
                                     m_fog_start, m_fog_end, m_fog_height_falloff);

    // If sun is enabled, create a virtual directional light for shadow casting
    if (m_sun_enabled && (m_skybox_mode == SkyboxMode::ProceduralSky || m_skybox_mode == SkyboxMode::SolidColor)) {
        // Create virtual sun light data for the lighting system
        CreateVirtualSunLight(lighting_system);
    }
}

void Skybox3D::CreateVirtualSunLight(LightingSystem* lighting_system) {
    if (!lighting_system || !m_sun_enabled) return;

    // Create a virtual directional light for the sun
    glm::vec3 sun_direction = GetSunDirection();

    // Calculate sun intensity based on height (simulate atmospheric absorption)
    float sun_height = glm::clamp(sun_direction.y, 0.0f, 1.0f);
    float atmospheric_factor = glm::mix(0.3f, 1.0f, sun_height); // Dimmer when low on horizon
    float effective_intensity = m_sun_intensity * atmospheric_factor;

    // Adjust sun color based on height (warmer when low, cooler when high)
    glm::vec3 effective_color = m_sun_color;
    if (sun_height < 0.3f) {
        // Add orange/red tint for low sun
        effective_color.r *= 1.2f;
        effective_color.g *= 0.9f;
        effective_color.b *= 0.7f;
    }

    // Add virtual sun light to lighting system
    lighting_system->AddVirtualDirectionalLight(
        sun_direction,
        effective_color,
        effective_intensity,
        true, // Cast shadows
        0.001f, // Shadow bias
        0.7f, // Shadow opacity
        m_shadow_color // Shadow color
    );
}

} // namespace Lupine
