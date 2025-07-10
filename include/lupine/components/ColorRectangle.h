#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>

namespace Lupine {

class Control;
class Node2D;

/**
 * @brief ColorRectangle UI component for displaying solid color rectangles
 *
 * ColorRectangle component displays solid color rectangles in UI.
 * It's a simple component for creating colored backgrounds, dividers,
 * or visual elements. It can be attached to Control or Node2D nodes.
 */
class ColorRectangle : public Component {
public:
    /**
     * @brief Constructor
     */
    ColorRectangle();

    /**
     * @brief Virtual destructor
     */
    virtual ~ColorRectangle() = default;

    /**
     * @brief Get rectangle color
     * @return Rectangle color (RGBA)
     */
    const glm::vec4& GetColor() const { return m_color; }

    /**
     * @brief Set rectangle color
     * @param color Rectangle color (RGBA)
     */
    void SetColor(const glm::vec4& color);

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
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "ColorRectangle"; }

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
    glm::vec4 m_color;
    float m_corner_radius;
    glm::vec4 m_border_color;
    float m_border_width;
    bool m_border_enabled;

    /**
     * @brief Render color rectangle
     * @param control Control node for positioning
     */
    void RenderRectangle(Control* control);

    /**
     * @brief Render color rectangle for Node2D
     * @param node2d Node2D for positioning
     */
    void RenderRectangleNode2D(Node2D* node2d);

    /**
     * @brief Render rectangle border
     * @param position Rectangle position
     * @param size Rectangle size
     */
    void RenderBorder(const glm::vec2& position, const glm::vec2& size);
};

} // namespace Lupine
