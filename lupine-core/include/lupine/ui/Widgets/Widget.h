#pragma once

#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace LupineUI {

/**
 * @brief Base widget class for all UI elements
 */
class Widget {
public:
    Widget();
    virtual ~Widget();
    
    /**
     * @brief Update the widget
     * @param deltaTime Time since last frame
     */
    virtual void Update(float deltaTime);
    
    /**
     * @brief Render the widget
     */
    virtual void Render();
    
    /**
     * @brief Add a child widget
     * @param child Child widget to add
     */
    virtual void AddChild(std::shared_ptr<Widget> child);
    
    /**
     * @brief Remove a child widget
     * @param child Child widget to remove
     */
    virtual void RemoveChild(std::shared_ptr<Widget> child);
    
    /**
     * @brief Get all child widgets
     * @return Vector of child widgets
     */
    const std::vector<std::shared_ptr<Widget>>& GetChildren() const { return m_children; }
    
    /**
     * @brief Set widget position
     * @param position New position
     */
    void SetPosition(const glm::vec2& position) { m_position = position; }
    
    /**
     * @brief Get widget position
     * @return Widget position
     */
    const glm::vec2& GetPosition() const { return m_position; }
    
    /**
     * @brief Set widget size
     * @param size New size
     */
    void SetSize(const glm::vec2& size) { m_size = size; }
    
    /**
     * @brief Get widget size
     * @return Widget size
     */
    const glm::vec2& GetSize() const { return m_size; }
    
    /**
     * @brief Set widget visibility
     * @param visible Visibility state
     */
    void SetVisible(bool visible) { m_visible = visible; }
    
    /**
     * @brief Get widget visibility
     * @return Visibility state
     */
    bool IsVisible() const { return m_visible; }
    
    /**
     * @brief Set widget enabled state
     * @param enabled Enabled state
     */
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    
    /**
     * @brief Get widget enabled state
     * @return Enabled state
     */
    bool IsEnabled() const { return m_enabled; }

protected:
    glm::vec2 m_position = glm::vec2(0.0f);
    glm::vec2 m_size = glm::vec2(100.0f);
    bool m_visible = true;
    bool m_enabled = true;
    std::vector<std::shared_ptr<Widget>> m_children;
    Widget* m_parent = nullptr;
};

} // namespace LupineUI
