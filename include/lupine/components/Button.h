#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>
#include <functional>

namespace Lupine {

class Control;
class Node2D;

/**
 * @brief Button UI component for interactive buttons
 *
 * Button component provides clickable UI elements with text display,
 * visual states (normal, hover, pressed), and click event handling.
 * It can be attached to Control or Node2D nodes.
 */
class Button : public Component {
public:
    /**
     * @brief Button visual states
     */
    enum class ButtonState {
        Normal,
        Hover,
        Pressed,
        Disabled,
        Focused
    };

    /**
     * @brief Tween parameters for button animations
     */
    struct TweenParams {
        float duration = 0.15f;          // Animation duration in seconds
        bool scale_enabled = true;       // Enable scale tweening
        bool position_enabled = false;   // Enable position tweening
        glm::vec2 hover_scale = glm::vec2(1.05f, 1.05f);     // Scale on hover
        glm::vec2 pressed_scale = glm::vec2(0.95f, 0.95f);   // Scale when pressed
        glm::vec2 hover_offset = glm::vec2(0.0f, -2.0f);     // Position offset on hover
        glm::vec2 pressed_offset = glm::vec2(0.0f, 1.0f);    // Position offset when pressed
    };

    /**
     * @brief Text alignment options
     */
    enum class TextAlign {
        Left,
        Center,
        Right
    };

    /**
     * @brief Vertical text alignment options
     */
    enum class VerticalAlign {
        Top,
        Center,
        Bottom
    };

    /**
     * @brief Constructor
     */
    Button();

    /**
     * @brief Virtual destructor
     */
    virtual ~Button() = default;

    /**
     * @brief Get button text
     * @return Button text
     */
    const std::string& GetText() const { return m_text; }

    /**
     * @brief Set button text
     * @param text Button text
     */
    void SetText(const std::string& text);

    /**
     * @brief Get font path
     * @return FontPath structure
     */
    const FontPath& GetFontPath() const { return m_font_path; }

    /**
     * @brief Set font path
     * @param path FontPath structure
     */
    void SetFontPath(const FontPath& path);

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
     * @brief Get background color for normal state
     * @return Background color (RGBA)
     */
    const glm::vec4& GetBackgroundColor() const { return m_background_color; }

    /**
     * @brief Set background color for normal state
     * @param color Background color (RGBA)
     */
    void SetBackgroundColor(const glm::vec4& color);

    /**
     * @brief Get background color for hover state
     * @return Hover background color (RGBA)
     */
    const glm::vec4& GetHoverColor() const { return m_hover_color; }

    /**
     * @brief Set background color for hover state
     * @param color Hover background color (RGBA)
     */
    void SetHoverColor(const glm::vec4& color);

    /**
     * @brief Get background color for pressed state
     * @return Pressed background color (RGBA)
     */
    const glm::vec4& GetPressedColor() const { return m_pressed_color; }

    /**
     * @brief Set background color for pressed state
     * @param color Pressed background color (RGBA)
     */
    void SetPressedColor(const glm::vec4& color);

    /**
     * @brief Get background color for disabled state
     * @return Disabled background color (RGBA)
     */
    const glm::vec4& GetDisabledColor() const { return m_disabled_color; }

    /**
     * @brief Set background color for disabled state
     * @param color Disabled background color (RGBA)
     */
    void SetDisabledColor(const glm::vec4& color);

    /**
     * @brief Get hover modulation factor
     * @return Hover modulation factor
     */
    float GetHoverModulation() const { return m_hover_modulation; }

    /**
     * @brief Set hover modulation factor
     * @param modulation Hover modulation factor (multiplier for hover effects)
     */
    void SetHoverModulation(float modulation);

    /**
     * @brief Get click modulation factor
     * @return Click modulation factor
     */
    float GetClickModulation() const { return m_click_modulation; }

    /**
     * @brief Set click modulation factor
     * @param modulation Click modulation factor (multiplier for click effects)
     */
    void SetClickModulation(float modulation);

    /**
     * @brief Get disabled modulation factor
     * @return Disabled modulation factor
     */
    float GetDisabledModulation() const { return m_disabled_modulation; }

    /**
     * @brief Set disabled modulation factor
     * @param modulation Disabled modulation factor (multiplier for disabled effects)
     */
    void SetDisabledModulation(float modulation);

    /**
     * @brief Get tween parameters
     * @return Tween parameters
     */
    const TweenParams& GetTweenParams() const { return m_tween_params; }

    /**
     * @brief Set tween parameters
     * @param params Tween parameters
     */
    void SetTweenParams(const TweenParams& params);

