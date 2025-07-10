#include "lupine/rendering/LightingSystem.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include "lupine/components/OmniLight.h"
#include "lupine/components/DirectionalLight.h"
#include "lupine/components/SpotLight.h"
#include "lupine/components/PrimitiveMesh.h"
#include "lupine/components/StaticMesh.h"
#include "lupine/components/SkinnedMesh.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/nodes/Node3D.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Lupine {

LightingSystem::LightingSystem()
    : m_ambient_light(0.1f, 0.1f, 0.1f)
    , m_initialized(false)
    , m_fog_enabled(false)
    , m_fog_color(0.7f, 0.8f, 0.9f)
    , m_fog_density(0.02f)
    , m_fog_start(10.0f)
    , m_fog_end(100.0f)
    , m_fog_height_falloff(0.1f)
    , m_shadow_map_array(0)
    , m_shadow_framebuffer(0)
    , m_shadow_depth_buffer(0)
    , m_next_shadow_map_index(0)
    , m_shadows_enabled(true) // Enable shadows by default
    , m_shadow_quality(ShadowQuality::Medium) { // Default to medium quality
}

void LightingSystem::Initialize() {
    if (m_initialized) return;

    m_light_data.reserve(MAX_LIGHTS);
    InitializeShadowMapping();
    m_initialized = true;
}

void LightingSystem::Shutdown() {
    if (!m_initialized) return;

    CleanupShadowMapping();
    m_light_data.clear();
    m_initialized = false;
}

void LightingSystem::UpdateLights(Scene* scene) {
    if (!m_initialized || !scene) return;

    m_light_data.clear();
    m_next_shadow_map_index = 0; // Reset shadow map index counter

    // Collect all nodes from the scene
    std::vector<Node*> all_nodes = scene->GetAllNodes();

    for (Node* node : all_nodes) {
        if (!node || !node->IsActive()) continue;

        // Check for OmniLight components
        if (auto* omni_light = node->GetComponent<OmniLight>()) {
            if (omni_light->IsEnabled() && m_light_data.size() < MAX_LIGHTS) {
                glm::vec3 world_pos = omni_light->GetWorldPosition();
                m_light_data.push_back(ConvertOmniLight(omni_light, world_pos));
            }
        }

        // Check for DirectionalLight components
        if (auto* dir_light = node->GetComponent<DirectionalLight>()) {
            if (dir_light->IsEnabled() && m_light_data.size() < MAX_LIGHTS) {
                glm::vec3 world_pos = dir_light->GetWorldPosition();
                m_light_data.push_back(ConvertDirectionalLight(dir_light, world_pos));
            }
        }

        // Check for SpotLight components
        if (auto* spot_light = node->GetComponent<SpotLight>()) {
            if (spot_light->IsEnabled() && m_light_data.size() < MAX_LIGHTS) {
                glm::vec3 world_pos = spot_light->GetWorldPosition();
                m_light_data.push_back(ConvertSpotLight(spot_light, world_pos));
            }
        }
    }

    // Sort lights by type for better shader performance (directional first, then point, then spot)
    std::sort(m_light_data.begin(), m_light_data.end(), [](const LightData& a, const LightData& b) {
        return a.type < b.type;
    });

    int shadow_casting_count = 0;
    for (const auto& light : m_light_data) {
        if (light.casts_shadows) shadow_casting_count++;
    }

}

