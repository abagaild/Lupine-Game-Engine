#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace Lupine {

// Forward declarations
class Scene;
class Node;
class OmniLight;
class DirectionalLight;
class SpotLight;
class Shader;



/**
 * @brief Light data structure for shader uniforms (std140 layout compatible)
 */
struct alignas(16) LightData {
    // Common properties (vec3 + float = 16 bytes each)
    glm::vec3 position;      // World position (for point/spot lights)
    float intensity;         // 16 bytes total
    glm::vec3 direction;     // Direction (for directional/spot lights)
    float range;             // 16 bytes total
    glm::vec3 color;         // Light color
    int type;                // 0=Directional, 1=Point/Omni, 2=Spot (16 bytes total)

    // Attenuation (for point/spot lights) - 16 bytes
    float attenuation_constant;
    float attenuation_linear;
    float attenuation_quadratic;
    int casts_shadows;       // 1 if this light casts shadows, 0 otherwise

    // Spot light and shadow properties - 16 bytes
    float inner_cone_angle;  // Cosine of inner cone angle
    float outer_cone_angle;  // Cosine of outer cone angle
    int shadow_map_index;    // Index into shadow map array (-1 if no shadows)
    float shadow_bias;       // Shadow bias to prevent acne

    // Shadow properties - 16 bytes
    float shadow_opacity;    // Shadow opacity (0.0 to 1.0)
    glm::vec3 shadow_color;  // Shadow color (RGB)
};

/**
 * @brief Lighting system for collecting and managing scene lights
 *
 * The LightingSystem collects all active lights from the scene and
 * provides them to the renderer in a format suitable for shaders.
 */
class LightingSystem {
public:
    /**
     * @brief Maximum number of lights supported by shaders
     */
    static constexpr int MAX_LIGHTS = 32;

    /**
     * @brief Maximum number of shadow maps supported
     */
    static constexpr int MAX_SHADOW_MAPS = 8;

    /**
     * @brief Shadow quality levels
     */
    enum class ShadowQuality {
        Low = 0,    // 1024x1024, 4 PCF samples
        Medium = 1, // 2048x2048, 9 PCF samples
        High = 2,   // 4096x4096, 16 PCF samples
        Ultra = 3   // 4096x4096, 25 PCF samples
    };

    /**
     * @brief Shadow map resolution (configurable based on quality)
     */
    static constexpr int SHADOW_MAP_SIZE_LOW = 1024;
    static constexpr int SHADOW_MAP_SIZE_MEDIUM = 2048;
    static constexpr int SHADOW_MAP_SIZE_HIGH = 4096;
    static constexpr int SHADOW_MAP_SIZE_ULTRA = 4096;

    /**
     * @brief Light types for shader identification
     */
    enum class LightType {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    /**
     * @brief Constructor
     */
    LightingSystem();

    /**
     * @brief Destructor
     */
    ~LightingSystem() = default;

    /**
     * @brief Initialize the lighting system
     */
    void Initialize();

    /**
     * @brief Shutdown the lighting system
     */
    void Shutdown();

    /**
     * @brief Update lighting data from scene
     * @param scene Scene to collect lights from
     */
    void UpdateLights(Scene* scene);

    /**
     * @brief Get collected light data
     * @return Vector of light data structures
     */
    const std::vector<LightData>& GetLightData() const { return m_light_data; }

    /**
     * @brief Get number of active lights
     * @return Number of lights
     */
    int GetLightCount() const { return static_cast<int>(m_light_data.size()); }

    /**
     * @brief Bind lighting data to shader uniforms
     * @param shader_program OpenGL shader program ID
     */
    void BindLightingUniforms(unsigned int shader_program);

    /**
     * @brief Get ambient light color
     * @return Ambient light color (RGB)
     */
    const glm::vec3& GetAmbientLight() const { return m_ambient_light; }

    /**
     * @brief Set ambient light color
     * @param ambient Ambient light color (RGB)
     */
    void SetAmbientLight(const glm::vec3& ambient) { m_ambient_light = ambient; }

    /**
     * @brief Set shadow quality level
     * @param quality Shadow quality level
     */
    void SetShadowQuality(ShadowQuality quality);

    /**
     * @brief Get current shadow quality level
     * @return Current shadow quality level
     */
    ShadowQuality GetShadowQuality() const { return m_shadow_quality; }

    /**
     * @brief Get current shadow map size based on quality
     * @return Shadow map size in pixels
     */
    int GetShadowMapSize() const;

    /**
     * @brief Render shadow maps for all shadow-casting lights
     * @param scene Scene to render shadows for
     */
    void RenderShadowMaps(Scene* scene);

    /**
     * @brief Bind shadow maps to shader uniforms
     * @param shader_program OpenGL shader program ID
     */
    void BindShadowMaps(unsigned int shader_program);

    /**
     * @brief Enable or disable shadow rendering
     * @param enabled True to enable shadows, false to disable
     */
    void SetShadowsEnabled(bool enabled) { m_shadows_enabled = enabled; }

    /**
     * @brief Check if shadows are enabled
     * @return True if shadows are enabled
     */
    bool GetShadowsEnabled() const { return m_shadows_enabled; }