    /**
     * @brief Get disabled state
     * @return True if button is disabled
     */
    bool IsDisabled() const { return m_disabled; }

    /**
     * @brief Set disabled state
     * @param disabled True to disable button
     */
    void SetDisabled(bool disabled);

    /**
     * @brief Set focused state
     * @param focused Whether button is focused
     */
    void SetFocused(bool focused);

    /**
     * @brief Get focused state
     * @return True if button is focused
     */
    bool IsFocused() const { return m_is_focused; }

    /**
     * @brief Get current button state
     * @return Current button state
     */
    ButtonState GetCurrentState() const { return m_current_state; }

    /**
     * @brief Set click callback function
     * @param callback Function to call when button is clicked
     */
    void SetOnClickCallback(std::function<void()> callback);

    // === Enhanced Button Properties ===

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
     * @brief Get text alignment
     * @return Text alignment
     */
    TextAlign GetTextAlign() const { return m_text_align; }

    /**
     * @brief Set text alignment
     * @param align Text alignment
     */
    void SetTextAlign(TextAlign align);

    /**
     * @brief Get vertical text alignment
     * @return Vertical text alignment
     */
    VerticalAlign GetVerticalAlign() const { return m_vertical_align; }

    /**
     * @brief Set vertical text alignment
     * @param align Vertical text alignment
     */
    void SetVerticalAlign(VerticalAlign align);

    /**
     * @brief Get icon texture path
     * @return Icon texture path
     */
    const std::string& GetIconPath() const { return m_icon_path; }

    /**
     * @brief Set icon texture path
     * @param path Icon texture path
     */
    void SetIconPath(const std::string& path);

    /**
     * @brief Get icon size
     * @return Icon size in pixels
     */
    const glm::vec2& GetIconSize() const { return m_icon_size; }

    /**
     * @brief Set icon size
     * @param size Icon size in pixels
     */
    void SetIconSize(const glm::vec2& size);

    /**
     * @brief Get padding
     * @return Padding (left, top, right, bottom)
     */
    const glm::vec4& GetPadding() const { return m_padding; }

    /**
     * @brief Set padding
     * @param padding Padding (left, top, right, bottom)
     */
    void SetPadding(const glm::vec4& padding);

    /**
     * @brief Set whether to use localization key for text
     * @param use_localization True to use localization key
     */
    void SetUseLocalizationKey(bool use_localization);

    /**
     * @brief Get whether localization key is used
     * @return True if using localization key
     */
    bool GetUseLocalizationKey() const { return m_use_localization_key; }

    /**
     * @brief Set localization key for text
     * @param key Localization key
     */
    void SetLocalizationKey(const std::string& key);

    /**
     * @brief Get localization key
     * @return Localization key
     */
    const std::string& GetLocalizationKey() const { return m_localization_key; }

    /**
     * @brief Get the actual text to display (localized if using key)
     * @return Display text
     */
    std::string GetDisplayText() const;

    /**
     * @brief Set click callback function
     * @param callback Function to call when button is clicked
     */
    void SetOnClickCallback(const std::function<void()>& callback) { m_on_click_callback = callback; }

    /**
     * @brief Check if mouse is currently over the button
     * @return True if mouse is over button
     */
    bool IsMouseOver() const { return m_is_mouse_over; }

    /**
     * @brief Check if button is currently pressed
     * @return True if button is pressed
     */
    bool IsPressed() const { return m_is_pressed; }

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Button"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "UI"; }

    // Component lifecycle methods
    virtual void OnReady() override;
    virtual void OnUpdate(float delta_time) override;
    virtual void OnInput(const void* event) override;
    virtual void OnDestroy() override;

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

    /**
     * @brief Get truncated text that fits within the given width
     * @param text Text to truncate
     * @param max_width Maximum width in pixels
     * @return Truncated text with ellipsis if needed
     */
    std::string GetTruncatedText(const std::string& text, float max_width) const;

    /**
     * @brief Get kerning between two characters
     * @param left Left character
     * @param right Right character
     * @return Kerning offset in pixels
     */
    float GetKerningBetweenChars(char left, char right) const;

private:
    // Basic properties
    std::string m_text;
    FontPath m_font_path;
    int m_font_size;
    glm::vec4 m_text_color;
    glm::vec4 m_background_color;
    glm::vec4 m_hover_color;
    glm::vec4 m_pressed_color;
    glm::vec4 m_disabled_color;
    bool m_disabled;
    ButtonState m_current_state;

    // Modulation parameters
    float m_hover_modulation;
    float m_click_modulation;
    float m_disabled_modulation;

    // Tweening parameters
    TweenParams m_tween_params;