void LightingSystem::BindLightingUniforms(unsigned int shader_program) {
    if (!m_initialized) return;

    glUseProgram(shader_program);

    // Bind ambient light
    int ambient_loc = glGetUniformLocation(shader_program, "uAmbientLight");
    if (ambient_loc != -1) {
        glUniform3fv(ambient_loc, 1, glm::value_ptr(m_ambient_light));
    }

    // Bind light count
    int light_count_loc = glGetUniformLocation(shader_program, "uLightCount");
    if (light_count_loc != -1) {
        glUniform1i(light_count_loc, GetLightCount());
    }

    // Bind camera position for specular lighting
    int camera_pos_loc = glGetUniformLocation(shader_program, "uCameraPos");
    if (camera_pos_loc != -1) {
        glm::vec3 camera_pos = GetCameraPosition();
        glUniform3fv(camera_pos_loc, 1, glm::value_ptr(camera_pos));
    }

    // Bind fog parameters
    int fog_enabled_loc = glGetUniformLocation(shader_program, "uFogEnabled");
    if (fog_enabled_loc != -1) {
        glUniform1i(fog_enabled_loc, m_fog_enabled ? 1 : 0);
    }

    int fog_color_loc = glGetUniformLocation(shader_program, "uFogColor");
    if (fog_color_loc != -1) {
        glUniform3fv(fog_color_loc, 1, glm::value_ptr(m_fog_color));
    }

    int fog_density_loc = glGetUniformLocation(shader_program, "uFogDensity");
    if (fog_density_loc != -1) {
        glUniform1f(fog_density_loc, m_fog_density);
    }

    int fog_start_loc = glGetUniformLocation(shader_program, "uFogStart");
    if (fog_start_loc != -1) {
        glUniform1f(fog_start_loc, m_fog_start);
    }

    int fog_end_loc = glGetUniformLocation(shader_program, "uFogEnd");
    if (fog_end_loc != -1) {
        glUniform1f(fog_end_loc, m_fog_end);
    }

    int fog_height_falloff_loc = glGetUniformLocation(shader_program, "uFogHeightFalloff");
    if (fog_height_falloff_loc != -1) {
        glUniform1f(fog_height_falloff_loc, m_fog_height_falloff);
    }

    // Bind light data array
    for (int i = 0; i < GetLightCount() && i < MAX_LIGHTS; ++i) {
        const LightData& light = m_light_data[i];
        std::string base = "uLights[" + std::to_string(i) + "]";

        // Bind light properties
        int pos_loc = glGetUniformLocation(shader_program, (base + ".position").c_str());
        if (pos_loc != -1) glUniform3fv(pos_loc, 1, glm::value_ptr(light.position));

        int dir_loc = glGetUniformLocation(shader_program, (base + ".direction").c_str());
        if (dir_loc != -1) glUniform3fv(dir_loc, 1, glm::value_ptr(light.direction));

        int color_loc = glGetUniformLocation(shader_program, (base + ".color").c_str());
        if (color_loc != -1) {
            glUniform3fv(color_loc, 1, glm::value_ptr(light.color));
        }

        int intensity_loc = glGetUniformLocation(shader_program, (base + ".intensity").c_str());
        if (intensity_loc != -1) glUniform1f(intensity_loc, light.intensity);

        int range_loc = glGetUniformLocation(shader_program, (base + ".range").c_str());
        if (range_loc != -1) glUniform1f(range_loc, light.range);

        int type_loc = glGetUniformLocation(shader_program, (base + ".type").c_str());
        if (type_loc != -1) glUniform1i(type_loc, light.type);

        int att_const_loc = glGetUniformLocation(shader_program, (base + ".attenuation_constant").c_str());
        if (att_const_loc != -1) glUniform1f(att_const_loc, light.attenuation_constant);

        int att_linear_loc = glGetUniformLocation(shader_program, (base + ".attenuation_linear").c_str());
        if (att_linear_loc != -1) glUniform1f(att_linear_loc, light.attenuation_linear);

        int att_quad_loc = glGetUniformLocation(shader_program, (base + ".attenuation_quadratic").c_str());
        if (att_quad_loc != -1) glUniform1f(att_quad_loc, light.attenuation_quadratic);

        int inner_cone_loc = glGetUniformLocation(shader_program, (base + ".inner_cone_angle").c_str());
        if (inner_cone_loc != -1) glUniform1f(inner_cone_loc, light.inner_cone_angle);

        int outer_cone_loc = glGetUniformLocation(shader_program, (base + ".outer_cone_angle").c_str());
        if (outer_cone_loc != -1) glUniform1f(outer_cone_loc, light.outer_cone_angle);

        // Shadow properties
        int casts_shadows_loc = glGetUniformLocation(shader_program, (base + ".casts_shadows").c_str());
        if (casts_shadows_loc != -1) glUniform1i(casts_shadows_loc, light.casts_shadows);

        int shadow_map_index_loc = glGetUniformLocation(shader_program, (base + ".shadow_map_index").c_str());
        if (shadow_map_index_loc != -1) glUniform1i(shadow_map_index_loc, light.shadow_map_index);

        int shadow_bias_loc = glGetUniformLocation(shader_program, (base + ".shadow_bias").c_str());
        if (shadow_bias_loc != -1) glUniform1f(shadow_bias_loc, light.shadow_bias);

        int shadow_opacity_loc = glGetUniformLocation(shader_program, (base + ".shadow_opacity").c_str());
        if (shadow_opacity_loc != -1) glUniform1f(shadow_opacity_loc, light.shadow_opacity);

        int shadow_color_loc = glGetUniformLocation(shader_program, (base + ".shadow_color").c_str());
        if (shadow_color_loc != -1) glUniform3fv(shadow_color_loc, 1, glm::value_ptr(light.shadow_color));
    }

    // Bind shadow maps
    BindShadowMaps(shader_program);

    // Bind light space matrices
    for (int i = 0; i < GetLightCount() && i < MAX_LIGHTS; ++i) {
        if (i < static_cast<int>(m_light_space_matrices.size())) {
            std::string matrix_name = "uLightSpaceMatrices[" + std::to_string(i) + "]";
            int matrix_loc = glGetUniformLocation(shader_program, matrix_name.c_str());
            if (matrix_loc != -1) {
                glUniformMatrix4fv(matrix_loc, 1, GL_FALSE, glm::value_ptr(m_light_space_matrices[i]));
            }
        }
    }
}

