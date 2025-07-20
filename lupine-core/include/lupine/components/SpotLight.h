#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>

namespace Lupine {

class Node3D;

/**
 * @brief SpotLight component for cone-shaped lighting
 *
 * SpotLight component provides cone-shaped lighting with distance attenuation,
 * angular falloff, color control, intensity, and shadow support.
 * It should be attached to Node3D nodes.
 */
class SpotLight : public Component {
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
    SpotLight();

    /**
     * @brief Virtual destructor
     */
    virtual ~SpotLight() = default;

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
     * @brief Get inner cone angle in degrees
     * @return Inner cone angle (full intensity)
     */
    float GetInnerConeAngle() const { return m_inner_cone_angle; }

    /**
     * @brief Set inner cone angle in degrees
     * @param angle Inner cone angle (full intensity)
     */
    void SetInnerConeAngle(float angle);

    /**
     * @brief Get outer cone angle in degrees
     * @return Outer cone angle (falloff to zero)
     */
    float GetOuterConeAngle() const { return m_outer_cone_angle; }

    /**
     * @brief Set outer cone angle in degrees
     * @param angle Outer cone angle (falloff to zero)
     */
    void SetOuterConeAngle(float angle);

    /**
     * @brief Get light direction in world space
     * @return Normalized direction vector
     */
    glm::vec3 GetDirection() const;

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
     * @brief Calculate angular attenuation for given direction
     * @param direction Direction from light to point (normalized)
     * @return Angular attenuation factor (0.0 to 1.0)
     */
    float CalculateAngularAttenuation(const glm::vec3& direction) const;

    /**
     * @brief Check if point is within light cone and range
     * @param world_position Point in world space
     * @return True if point is within light influence
     */
    bool IsPointInCone(const glm::vec3& world_position) const;

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "SpotLight"; }

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
    float m_inner_cone_angle;
    float m_outer_cone_angle;
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
    unsigned int m_debug_cone_vao;
    unsigned int m_debug_cone_vbo;
    unsigned int m_debug_cone_ebo;
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
     * @brief Generate cone mesh for debug visualization
     * @param range Cone range (height)
     * @param angle Cone angle in radians
     */
    void GenerateConeMesh(float range, float angle);
};

} // namespace Lupine