    /**
     * @brief Get shadow map texture array ID
     * @return OpenGL texture array ID for shadow maps
     */
    unsigned int GetShadowMapArray() const { return m_shadow_map_array; }

    /**
     * @brief Get current camera position for specular lighting calculations
     * @return Camera position in world space
     */
    glm::vec3 GetCameraPosition() const;

    /**
     * @brief Add a virtual directional light (e.g., from skybox sun)
     * @param direction Light direction
     * @param color Light color
     * @param intensity Light intensity
     * @param cast_shadows Whether this light should cast shadows
     * @param shadow_bias Shadow bias value
     * @param shadow_opacity Shadow opacity
     * @param shadow_color Shadow color
     */
    void AddVirtualDirectionalLight(const glm::vec3& direction, const glm::vec3& color,
                                   float intensity, bool cast_shadows = false,
                                   float shadow_bias = 0.001f, float shadow_opacity = 0.8f,
                                   const glm::vec3& shadow_color = glm::vec3(0.2f, 0.2f, 0.3f));

    /**
     * @brief Set fog parameters for scene rendering
     * @param enabled Whether fog is enabled
     * @param color Fog color
     * @param density Fog density
     * @param start Fog start distance
     * @param end Fog end distance
     * @param height_falloff Height-based fog falloff
     */
    void SetFogParameters(bool enabled, const glm::vec3& color, float density,
                         float start, float end, float height_falloff);

    /**
     * @brief Collect lights from scene (for multipass system)
     * @param scene Scene to collect lights from
     */
    virtual void CollectLights(Scene* scene) { UpdateLights(scene); }

protected:
    std::vector<LightData> m_light_data;
    glm::vec3 m_ambient_light;
    bool m_initialized;

    // Fog parameters
    bool m_fog_enabled;
    glm::vec3 m_fog_color;
    float m_fog_density;
    float m_fog_start;
    float m_fog_end;
    float m_fog_height_falloff;

    // Shadow mapping resources
    unsigned int m_shadow_map_array;        // Texture array for shadow maps
    unsigned int m_shadow_framebuffer;      // Framebuffer for shadow rendering
    unsigned int m_shadow_depth_buffer;     // Depth buffer for shadow rendering
    std::unique_ptr<Shader> m_shadow_shader; // Shader for shadow map generation
    int m_next_shadow_map_index;            // Next available shadow map index
    std::vector<glm::mat4> m_light_space_matrices; // Light space matrices for shadow mapping
    bool m_shadows_enabled;                 // Flag to enable/disable shadow rendering
    ShadowQuality m_shadow_quality;         // Current shadow quality level

    /**
     * @brief Convert OmniLight to LightData
     * @param omni_light OmniLight component
     * @param world_position Light world position
     * @return LightData structure
     */
    LightData ConvertOmniLight(OmniLight* omni_light, const glm::vec3& world_position);

    /**
     * @brief Convert DirectionalLight to LightData
     * @param dir_light DirectionalLight component
     * @param world_position Light world position
     * @return LightData structure
     */
    LightData ConvertDirectionalLight(DirectionalLight* dir_light, const glm::vec3& world_position);

    /**
     * @brief Convert SpotLight to LightData
     * @param spot_light SpotLight component
     * @param world_position Light world position
     * @return LightData structure
     */
    LightData ConvertSpotLight(SpotLight* spot_light, const glm::vec3& world_position);

    /**
     * @brief Initialize shadow mapping resources
     */
    void InitializeShadowMapping();

    /**
     * @brief Cleanup shadow mapping resources
     */
    void CleanupShadowMapping();

    /**
     * @brief Create shadow shader for depth rendering
     */
    void CreateShadowShader();

    /**
     * @brief Render shadow map for a specific light
     * @param light_data Light to render shadow map for
     * @param scene Scene to render
     */
    void RenderShadowMapForLight(const LightData& light_data, Scene* scene);

    /**
     * @brief Calculate light space matrix for directional light
     * @param light_data Light data
     * @return Light space matrix
     */
    glm::mat4 CalculateDirectionalLightMatrix(const LightData& light_data);

    /**
     * @brief Calculate light space matrix for point light
     * @param light_data Light data
     * @param face_index Cube face index (0-5)
     * @return Light space matrix
     */
    glm::mat4 CalculatePointLightMatrix(const LightData& light_data, int face_index);

    /**
     * @brief Calculate light space matrix for point light using directional approach
     * @param light_data Light data
     * @return Light space matrix
     */
    glm::mat4 CalculatePointLightAsDirectional(const LightData& light_data);

    /**
     * @brief Calculate light space matrix for spot light
     * @param light_data Light data
     * @return Light space matrix
     */
    glm::mat4 CalculateSpotLightMatrix(const LightData& light_data);

    /**
     * @brief Render scene geometry for shadow mapping
     * @param scene Scene to render
     * @param light_space_matrix Light space transformation matrix
     */
    void RenderSceneForShadows(Scene* scene, const glm::mat4& light_space_matrix);

    /**
     * @brief Render a single node for shadow mapping
     * @param node Node to render
     * @param light_space_matrix Light space transformation matrix
     */
    void RenderNodeForShadows(Node* node, const glm::mat4& light_space_matrix);
};

} // namespace Lupine