glm::vec3 LightingSystem::GetCameraPosition() const {
    // Extract camera position from the inverse view matrix
    glm::mat4 view_matrix = Renderer::GetViewMatrix();
    glm::mat4 inverse_view = glm::inverse(view_matrix);
    return glm::vec3(inverse_view[3]);
}

void LightingSystem::AddVirtualDirectionalLight(const glm::vec3& direction, const glm::vec3& color,
                                               float intensity, bool cast_shadows,
                                               float shadow_bias, float shadow_opacity,
                                               const glm::vec3& shadow_color) {
    // Check if we have room for another light
    if (m_light_data.size() >= MAX_LIGHTS) {
        return; // No room for more lights
    }

    // Create virtual directional light data
    LightData virtual_light = {};
    virtual_light.position = glm::vec3(0.0f); // Not used for directional lights
    virtual_light.direction = glm::normalize(direction);
    virtual_light.color = color;
    virtual_light.intensity = intensity;
    virtual_light.range = 0.0f; // Not used for directional lights
    virtual_light.type = static_cast<int>(LightType::Directional);

    // Attenuation not used for directional lights
    virtual_light.attenuation_constant = 1.0f;
    virtual_light.attenuation_linear = 0.0f;
    virtual_light.attenuation_quadratic = 0.0f;

    // Cone angles not used for directional lights
    virtual_light.inner_cone_angle = 0.0f;
    virtual_light.outer_cone_angle = 0.0f;

    // Shadow properties
    virtual_light.casts_shadows = cast_shadows ? 1 : 0;
    virtual_light.shadow_map_index = cast_shadows ? m_next_shadow_map_index++ : -1;
    virtual_light.shadow_bias = shadow_bias;
    virtual_light.shadow_opacity = shadow_opacity;
    virtual_light.shadow_color = shadow_color;

    // Clamp shadow map index to available slots
    if (virtual_light.shadow_map_index >= MAX_SHADOW_MAPS) {
        virtual_light.casts_shadows = 0;
        virtual_light.shadow_map_index = -1;
        m_next_shadow_map_index--; // Revert increment
    }

    // Add to light data
    m_light_data.push_back(virtual_light);
}

