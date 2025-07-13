#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>

namespace Lupine {

class Control;
class Node2D;

/**
 * @brief ProgressBar UI component for displaying progress
 *
 * ProgressBar component provides a visual representation of progress
 * with customizable fill direction, colors, and styling options.
 * It can be attached to Control or Node2D nodes.
 */
class ProgressBar : public Component {
public:
    /**
     * @brief Fill direction options
     */
    enum class FillDirection {
        LeftToRight,
        RightToLeft,
        TopToBottom,
        BottomToTop
    };

    /**
     * @brief Constructor
     */
    ProgressBar();

    /**
     * @brief Virtual destructor
     */
    virtual ~ProgressBar() = default;

    /**
     * @brief Get current value
     * @return Current progress value
     */
    float GetValue() const { return m_value; }

    /**
     * @brief Set current value
     * @param value Progress value (clamped to min/max range)
     */
    void SetValue(float value);

    /**
     * @brief Get minimum value
     * @return Minimum progress value
     */
    float GetMinValue() const { return m_min_value; }

    /**
     * @brief Set minimum value
     * @param min_value Minimum progress value
     */
    void SetMinValue(float min_value);

    /**
     * @brief Get maximum value
     * @return Maximum progress value
     */
    float GetMaxValue() const { return m_max_value; }

    /**
     * @brief Set maximum value
     * @param max_value Maximum progress value
     */
    void SetMaxValue(float max_value);

    /**
     * @brief Get progress as percentage (0.0 to 1.0)
     * @return Progress percentage
     */
    float GetProgress() const;

    /**
     * @brief Set progress as percentage (0.0 to 1.0)
     * @param progress Progress percentage
     */
    void SetProgress(float progress);

    /**
     * @brief Get fill direction
     * @return Fill direction
     */
    FillDirection GetFillDirection() const { return m_fill_direction; }

    /**
     * @brief Set fill direction
     * @param direction Fill direction
     */
    void SetFillDirection(FillDirection direction);

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
     * @brief Get fill color
     * @return Fill color (RGBA)
     */
    const glm::vec4& GetFillColor() const { return m_fill_color; }

    /**
     * @brief Set fill color
     * @param color Fill color (RGBA)
     */
    void SetFillColor(const glm::vec4& color);

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
     * @brief Get corner radius
     * @return Corner radius in pixels
     */
    float GetCornerRadius() const { return m_corner_radius; }

    /**
     * @brief Set corner radius
     * @param radius Corner radius in pixels
     */
    void SetCornerRadius(float radius);

    /**
     * @brief Get whether to show percentage text
     * @return True if percentage text is shown
     */
    bool GetShowPercentage() const { return m_show_percentage; }

    /**
     * @brief Set whether to show percentage text
     * @param show True to show percentage text
     */
    void SetShowPercentage(bool show);

    /**
     * @brief Get custom text
     * @return Custom text string
     */
    const std::string& GetCustomText() const { return m_custom_text; }

    /**
     * @brief Set custom text (overrides percentage display when not empty)
     * @param text Custom text to display
     */
    void SetCustomText(const std::string& text);

    /**
     * @brief Get text color
     * @return Text color (RGBA)
     */
    const glm::vec4& GetTextColor() const { return m_text_color; }

    /**
     * @brief Set text color
     * @param color Text color (RGBA)
     */
    void SetTextColor(const glm::vec4& color);

    /**
     * @brief Get font size
     * @return Font size in pixels
     */
    int GetFontSize() const { return m_font_size; }

    /**
     * @brief Set font size
     * @param size Font size in pixels
     */
    void SetFontSize(int size);

    /**
     * @brief Get font path
     * @return Font path structure
     */
    const FontPath& GetFontPath() const { return m_font_path; }

    /**
     * @brief Set font path
     * @param path Font path structure
     */
    void SetFontPath(const FontPath& path);

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "ProgressBar"; }

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
    // Progress properties
    float m_value;
    float m_min_value;
    float m_max_value;
    FillDirection m_fill_direction;

    // Visual properties
    glm::vec4 m_background_color;
    glm::vec4 m_fill_color;
    glm::vec4 m_border_color;
    float m_border_width;
    float m_corner_radius;

    // Text properties
    bool m_show_percentage;
    std::string m_custom_text;
    glm::vec4 m_text_color;
    int m_font_size;
    FontPath m_font_path;

    // Font rendering data
    void* m_font_handle;
    bool m_font_loaded;

    /**
     * @brief Load font for text rendering
     */
    void LoadFont();

    /**
     * @brief Render progress bar background
     * @param control Control node for positioning
     */
    void RenderBackground(Control* control);

    /**
     * @brief Render progress bar background for Node2D
     * @param node2d Node2D for positioning
     */
    void RenderBackgroundNode2D(Node2D* node2d);

    /**
     * @brief Render progress bar fill
     * @param control Control node for positioning
     */
    void RenderFill(Control* control);

    /**
     * @brief Render progress bar fill for Node2D
     * @param node2d Node2D for positioning
     */
    void RenderFillNode2D(Node2D* node2d);

    /**
     * @brief Render border if enabled
     * @param position Bar position
     * @param size Bar size
     */
    void RenderBorder(const glm::vec2& position, const glm::vec2& size);

    /**
     * @brief Render percentage text if enabled
     * @param control Control node for positioning
     */
    void RenderText(Control* control);

    /**
     * @brief Render percentage text for Node2D if enabled
     * @param node2d Node2D for positioning
     */
    void RenderTextNode2D(Node2D* node2d);

    /**
     * @brief Calculate fill rectangle based on progress and direction
     * @param container_pos Container position
     * @param container_size Container size
     * @return Fill rectangle (position and size)
     */
    glm::vec4 CalculateFillRect(const glm::vec2& container_pos, const glm::vec2& container_size) const;

    /**
     * @brief Clamp value to min/max range
     * @param value Value to clamp
     * @return Clamped value
     */
    float ClampValue(float value) const;
};

} // namespace Lupine