    // Animation state
    float m_tween_time;
    ButtonState m_target_state;
    glm::vec2 m_current_scale;
    glm::vec2 m_current_offset;
    glm::vec2 m_start_scale;
    glm::vec2 m_start_offset;
    glm::vec2 m_target_scale;
    glm::vec2 m_target_offset;

    // Localization properties
    bool m_use_localization_key;
    std::string m_localization_key;

    // Enhanced properties
    float m_corner_radius;
    float m_border_width;
    glm::vec4 m_border_color;
    TextAlign m_text_align;
    VerticalAlign m_vertical_align;
    std::string m_icon_path;
    glm::vec2 m_icon_size;
    glm::vec4 m_padding; // left, top, right, bottom

    // Font rendering data
    void* m_font_handle;
    bool m_font_loaded;

    // Icon rendering data
    unsigned int m_icon_texture_id;
    bool m_icon_loaded;

    // Input handling
    bool m_is_mouse_over;
    bool m_is_pressed;
    bool m_is_focused;
    bool m_is_being_destroyed;

    // Event callbacks
    std::function<void()> m_on_click_callback;
    std::function<void()> m_on_hover_callback;
    std::function<void()> m_on_focus_callback;

    /**
     * @brief Load font for text rendering
     */
    void LoadFont();

    /**
     * @brief Load icon texture
     */
    void LoadIcon();

    /**
     * @brief Render button background with enhanced styling
     * @param control Control node for positioning
     */
    void RenderBackground(Control* control);

    /**
     * @brief Render button background for Node2D with enhanced styling
     * @param node2d Node2D for positioning
     */
    void RenderBackgroundNode2D(Node2D* node2d);

    /**
     * @brief Render button text with alignment for Control nodes
     * @param control Control node for positioning
     */
    void RenderTextControl(Control* control);

    /**
     * @brief Render button text for Node2D with alignment
     * @param node2d Node2D for positioning
     */
    void RenderTextNode2D(Node2D* node2d);

    /**
     * @brief Render button icon
     * @param control Control node for positioning
     */
    void RenderIcon(Control* control);

    /**
     * @brief Render button icon for Node2D
     * @param node2d Node2D for positioning
     */
    void RenderIconNode2D(Node2D* node2d);

    /**
     * @brief Render border if enabled
     * @param position Button position
     * @param size Button size
     */
    void RenderBorder(const glm::vec2& position, const glm::vec2& size);

    /**
     * @brief Check if point is inside button bounds
     * @param point Point to check
     * @return True if point is inside button
     */
    bool IsPointInside(const glm::vec2& point) const;

    /**
     * @brief Update button state based on input
     */
    void UpdateButtonState();

    /**
     * @brief Get current background color based on state
     * @return Background color for current state
     */
    glm::vec4 GetCurrentBackgroundColor() const;

    /**
     * @brief Calculate text position based on alignment
     * @param container_pos Container position
     * @param container_size Container size
     * @param text_size Text size
     * @return Calculated text position
     */
    glm::vec2 CalculateTextPosition(const glm::vec2& container_pos, const glm::vec2& container_size, const glm::vec2& text_size) const;

    /**
     * @brief Calculate icon position
     * @param container_pos Container position
     * @param container_size Container size
     * @return Calculated icon position
     */
    glm::vec2 CalculateIconPosition(const glm::vec2& container_pos, const glm::vec2& container_size) const;

    /**
     * @brief Update tweening animation
     * @param delta_time Time since last update in seconds
     */
    void UpdateTweening(float delta_time);

    /**
     * @brief Start tween to target state
     * @param target_state Target button state
     */
    void StartTween(ButtonState target_state);

    /**
     * @brief Apply easing function to interpolation value
     * @param t Interpolation value (0-1)
     * @return Eased interpolation value
     */
    float ApplyEasing(float t) const;

    /**
     * @brief Get scale for given button state
     * @param state Button state
     * @return Scale vector
     */
    glm::vec2 GetScaleForState(ButtonState state) const;

    /**
     * @brief Get offset for given button state
     * @param state Button state
     * @return Offset vector
     */
    glm::vec2 GetOffsetForState(ButtonState state) const;

    /**
     * @brief Update mouse hover state
     * @param mouse_pos Mouse position in screen coordinates
     */
    void UpdateMouseHover(const glm::vec2& mouse_pos);

    /**
     * @brief Transform screen coordinates to local coordinates
     * @param screen_pos Screen position
     * @return Local position relative to button
     */
    glm::vec2 ScreenToLocal(const glm::vec2& screen_pos) const;
};

} // namespace Lupine