void LightingSystem::SetFogParameters(bool enabled, const glm::vec3& color, float density,
                                     float start, float end, float height_falloff) {
    m_fog_enabled = enabled;
    m_fog_color = color;
    m_fog_density = std::max(0.0f, density);
    m_fog_start = std::max(0.0f, start);
    m_fog_end = std::max(start + 0.1f, end);
    m_fog_height_falloff = std::max(0.0f, height_falloff);
}



LightData LightingSystem::ConvertOmniLight(OmniLight* omni_light, const glm::vec3& world_position) {
    LightData data = {};

    data.position = world_position;
    data.direction = glm::vec3(0.0f); // Not used for point lights
    data.color = omni_light->GetColor();
    data.intensity = omni_light->GetIntensity();
    data.range = omni_light->GetRange();
    data.type = static_cast<int>(LightType::Point);
    
    data.attenuation_constant = omni_light->GetAttenuationConstant();
    data.attenuation_linear = omni_light->GetAttenuationLinear();
    data.attenuation_quadratic = omni_light->GetAttenuationQuadratic();
    
    data.inner_cone_angle = 0.0f; // Not used for point lights
    data.outer_cone_angle = 0.0f; // Not used for point lights

    // Shadow mapping
    data.casts_shadows = (omni_light->GetShadowMode() == OmniLight::ShadowMode::Enabled) ? 1 : 0;
    data.shadow_map_index = data.casts_shadows ? m_next_shadow_map_index++ : -1;
    data.shadow_bias = omni_light->GetShadowBias();
    data.shadow_opacity = omni_light->GetShadowOpacity();
    data.shadow_color = omni_light->GetShadowColor();

    // Clamp shadow map index to available slots
    if (data.shadow_map_index >= MAX_SHADOW_MAPS) {
        data.casts_shadows = 0;
        data.shadow_map_index = -1;
        m_next_shadow_map_index--; // Revert increment
    }

    return data;
}

LightData LightingSystem::ConvertDirectionalLight(DirectionalLight* dir_light, const glm::vec3& world_position) {
    LightData data = {};
    
    data.position = world_position; // Position not really used for directional lights
    data.direction = dir_light->GetDirection();
    data.color = dir_light->GetColor();
    data.intensity = dir_light->GetIntensity();
    data.range = 0.0f; // Infinite range for directional lights
    data.type = static_cast<int>(LightType::Directional);
    
    data.attenuation_constant = 1.0f; // No attenuation for directional lights
    data.attenuation_linear = 0.0f;
    data.attenuation_quadratic = 0.0f;
    
    data.inner_cone_angle = 0.0f; // Not used for directional lights
    data.outer_cone_angle = 0.0f; // Not used for directional lights

    // Shadow mapping
    data.casts_shadows = (dir_light->GetShadowMode() == DirectionalLight::ShadowMode::Enabled) ? 1 : 0;
    data.shadow_map_index = data.casts_shadows ? m_next_shadow_map_index++ : -1;
    data.shadow_bias = dir_light->GetShadowBias();
    data.shadow_opacity = dir_light->GetShadowOpacity();
    data.shadow_color = dir_light->GetShadowColor();

    // Clamp shadow map index to available slots
    if (data.shadow_map_index >= MAX_SHADOW_MAPS) {
        data.casts_shadows = 0;
        data.shadow_map_index = -1;
        m_next_shadow_map_index--; // Revert increment
    }

    return data;
}

