#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>

// Forward declarations
namespace Lupine {
    class Control;
    class Node2D;
}

namespace Lupine {

/**
 * @brief Text label component for rendering text
 * 
 * Label component renders text using a specified font.
 * It should be attached to Control nodes for UI text.
 */
class Label : public Component {
public:
    /**
     * @brief Text alignment options
     */
    enum class TextAlign {
        Left,
        Center,
        Right,
        Justify
    };
    
    /**
     * @brief Vertical alignment options
     */
    enum class VerticalAlign {
        Top,
        Center,
        Bottom
    };
    
    /**
     * @brief Constructor
     */
    Label();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Label() = default;
    
    /**
     * @brief Get text content
     * @return Text string
     */
    const std::string& GetText() const { return m_text; }
    
    /**
     * @brief Set text content
     * @param text New text string
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
    const glm::vec4& GetColor() const { return m_color; }
    
    /**
     * @brief Set text color
     * @param color Text color (RGBA)
     */
    void SetColor(const glm::vec4& color);
    
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
     * @brief Get vertical alignment
     * @return Vertical alignment
     */
    VerticalAlign GetVerticalAlign() const { return m_vertical_align; }
    
    /**
     * @brief Set vertical alignment
     * @param align Vertical alignment
     */
    void SetVerticalAlign(VerticalAlign align);
    
    /**
     * @brief Get word wrap flag
     * @return True if word wrap is enabled
     */
    bool GetWordWrap() const { return m_word_wrap; }
    
    /**
     * @brief Set word wrap flag
     * @param wrap True to enable word wrap
     */
    void SetWordWrap(bool wrap);
    
    /**
     * @brief Get line spacing multiplier
     * @return Line spacing multiplier
     */
    float GetLineSpacing() const { return m_line_spacing; }
    
    /**
     * @brief Set line spacing multiplier
     * @param spacing Line spacing multiplier
     */
    void SetLineSpacing(float spacing);
    
    /**
     * @brief Calculate text size
     * @return Text size in pixels
     */
    glm::vec2 CalculateTextSize() const;

    /**
     * @brief Enable/disable text outline
     * @param enabled True to enable outline
     */
    void SetOutlineEnabled(bool enabled);

    /**
     * @brief Get outline enabled state
     * @return True if outline is enabled
     */
    bool GetOutlineEnabled() const { return m_outline_enabled; }

    /**
     * @brief Set outline color
     * @param color Outline color (RGBA)
     */
    void SetOutlineColor(const glm::vec4& color);

    /**
     * @brief Get outline color
     * @return Outline color (RGBA)
     */
    const glm::vec4& GetOutlineColor() const { return m_outline_color; }

    /**
     * @brief Set outline width
     * @param width Outline width in pixels
     */
    void SetOutlineWidth(float width);

    /**
     * @brief Get outline width
     * @return Outline width in pixels
     */
    float GetOutlineWidth() const { return m_outline_width; }

    /**
     * @brief Enable/disable text shadow
     * @param enabled True to enable shadow
     */
    void SetShadowEnabled(bool enabled);

    /**
     * @brief Get shadow enabled state
     * @return True if shadow is enabled
     */
    bool GetShadowEnabled() const { return m_shadow_enabled; }

    /**
     * @brief Set shadow color
     * @param color Shadow color (RGBA)
     */
    void SetShadowColor(const glm::vec4& color);

    /**
     * @brief Get shadow color
     * @return Shadow color (RGBA)
     */
    const glm::vec4& GetShadowColor() const { return m_shadow_color; }

    /**
     * @brief Set shadow offset
     * @param offset Shadow offset (X, Y)
     */
    void SetShadowOffset(const glm::vec2& offset);

    /**
     * @brief Get shadow offset
     * @return Shadow offset (X, Y)
     */
    const glm::vec2& GetShadowOffset() const { return m_shadow_offset; }

    /**
     * @brief Set kerning adjustment
     * @param kerning Kerning adjustment value (additional spacing between characters)
     */
    void SetKerning(float kerning);

    /**
     * @brief Get kerning adjustment
     * @return Kerning adjustment value
     */
    float GetKerning() const { return m_kerning; }

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
     * @brief Get component type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Label"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "UI"; }

    // Lifecycle methods
    void OnReady() override;
    void OnUpdate(float delta_time) override;

protected:
    void InitializeExportVariables() override;

private:
    std::string m_text;
    FontPath m_font_path;
    int m_font_size;
    glm::vec4 m_color;
    TextAlign m_text_align;
    VerticalAlign m_vertical_align;
    bool m_word_wrap;
    float m_line_spacing;
    float m_kerning; // Kerning adjustment factor

    // Advanced text rendering properties
    bool m_outline_enabled;
    glm::vec4 m_outline_color;
    float m_outline_width;
    bool m_shadow_enabled;
    glm::vec4 m_shadow_color;
    glm::vec2 m_shadow_offset;
    float m_shadow_blur;

    // Localization properties
    bool m_use_localization_key;
    std::string m_localization_key;

    // Internal rendering data
    void* m_font_handle; // TTF_Font* or similar
    bool m_font_loaded;
    bool m_is_being_destroyed;
    
    /**
     * @brief Load font from file
     */
    void LoadFont();

    /**
     * @brief Get kerning between two characters
     * @param left Left character
     * @param right Right character
     * @return Kerning offset in pixels
     */
    float GetKerningBetweenChars(char left, char right) const;

    /**
     * @brief Update export variables from internal state
     */
    void UpdateExportVariables();
    
    /**
     * @brief Update internal state from export variables
     */
    void UpdateFromExportVariables();
    
    /**
     * @brief Convert TextAlign enum to int for export variables
     */
    int TextAlignToInt(TextAlign align) const;
    
    /**
     * @brief Convert int to TextAlign enum from export variables
     */
    TextAlign IntToTextAlign(int align) const;
    
    /**
     * @brief Convert VerticalAlign enum to int for export variables
     */
    int VerticalAlignToInt(VerticalAlign align) const;
    
    /**
     * @brief Convert int to VerticalAlign enum from export variables
     */
    VerticalAlign IntToVerticalAlign(int align) const;

    /**
     * @brief Render text using the loaded font for Control nodes
     * @param control Control node that owns this label
     */
    void RenderTextControl(Control* control);

    /**
     * @brief Render text using the loaded font for Node2D nodes
     * @param node2d Node2D node that owns this label
     */
    void RenderTextNode2D(Node2D* node2d);
};

} // namespace Lupine
