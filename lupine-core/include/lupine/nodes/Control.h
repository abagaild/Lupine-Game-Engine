#pragma once

#include "lupine/core/Node.h"
#include <glm/glm.hpp>

namespace Lupine {

/**
 * @brief Control node for UI elements in screen space
 * 
 * Control represents UI elements with screen-space coordinates,
 * anchoring, margins, and size properties.
 */
class Control : public Node {
public:
    /**
     * @brief Anchor presets for common UI layouts
     */
    enum class AnchorPreset {
        TopLeft,
        TopCenter,
        TopRight,
        CenterLeft,
        Center,
        CenterRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
        LeftWide,
        TopWide,
        RightWide,
        BottomWide,
        VCenterWide,
        HCenterWide,
        FullRect
    };
    
    /**
     * @brief Constructor
     * @param name Name of the node
     */
    explicit Control(const std::string& name = "Control");
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Control() = default;
    
    /**
     * @brief Get position in screen space
     * @return 2D position vector
     */
    const glm::vec2& GetPosition() const { return m_position; }
    
    /**
     * @brief Set position in screen space
     * @param position New position
     */
    void SetPosition(const glm::vec2& position) { m_position = position; }
    
    /**
     * @brief Set position in screen space
     * @param x X coordinate
     * @param y Y coordinate
     */
    void SetPosition(float x, float y) { m_position = glm::vec2(x, y); }
    
    /**
     * @brief Get size of the control
     * @return 2D size vector
     */
    const glm::vec2& GetSize() const { return m_size; }
    
    /**
     * @brief Set size of the control
     * @param size New size
     */
    void SetSize(const glm::vec2& size) { m_size = size; }
    
    /**
     * @brief Set size of the control
     * @param width Width
     * @param height Height
     */
    void SetSize(float width, float height) { m_size = glm::vec2(width, height); }
    
    /**
     * @brief Get anchor minimum (0-1 range)
     * @return Anchor minimum vector
     */
    const glm::vec2& GetAnchorMin() const { return m_anchor_min; }
    
    /**
     * @brief Set anchor minimum (0-1 range)
     * @param anchor_min New anchor minimum
     */
    void SetAnchorMin(const glm::vec2& anchor_min) { m_anchor_min = anchor_min; }
    
    /**
     * @brief Get anchor maximum (0-1 range)
     * @return Anchor maximum vector
     */
    const glm::vec2& GetAnchorMax() const { return m_anchor_max; }
    
    /**
     * @brief Set anchor maximum (0-1 range)
     * @param anchor_max New anchor maximum
     */
    void SetAnchorMax(const glm::vec2& anchor_max) { m_anchor_max = anchor_max; }
    
    /**
     * @brief Set anchors using preset
     * @param preset Anchor preset
     */
    void SetAnchorPreset(AnchorPreset preset);
    
    /**
     * @brief Get margin left
     * @return Left margin
     */
    float GetMarginLeft() const { return m_margin_left; }
    
    /**
     * @brief Set margin left
     * @param margin Left margin
     */
    void SetMarginLeft(float margin) { m_margin_left = margin; }
    
    /**
     * @brief Get margin top
     * @return Top margin
     */
    float GetMarginTop() const { return m_margin_top; }
    
    /**
     * @brief Set margin top
     * @param margin Top margin
     */
    void SetMarginTop(float margin) { m_margin_top = margin; }
    
    /**
     * @brief Get margin right
     * @return Right margin
     */
    float GetMarginRight() const { return m_margin_right; }
    
    /**
     * @brief Set margin right
     * @param margin Right margin
     */
    void SetMarginRight(float margin) { m_margin_right = margin; }
    
    /**
     * @brief Get margin bottom
     * @return Bottom margin
     */
    float GetMarginBottom() const { return m_margin_bottom; }
    
    /**
     * @brief Set margin bottom
     * @param margin Bottom margin
     */
    void SetMarginBottom(float margin) { m_margin_bottom = margin; }
    
    /**
     * @brief Set all margins
     * @param left Left margin
     * @param top Top margin
     * @param right Right margin
     * @param bottom Bottom margin
     */
    void SetMargins(float left, float top, float right, float bottom);
    
    /**
     * @brief Get global rectangle (position and size in screen space)
     * @return Rectangle (x, y, width, height)
     */
    glm::vec4 GetGlobalRect() const;
    
    /**
     * @brief Check if point is inside the control
     * @param point Point in screen space
     * @return True if point is inside
     */
    bool ContainsPoint(const glm::vec2& point) const;
    
    /**
     * @brief Get whether this control renders in world space
     * @return True if renders in world space (affected by camera), false for screen space (UI overlay)
     */
    bool GetWorldSpace() const { return m_world_space; }

    /**
     * @brief Set whether this control renders in world space
     * @param world_space True for world space (affected by camera), false for screen space (UI overlay)
     */
    void SetWorldSpace(bool world_space) { m_world_space = world_space; }

    /**
     * @brief Get node type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Control"; }

protected:
    /**
     * @brief Copy type-specific properties to another node (override for Control)
     * @param target Target node to copy properties to
     */
    void CopyTypeSpecificProperties(Node* target) const override;

    glm::vec2 m_position;
    glm::vec2 m_size;
    glm::vec2 m_anchor_min;
    glm::vec2 m_anchor_max;
    float m_margin_left;
    float m_margin_top;
    float m_margin_right;
    float m_margin_bottom;
    bool m_world_space;
    
    /**
     * @brief Get parent control size for anchor calculations
     * @return Parent size, or screen size if no parent
     */
    glm::vec2 GetParentSize() const;
};

} // namespace Lupine