LightData LightingSystem::ConvertSpotLight(SpotLight* spot_light, const glm::vec3& world_position) {
    LightData data = {};
    
    data.position = world_position;
    data.direction = spot_light->GetDirection();
    data.color = spot_light->GetColor();
    data.intensity = spot_light->GetIntensity();
    data.range = spot_light->GetRange();
    data.type = static_cast<int>(LightType::Spot);
    
    data.attenuation_constant = spot_light->GetAttenuationConstant();
    data.attenuation_linear = spot_light->GetAttenuationLinear();
    data.attenuation_quadratic = spot_light->GetAttenuationQuadratic();
    
    // Convert angles to cosines for shader efficiency
    data.inner_cone_angle = cos(glm::radians(spot_light->GetInnerConeAngle() * 0.5f));
    data.outer_cone_angle = cos(glm::radians(spot_light->GetOuterConeAngle() * 0.5f));

    // Shadow mapping
    data.casts_shadows = (spot_light->GetShadowMode() == SpotLight::ShadowMode::Enabled) ? 1 : 0;
    data.shadow_map_index = data.casts_shadows ? m_next_shadow_map_index++ : -1;
    data.shadow_bias = spot_light->GetShadowBias();
    data.shadow_opacity = spot_light->GetShadowOpacity();
    data.shadow_color = spot_light->GetShadowColor();

    // Clamp shadow map index to available slots
    if (data.shadow_map_index >= MAX_SHADOW_MAPS) {
        data.casts_shadows = 0;
        data.shadow_map_index = -1;
        m_next_shadow_map_index--; // Revert increment
    }

    return data;
}

void LightingSystem::InitializeShadowMapping() {
    // Create shadow map texture array
    glGenTextures(1, &m_shadow_map_array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadow_map_array);

    // Allocate storage for shadow map array with higher precision
    int shadow_size = GetShadowMapSize();
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
                 shadow_size, shadow_size, MAX_SHADOW_MAPS,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Enhanced texture parameters for better shadow quality
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Keep manual comparison for now (can be upgraded to hardware comparison later)
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    // Set border color to white (maximum depth = no shadow)
    float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border_color);

    // Create framebuffer for shadow rendering
    glGenFramebuffers(1, &m_shadow_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_framebuffer);

    // Attach shadow map array as depth attachment
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_shadow_map_array, 0);

    // No color buffer needed for shadow mapping
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "LightingSystem: Shadow framebuffer not complete!" << std::endl;
    }

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create shadow shader for depth rendering
    CreateShadowShader();
}

void LightingSystem::CleanupShadowMapping() {
    if (m_shadow_map_array != 0) {
        glDeleteTextures(1, &m_shadow_map_array);
        m_shadow_map_array = 0;
    }

    if (m_shadow_framebuffer != 0) {
        glDeleteFramebuffers(1, &m_shadow_framebuffer);
        m_shadow_framebuffer = 0;
    }
}

void LightingSystem::RenderShadowMaps(Scene* scene) {
    if (!scene || m_shadow_map_array == 0 || !m_shadows_enabled) {
        if (!m_shadows_enabled) {

        }
        if (m_shadow_map_array == 0) {

        }
        return;
    }


    // Store current OpenGL state to restore later
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    GLint current_framebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current_framebuffer);

    GLint current_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);

    GLint current_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

    GLint current_active_texture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &current_active_texture);

    // Store current state
    GLboolean colorMask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
    GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean depthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    GLboolean blend = glIsEnabled(GL_BLEND);
    GLboolean cullFace = glIsEnabled(GL_CULL_FACE);

    GLint depthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);

    // Reset shadow map index counter for this frame
    m_next_shadow_map_index = 0;

    // Clear light space matrices
    m_light_space_matrices.clear();
    m_light_space_matrices.resize(MAX_LIGHTS, glm::mat4(1.0f));

    // Bind shadow framebuffer and set viewport
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_framebuffer);
    int shadow_size = GetShadowMapSize();
    glViewport(0, 0, shadow_size, shadow_size);

    // Configure OpenGL state for shadow rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT); // Cull front faces to reduce peter panning

    // Render shadow maps for each shadow-casting light
    int shadow_casting_lights = 0;
    for (size_t i = 0; i < m_light_data.size(); ++i) {
        const auto& light_data = m_light_data[i];
        if (light_data.casts_shadows && light_data.shadow_map_index >= 0) {
            shadow_casting_lights++;

            // Calculate and store light space matrix
            glm::mat4 light_space_matrix;
            if (light_data.type == static_cast<int>(LightType::Directional)) {
                light_space_matrix = CalculateDirectionalLightMatrix(light_data);
            } else if (light_data.type == static_cast<int>(LightType::Point)) {
                // For point lights, use a simplified directional approach for now
                // TODO: Implement proper cube mapping for point lights
                light_space_matrix = CalculatePointLightAsDirectional(light_data);
            } else if (light_data.type == static_cast<int>(LightType::Spot)) {
                light_space_matrix = CalculateSpotLightMatrix(light_data);
            }

            if (i < m_light_space_matrices.size()) {
                m_light_space_matrices[i] = light_space_matrix;
            }

            RenderShadowMapForLight(light_data, scene);
        }
    }


    // Restore all OpenGL state
    glBindFramebuffer(GL_FRAMEBUFFER, current_framebuffer);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glUseProgram(current_program);

    // Restore texture state
    glActiveTexture(current_active_texture);
    glBindTexture(GL_TEXTURE_2D, current_texture);

    // Restore render state
    glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    glDepthMask(depthMask);
    glDepthFunc(depthFunc);

    if (!depthTest) {
        glDisable(GL_DEPTH_TEST);
    }
    if (blend) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    if (cullFace) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK); // Restore default back face culling
    } else {
        glDisable(GL_CULL_FACE);
    }
}

