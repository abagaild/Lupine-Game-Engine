#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>

namespace Lupine {

class Node3D;
class GraphicsVertexArray;
class GraphicsBuffer;

/**
 * @brief DirectionalLight component for directional lighting (like sunlight)
 *
 * DirectionalLight component provides directional lighting that affects
 * all objects uniformly regardless of distance. Commonly used for sunlight
 * or moonlight. It should be attached to Node3D nodes.
 */
class DirectionalLight : public Component {
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
    DirectionalLight();

    /**
     * @brief Virtual destructor
     */
    virtual ~DirectionalLight() = default;

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
     * @brief Get light direction in world space
     * @return Normalized direction vector
     */
    glm::vec3 GetDirection() const;

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
     * @brief Get shadow cascade count
     * @return Number of shadow cascades
     */
    int GetShadowCascades() const { return m_shadow_cascades; }

    /**
     * @brief Set shadow cascade count
     * @param cascades Number of shadow cascades
     */
    void SetShadowCascades(int cascades);

    /**
     * @brief Get shadow distance
     * @return Maximum shadow rendering distance
     */
    float GetShadowDistance() const { return m_shadow_distance; }

    /**
     * @brief Set shadow distance
     * @param distance Maximum shadow rendering distance
     */
    void SetShadowDistance(float distance);

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
     * @brief Calculate light contribution for a surface
     * @param surface_normal Surface normal vector
     * @return Light contribution factor (0.0 to 1.0)
     */
    float CalculateLightContribution(const glm::vec3& surface_normal) const;

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "DirectionalLight"; }

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
    bool m_enabled;

    // Shadow properties
    ShadowMode m_shadow_mode;
    float m_shadow_opacity;
    float m_shadow_bias;
    int m_shadow_cascades;
    float m_shadow_distance;
    glm::vec4 m_shadow_color;

    // Debug properties
    bool m_debug_enabled;

    // Internal rendering data
    std::shared_ptr<GraphicsVertexArray> m_debug_arrow_vao;
    std::shared_ptr<GraphicsBuffer> m_debug_arrow_vbo;
    std::shared_ptr<GraphicsBuffer> m_debug_arrow_ebo;
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
     * @brief Generate arrow mesh for debug visualization
     * @param length Arrow length
     */
    void GenerateArrowMesh(float length);
};

} // namespace Lupine
