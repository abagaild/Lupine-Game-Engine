#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace Lupine {

class GraphicsShader;

/**
 * @brief Skybox3D component for 3D background rendering
 *
 * Skybox3D component provides background rendering for 3D scenes with support for:
 * - Solid color backgrounds
 * - Panoramic image textures (equirectangular, cubemap)
 * - Sun lighting with configurable color, intensity, and shadow color
 * 
 * The skybox is rendered as a background layer before other 3D content,
 * ensuring it appears infinitely far away.
 */
class Skybox3D : public Component {
public:
    /**
     * @brief Skybox rendering modes
     */
    enum class SkyboxMode {
        SolidColor,     // Solid color background
        PanoramicImage, // Panoramic texture (equirectangular)
        Cubemap,        // Cubemap texture (6 faces)
        ProceduralSky   // Procedural sky with sun
    };

    /**
     * @brief Constructor
     */
    Skybox3D();

    /**
     * @brief Virtual destructor
     */
    virtual ~Skybox3D();

    /**
     * @brief Get component type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Skybox3D"; }

    /**
     * @brief Get skybox mode
     * @return Current skybox mode
     */
    SkyboxMode GetSkyboxMode() const { return m_skybox_mode; }

    /**
     * @brief Set skybox mode
     * @param mode New skybox mode
     */
    void SetSkyboxMode(SkyboxMode mode);

    /**
     * @brief Get background color (for solid color mode)
     * @return Background color (RGBA)
     */
    const glm::vec4& GetBackgroundColor() const { return m_background_color; }

    /**
     * @brief Set background color (for solid color mode)
     * @param color Background color (RGBA)
     */
    void SetBackgroundColor(const glm::vec4& color);

    /**
     * @brief Get panoramic texture path
     * @return Texture file path
     */
    const std::string& GetTexturePath() const { return m_texture_path; }

    /**
     * @brief Set panoramic texture path
     * @param path Texture file path
     */
    void SetTexturePath(const std::string& path);

    /**
     * @brief Get sun enabled state
     * @return True if sun is enabled
     */
    bool IsSunEnabled() const { return m_sun_enabled; }

    /**
     * @brief Set sun enabled state
     * @param enabled True to enable sun
     */
    void SetSunEnabled(bool enabled);

    /**
     * @brief Get sun color
     * @return Sun color (RGB)
     */
    glm::vec3 GetSunColor() const { return glm::vec3(m_sun_color.r, m_sun_color.g, m_sun_color.b); }

    /**
     * @brief Set sun color
     * @param color Sun color (RGB)
     */
    void SetSunColor(const glm::vec3& color);

    /**
     * @brief Get sun intensity
     * @return Sun intensity multiplier
     */
    float GetSunIntensity() const { return m_sun_intensity; }

    /**
     * @brief Set sun intensity
     * @param intensity Sun intensity multiplier
     */
    void SetSunIntensity(float intensity);

    /**
     * @brief Get shadow color
     * @return Shadow color (RGB)
     */
    glm::vec3 GetShadowColor() const { return glm::vec3(m_shadow_color.r, m_shadow_color.g, m_shadow_color.b); }

    /**
     * @brief Set shadow color
     * @param color Shadow color (RGB)
     */
    void SetShadowColor(const glm::vec3& color);

    /**
     * @brief Get sun rotation (pitch and yaw in degrees)
     * @return Sun rotation vector (x=pitch, y=yaw)
     */
    const glm::vec2& GetSunRotation() const { return m_sun_rotation; }

    /**
     * @brief Set sun rotation (pitch and yaw in degrees)
     * @param rotation Sun rotation vector (x=pitch, y=yaw)
     */
    void SetSunRotation(const glm::vec2& rotation);

    /**
     * @brief Get sun direction (normalized)
     * @return Sun direction vector calculated from sun rotation
     */
    glm::vec3 GetSunDirection() const;

    /**
     * @brief Get texture rotation
     * @return Texture rotation in degrees
     */
    float GetTextureRotation() const { return m_texture_rotation; }

    /**
     * @brief Set texture rotation
     * @param rotation Texture rotation in degrees
     */
    void SetTextureRotation(float rotation);