void LightingSystem::RenderShadowMapForLight(const LightData& light_data, Scene* scene) {
    if (!scene || !m_shadow_shader) {
        return;
    }

    // Attach the specific shadow map layer
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              m_shadow_map_array, 0, light_data.shadow_map_index);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Use shadow shader
    m_shadow_shader->Use();

    // Calculate light space matrix based on light type
    glm::mat4 light_space_matrix(1.0f); // Initialize to identity matrix
    if (light_data.type == static_cast<int>(LightType::Directional)) {
        light_space_matrix = CalculateDirectionalLightMatrix(light_data);
    } else if (light_data.type == static_cast<int>(LightType::Point)) {
        // Use simplified directional approach for point lights
        light_space_matrix = CalculatePointLightAsDirectional(light_data);
    } else if (light_data.type == static_cast<int>(LightType::Spot)) {
        light_space_matrix = CalculateSpotLightMatrix(light_data);
    } else {
        return; // Unknown light type
    }

    // Set light space matrix uniform
    m_shadow_shader->SetMatrix4("lightSpaceMatrix", light_space_matrix);

    // Render scene geometry from light's perspective
    RenderSceneForShadows(scene, light_space_matrix);

    // Unbind shadow shader
    glUseProgram(0);
}

void LightingSystem::BindShadowMaps(unsigned int shader_program) {
    if (m_shadow_map_array != 0) {
        glActiveTexture(GL_TEXTURE10); // Use texture unit 10 for shadow maps
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadow_map_array);
        glUniform1i(glGetUniformLocation(shader_program, "u_shadow_maps"), 10);

    } else {

    }
}

void LightingSystem::CreateShadowShader() {
    std::string shadow_vertex = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        uniform mat4 lightSpaceMatrix;
        uniform mat4 model;

        void main() {
            gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
        }
    )";

    std::string shadow_fragment = R"(
        #version 330 core

        void main() {
            // OpenGL automatically writes depth to gl_FragDepth
            // No need to explicitly write anything
        }
    )";

    m_shadow_shader = std::make_unique<Shader>(shadow_vertex, shadow_fragment);
}

