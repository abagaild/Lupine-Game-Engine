#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>

namespace Lupine {

class Node3D;

/**
 * @brief OmniLight component for omnidirectional point lighting
 *
 * OmniLight component provides omnidirectional point lighting with
 * distance attenuation, color control, intensity, and shadow support.
 * It should be attached to Node3D nodes.
 */
class OmniLight : public Component {
public:
    /**
     * @brief Shadow mode options
     */
    enum class ShadowMode {
        Disabled,       // No shadows
        Enabled         // Cast shadows
    };

    /**
     * @brief Constructor
     */
    OmniLight();

    /**
     * @brief Virtual destructor
     */
    virtual ~OmniLight() = default;

    /**
     * @brief Get light color
     * @return Light color (RGB)
     */
    glm::vec3 GetColor() const { return glm::vec3(m_color.r, m_color.g, m_color.b); }

    /**
     * @brief Set light color
     * @param color Light color (RGB)
     */
    void SetColor(const glm::vec3& color);

    /**
     * @brief Get light intensity
     * @return Light intensity multiplier
     */
    float GetIntensity() const { return m_intensity; }

    /**
     * @brief Set light intensity
     * @param intensity Light intensity multiplier
     */
    void SetIntensity(float intensity);

    /**
     * @brief Get light range
     * @return Maximum light range in world units
     */
    float GetRange() const { return m_range; }

    /**
     * @brief Set light range
     * @param range Maximum light range in world units
     */
    void SetRange(float range);

    /**
     * @brief Get attenuation constant
     * @return Constant attenuation factor
     */
    float GetAttenuationConstant() const { return m_attenuation_constant; }

    /**
     * @brief Set attenuation constant
     * @param constant Constant attenuation factor
     */
    void SetAttenuationConstant(float constant);

    /**
     * @brief Get attenuation linear
     * @return Linear attenuation factor
     */
    float GetAttenuationLinear() const { return m_attenuation_linear; }

    /**
     * @brief Set attenuation linear
     * @param linear Linear attenuation factor
     */
    void SetAttenuationLinear(float linear);

    /**
     * @brief Get attenuation quadratic
     * @return Quadratic attenuation factor
     */
    float GetAttenuationQuadratic() const { return m_attenuation_quadratic; }

    /**
     * @brief Set attenuation quadratic
     * @param quadratic Quadratic attenuation factor
     */
    void SetAttenuationQuadratic(float quadratic);

    /**
     * @brief Get whether light is enabled
     * @return True if light is enabled
     */
    bool IsEnabled() const { return m_enabled; }

    /**
     * @brief Set whether light is enabled
     * @param enabled True to enable light
     */
    void SetEnabled(bool enabled);

    /**
     * @brief Get shadow mode
     * @return Current shadow mode
     */
    ShadowMode GetShadowMode() const { return m_shadow_mode; }

    /**
     * @brief Set shadow mode
     * @param mode Shadow mode
     */
    void SetShadowMode(ShadowMode mode);

    /**
     * @brief Get shadow opacity
     * @return Shadow opacity (0.0 to 1.0)
     */
    float GetShadowOpacity() const { return m_shadow_opacity; }

    /**
     * @brief Set shadow opacity
     * @param opacity Shadow opacity (0.0 to 1.0)
     */
    void SetShadowOpacity(float opacity);

    /**
     * @brief Get shadow bias
     * @return Shadow bias to prevent shadow acne
     */
    float GetShadowBias() const { return m_shadow_bias; }

    /**
     * @brief Set shadow bias
     * @param bias Shadow bias to prevent shadow acne
     */
    void SetShadowBias(float bias);

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
     * @brief Get whether to show debug visualization
     * @return True if debug visualization is enabled
     */
    bool IsDebugEnabled() const { return m_debug_enabled; }

    /**
     * @brief Set whether to show debug visualization
     * @param enabled True to enable debug visualization
     */
    void SetDebugEnabled(bool enabled);

    /**
     * @brief Get light position in world space
     * @return World position of the light
     */
    glm::vec3 GetWorldPosition() const;

    /**
     * @brief Calculate light attenuation at given distance
     * @param distance Distance from light
     * @return Attenuation factor (0.0 to 1.0)
     */
    float CalculateAttenuation(float distance) const;

    /**
     * @brief Check if point is within light range
     * @param world_position Point in world space
     * @return True if point is within light range
     */
    bool IsPointInRange(const glm::vec3& world_position) const;

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "OmniLight"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "Light"; }

    // Component lifecycle methods
    virtual void OnReady() override;
    virtual void OnUpdate(float delta_time) override;

protected:
    /**
     * @brief Initialize export variables
     */
    virtual void InitializeExportVariables() override;

    /**
     * @brief Update export variables
     */
    void UpdateExportVariables();

    /**
     * @brief Update from export variables
     */
    void UpdateFromExportVariables();

private:
    // Light properties
    glm::vec4 m_color;
    float m_intensity;
    float m_range;
    bool m_enabled;

    // Attenuation properties
    float m_attenuation_constant;
    float m_attenuation_linear;
    float m_attenuation_quadratic;

    // Shadow properties
    ShadowMode m_shadow_mode;
    float m_shadow_opacity;
    float m_shadow_bias;
    glm::vec4 m_shadow_color;

    // Debug properties
    bool m_debug_enabled;

    // Internal rendering data
    unsigned int m_debug_sphere_vao;
    unsigned int m_debug_sphere_vbo;
    unsigned int m_debug_sphere_ebo;
    bool m_debug_mesh_initialized;

    /**
     * @brief Initialize debug visualization mesh
     */
    void InitializeDebugMesh();

    /**
     * @brief Render debug visualization
     * @param node3d Node3D for positioning
     */
    void RenderDebugVisualization(Node3D* node3d);

    /**
     * @brief Generate sphere mesh for debug visualization
     * @param radius Sphere radius
     * @param segments Number of segments
     */
    void GenerateSphereMesh(float radius, int segments);
};

} // namespace Lupine