    /**
     * @brief Get exposure value for HDR textures
     * @return Exposure multiplier
     */
    float GetExposure() const { return m_exposure; }

    /**
     * @brief Set exposure value for HDR textures
     * @param exposure Exposure multiplier
     */
    void SetExposure(float exposure);

    /**
     * @brief Get fog enabled state
     * @return True if fog is enabled
     */
    bool GetFogEnabled() const { return m_fog_enabled; }

    /**
     * @brief Set fog enabled state
     * @param enabled True to enable fog
     */
    void SetFogEnabled(bool enabled);

    /**
     * @brief Get fog color
     * @return Fog color (RGB)
     */
    glm::vec3 GetFogColor() const { return glm::vec3(m_fog_color.r, m_fog_color.g, m_fog_color.b); }

    /**
     * @brief Set fog color
     * @param color Fog color (RGB)
     */
    void SetFogColor(const glm::vec3& color);

    /**
     * @brief Get fog density
     * @return Fog density value
     */
    float GetFogDensity() const { return m_fog_density; }

    /**
     * @brief Set fog density
     * @param density Fog density value
     */
    void SetFogDensity(float density);

    /**
     * @brief Get fog start distance
     * @return Distance where fog starts
     */
    float GetFogStart() const { return m_fog_start; }

    /**
     * @brief Set fog start distance
     * @param start Distance where fog starts
     */
    void SetFogStart(float start);

    /**
     * @brief Get fog end distance
     * @return Distance where fog reaches maximum density
     */
    float GetFogEnd() const { return m_fog_end; }

    /**
     * @brief Set fog end distance
     * @param end Distance where fog reaches maximum density
     */
    void SetFogEnd(float end);

    /**
     * @brief Get fog height falloff
     * @return Height falloff factor for fog
     */
    float GetFogHeightFalloff() const { return m_fog_height_falloff; }

    /**
     * @brief Set fog height falloff
     * @param falloff Height falloff factor for fog
     */
    void SetFogHeightFalloff(float falloff);

    // Component lifecycle methods
    void OnReady() override;
    void OnUpdate(float delta_time) override;

    /**
     * @brief Render the skybox (called by renderer)
     */
    void Render();

    /**
     * @brief Update lighting system with skybox ambient contribution
     * @param lighting_system Lighting system to update
     */
    void UpdateLighting(class LightingSystem* lighting_system);

    /**
     * @brief Create virtual sun light for shadow casting
     * @param lighting_system Lighting system to add sun light to
     */
    void CreateVirtualSunLight(LightingSystem* lighting_system);

    // Serialization support
    void UpdateExportVariables();
    void UpdateFromExportVariables();

protected:
    /**
     * @brief Initialize export variables
     */
    void InitializeExportVariables() override;

private:
    // Skybox properties
    SkyboxMode m_skybox_mode;
    glm::vec4 m_background_color;
    std::string m_texture_path;
    float m_texture_rotation;
    float m_exposure;

    // Sun properties
    bool m_sun_enabled;
    glm::vec4 m_sun_color;
    float m_sun_intensity;
    glm::vec4 m_shadow_color;
    glm::vec2 m_sun_rotation; // x=pitch, y=yaw in degrees

    // Fog properties
    bool m_fog_enabled;
    glm::vec4 m_fog_color;
    float m_fog_density;
    float m_fog_start;
    float m_fog_end;
    float m_fog_height_falloff;

    // Rendering resources
    unsigned int m_skybox_VAO;
    unsigned int m_skybox_VBO;
    unsigned int m_texture_id;
    bool m_texture_loaded;
    bool m_mesh_initialized;

    // Shaders
    static std::shared_ptr<GraphicsShader> s_skybox_shader;
    static std::shared_ptr<GraphicsShader> s_procedural_sky_shader;
    static bool s_shaders_initialized;

    // Internal methods
    void InitializeMesh();
    void LoadTexture();
    void InitializeShaders();
    void CleanupResources();
    
    // Utility methods
    int SkyboxModeToInt(SkyboxMode mode) const;
    SkyboxMode IntToSkyboxMode(int value) const;
    
    // Rendering methods
    void RenderSolidColor();
    void RenderPanoramicTexture();
    void RenderProceduralSky();

    // Helper methods
    void SetFogUniforms();
};

} // namespace Lupine