glm::mat4 LightingSystem::CalculateDirectionalLightMatrix(const LightData& light_data) {
    // For directional lights, create an orthographic projection
    // that covers the scene from the light's perspective

    // Light direction (normalized)
    glm::vec3 light_dir = glm::normalize(light_data.direction);

    // Enhanced scene bounds calculation for better shadow coverage
    glm::vec3 scene_center = glm::vec3(0.0f);
    float scene_radius = 50.0f; // Increased for better coverage

    // Create a stable view matrix looking down the light direction
    glm::vec3 light_pos = scene_center - light_dir * scene_radius * 3.0f;
    glm::vec3 target = scene_center;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    // If light direction is too close to up vector, use different up
    if (glm::abs(glm::dot(light_dir, up)) > 0.95f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    // Ensure up vector is perpendicular to light direction for stability
    up = glm::normalize(glm::cross(glm::cross(light_dir, up), light_dir));

    glm::mat4 light_view = glm::lookAt(light_pos, target, up);

    // Enhanced orthographic projection with better bounds
    float ortho_size = scene_radius * 1.5f; // Increased padding for better coverage
    float near_plane = 0.1f;
    float far_plane = scene_radius * 5.0f; // Extended far plane for better depth coverage

    glm::mat4 light_projection = glm::ortho(-ortho_size, ortho_size,
                                           -ortho_size, ortho_size,
                                           near_plane, far_plane);

    return light_projection * light_view;
}

glm::mat4 LightingSystem::CalculatePointLightMatrix(const LightData& light_data, int face_index) {
    // For point lights, we need 6 matrices for cube mapping
    // This is a simplified version using just one face

    glm::vec3 light_pos = light_data.position;

    // Define the 6 directions for cube faces
    glm::vec3 directions[6] = {
        glm::vec3(1.0f, 0.0f, 0.0f),   // +X
        glm::vec3(-1.0f, 0.0f, 0.0f),  // -X
        glm::vec3(0.0f, 1.0f, 0.0f),   // +Y
        glm::vec3(0.0f, -1.0f, 0.0f),  // -Y
        glm::vec3(0.0f, 0.0f, 1.0f),   // +Z
        glm::vec3(0.0f, 0.0f, -1.0f)   // -Z
    };

    glm::vec3 ups[6] = {
        glm::vec3(0.0f, -1.0f, 0.0f),  // +X
        glm::vec3(0.0f, -1.0f, 0.0f),  // -X
        glm::vec3(0.0f, 0.0f, 1.0f),   // +Y
        glm::vec3(0.0f, 0.0f, -1.0f),  // -Y
        glm::vec3(0.0f, -1.0f, 0.0f),  // +Z
        glm::vec3(0.0f, -1.0f, 0.0f)   // -Z
    };

    glm::vec3 target = light_pos + directions[face_index];
    glm::mat4 light_view = glm::lookAt(light_pos, target, ups[face_index]);

    // Perspective projection for point light
    glm::mat4 light_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, light_data.range);

    return light_projection * light_view;
}

