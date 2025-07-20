#pragma once

#include "Widget.h"
#include <string>
#include <glm/glm.hpp>

namespace LupineUI {

/**
 * @brief Label widget for displaying text
 */
class Label : public Widget {
public:
    /**
     * @brief Constructor
     * @param text Initial text
     */
    explicit Label(const std::string& text = "");
    
    /**
     * @brief Destructor
     */
    ~Label() override;
    
    /**
     * @brief Render the label
     */
    void Render() override;
    
    /**
     * @brief Set the text
     * @param text New text
     */
    void SetText(const std::string& text);
    
    /**
     * @brief Get the text
     * @return Current text
     */
    const std::string& GetText() const { return m_text; }
    
    /**
     * @brief Set the font size
     * @param size Font size
     */
    void SetFontSize(float size) { m_font_size = size; }
    
    /**
     * @brief Get the font size
     * @return Font size
     */
    float GetFontSize() const { return m_font_size; }
    
    /**
     * @brief Set the text color
     * @param color Text color
     */
    void SetTextColor(const glm::vec4& color) { m_text_color = color; }
    
    /**
     * @brief Get the text color
     * @return Text color
     */
    const glm::vec4& GetTextColor() const { return m_text_color; }

private:
    std::string m_text;
    float m_font_size = 14.0f;
    glm::vec4 m_text_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};

} // namespace LupineUI
