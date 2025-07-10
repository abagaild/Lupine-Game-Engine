#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>

namespace Lupine {

class Control;
class Node2D;

/**
 * @brief Panel UI component for basic rectangular background containers
 *
 * Panel component provides a simple rectangular background that can be used
 * as a container or visual element. It supports solid colors and can be
 * attached to Control or Node2D nodes.
 */
class Panel : public Component {
public:
    /**
     * @brief Constructor
     */
    Panel();

    /**
     * @brief Virtual destructor
     */
    virtual ~Panel() = default;

    /**
     * @brief Get background color
     * @return Background color (RGBA)
     */
    const glm::vec4& GetBackgroundColor() const { return m_background_color; }

    /**
     * @brief Set background color
     * @param color Background color (RGBA)
     */
    void SetBackgroundColor(const glm::vec4& color);

    /**
     * @brief Get border color
     * @return Border color (RGBA)
     */
    const glm::vec4& GetBorderColor() const { return m_border_color; }

    /**
     * @brief Set border color
     * @param color Border color (RGBA)
     */
    void SetBorderColor(const glm::vec4& color);

    /**
     * @brief Get border width
     * @return Border width in pixels
     */
    float GetBorderWidth() const { return m_border_width; }

    /**
     * @brief Set border width
     * @param width Border width in pixels
     */
    void SetBorderWidth(float width);

    /**
     * @brief Get whether border is enabled
     * @return True if border is enabled
     */
    bool IsBorderEnabled() const { return m_border_enabled; }

    /**
     * @brief Set whether border is enabled
     * @param enabled True to enable border
     */
    void SetBorderEnabled(bool enabled);

    /**
     * @brief Get corner radius for rounded corners
     * @return Corner radius in pixels
     */
    float GetCornerRadius() const { return m_corner_radius; }

    /**
     * @brief Set corner radius for rounded corners
     * @param radius Corner radius in pixels
     */
    void SetCornerRadius(float radius);

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Panel"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "UI"; }

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
    glm::vec4 m_background_color;
    glm::vec4 m_border_color;
    float m_border_width;
    bool m_border_enabled;
    float m_corner_radius;

    /**
     * @brief Render panel background
     * @param control Control node for positioning
     */
    void RenderPanel(Control* control);

    /**
     * @brief Render panel background for Node2D
     * @param node2d Node2D for positioning
     */
    void RenderPanelNode2D(Node2D* node2d);

    /**
     * @brief Render panel border
     * @param position Panel position
     * @param size Panel size
     */
    void RenderBorder(const glm::vec2& position, const glm::vec2& size);
};

} // namespace Lupine