glm::mat4 LightingSystem::CalculatePointLightAsDirectional(const LightData& light_data) {
    // Enhanced point light shadow using directional approach
    // Cast shadows in the most effective direction based on scene layout
    glm::vec3 light_pos = light_data.position;

    // Choose shadow direction based on light height
    glm::vec3 light_dir;
    if (light_pos.y > 5.0f) {
        // High lights cast shadows downward
        light_dir = glm::vec3(0.0f, -1.0f, 0.0f);
    } else {
        // Low lights cast shadows forward/downward
        light_dir = glm::normalize(glm::vec3(0.0f, -0.7f, -0.7f));
    }

    // Create a view matrix looking from the light
    glm::vec3 target = light_pos + light_dir;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    // Ensure up vector is not parallel to light direction
    if (abs(glm::dot(light_dir, up)) > 0.95f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::mat4 light_view = glm::lookAt(light_pos, target, up);

    // Enhanced projection for better shadow coverage
    float ortho_size = glm::max(light_data.range * 1.2f, 15.0f); // Increased coverage
    glm::mat4 light_projection = glm::ortho(-ortho_size, ortho_size,
                                           -ortho_size, ortho_size,
                                           0.1f, light_data.range * 2.5f); // Extended range

    return light_projection * light_view;
}

glm::mat4 LightingSystem::CalculateSpotLightMatrix(const LightData& light_data) {
    glm::vec3 light_pos = light_data.position;
    glm::vec3 light_dir = glm::normalize(light_data.direction);
    glm::vec3 target = light_pos + light_dir;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    // If light direction is too close to up vector, use different up
    if (glm::abs(glm::dot(light_dir, up)) > 0.95f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::mat4 light_view = glm::lookAt(light_pos, target, up);

    // Calculate FOV from outer cone angle (stored as cosine)
    // Convert from cosine back to angle and double it for full cone
    float half_angle = glm::acos(glm::clamp(light_data.outer_cone_angle, 0.0f, 1.0f));
    float fov = half_angle * 2.0f;

    // Clamp FOV to reasonable values
    fov = glm::clamp(fov, glm::radians(1.0f), glm::radians(179.0f));

    float near_plane = 0.1f;
    float far_plane = glm::max(light_data.range, 1.0f);

    glm::mat4 light_projection = glm::perspective(fov, 1.0f, near_plane, far_plane);

    return light_projection * light_view;
}

void LightingSystem::RenderSceneForShadows(Scene* scene, const glm::mat4& light_space_matrix) {
    if (!scene || !m_shadow_shader) return;

    // Recursively render all nodes that can cast shadows
    RenderNodeForShadows(scene->GetRootNode(), light_space_matrix);
}

void LightingSystem::RenderNodeForShadows(Node* node, const glm::mat4& light_space_matrix) {
    if (!node || !node->IsActive()) return;

    // Check if this is a 3D node with renderable components
    if (Node3D* node3d = dynamic_cast<Node3D*>(node)) {
        bool rendered_shadow = false;

        // Check for PrimitiveMesh component that can cast shadows
        if (auto* primitive_mesh = node->GetComponent<PrimitiveMesh>()) {
            if (primitive_mesh->GetCastsShadows()) {
                // Get the node's world transform
                glm::mat4 model_matrix = node3d->GetGlobalTransform();

                // Set model matrix uniform
                m_shadow_shader->SetMatrix4("model", model_matrix);

                // Render the primitive mesh for shadow mapping
                primitive_mesh->RenderForShadows();
                rendered_shadow = true;
            }
        }

        // Check for StaticMesh component that can cast shadows
        if (!rendered_shadow) {
            if (auto* static_mesh = node->GetComponent<StaticMesh>()) {
                if (static_mesh->GetCastsShadows()) {
                    // Get the node's world transform
                    glm::mat4 model_matrix = node3d->GetGlobalTransform();

                    // Set model matrix uniform
                    m_shadow_shader->SetMatrix4("model", model_matrix);

                    // Render the static mesh for shadow mapping
                    static_mesh->RenderForShadows();
                    rendered_shadow = true;
                }
            }
        }

        // Check for SkinnedMesh component that can cast shadows
        if (!rendered_shadow) {
            if (auto* skinned_mesh = node->GetComponent<SkinnedMesh>()) {
                if (skinned_mesh->GetCastsShadows()) {
                    // Get the node's world transform
                    glm::mat4 model_matrix = node3d->GetGlobalTransform();

                    // Set model matrix uniform
                    m_shadow_shader->SetMatrix4("model", model_matrix);

                    // Render the skinned mesh for shadow mapping
                    skinned_mesh->RenderForShadows();
                    rendered_shadow = true;
                }
            }
        }
    }

    // Recursively render children
    const auto& children = node->GetChildren();
    for (const auto& child : children) {
        RenderNodeForShadows(child.get(), light_space_matrix);
    }
}

void LightingSystem::SetShadowQuality(ShadowQuality quality) {
    if (m_shadow_quality != quality) {
        m_shadow_quality = quality;

        // Reinitialize shadow mapping with new quality settings
        if (m_initialized) {
            CleanupShadowMapping();
            InitializeShadowMapping();
        }
    }
}

int LightingSystem::GetShadowMapSize() const {
    switch (m_shadow_quality) {
        case ShadowQuality::Low:
            return SHADOW_MAP_SIZE_LOW;
        case ShadowQuality::Medium:
            return SHADOW_MAP_SIZE_MEDIUM;
        case ShadowQuality::High:
            return SHADOW_MAP_SIZE_HIGH;
        case ShadowQuality::Ultra:
            return SHADOW_MAP_SIZE_ULTRA;
        default:
            return SHADOW_MAP_SIZE_MEDIUM;
    }
}

} // namespace Lupine
