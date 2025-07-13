#include "lupine/nodes/Control.h"

namespace Lupine {

Control::Control(const std::string& name)
    : Node(name)
    , m_position(0.0f, 0.0f)
    , m_size(100.0f, 100.0f)
    , m_anchor_min(0.0f, 0.0f)
    , m_anchor_max(0.0f, 0.0f)
    , m_margin_left(0.0f)
    , m_margin_top(0.0f)
    , m_margin_right(0.0f)
    , m_margin_bottom(0.0f)
    , m_world_space(false) { // Default to screen space (UI overlay)
}

void Control::SetAnchorPreset(AnchorPreset preset) {
    switch (preset) {
        case AnchorPreset::TopLeft:
            m_anchor_min = glm::vec2(0.0f, 0.0f);
            m_anchor_max = glm::vec2(0.0f, 0.0f);
            break;
        case AnchorPreset::TopCenter:
            m_anchor_min = glm::vec2(0.5f, 0.0f);
            m_anchor_max = glm::vec2(0.5f, 0.0f);
            break;
        case AnchorPreset::TopRight:
            m_anchor_min = glm::vec2(1.0f, 0.0f);
            m_anchor_max = glm::vec2(1.0f, 0.0f);
            break;
        case AnchorPreset::CenterLeft:
            m_anchor_min = glm::vec2(0.0f, 0.5f);
            m_anchor_max = glm::vec2(0.0f, 0.5f);
            break;
        case AnchorPreset::Center:
            m_anchor_min = glm::vec2(0.5f, 0.5f);
            m_anchor_max = glm::vec2(0.5f, 0.5f);
            break;
        case AnchorPreset::CenterRight:
            m_anchor_min = glm::vec2(1.0f, 0.5f);
            m_anchor_max = glm::vec2(1.0f, 0.5f);
            break;
        case AnchorPreset::BottomLeft:
            m_anchor_min = glm::vec2(0.0f, 1.0f);
            m_anchor_max = glm::vec2(0.0f, 1.0f);
            break;
        case AnchorPreset::BottomCenter:
            m_anchor_min = glm::vec2(0.5f, 1.0f);
            m_anchor_max = glm::vec2(0.5f, 1.0f);
            break;
        case AnchorPreset::BottomRight:
            m_anchor_min = glm::vec2(1.0f, 1.0f);
            m_anchor_max = glm::vec2(1.0f, 1.0f);
            break;
        case AnchorPreset::LeftWide:
            m_anchor_min = glm::vec2(0.0f, 0.0f);
            m_anchor_max = glm::vec2(0.0f, 1.0f);
            break;
        case AnchorPreset::TopWide:
            m_anchor_min = glm::vec2(0.0f, 0.0f);
            m_anchor_max = glm::vec2(1.0f, 0.0f);
            break;
        case AnchorPreset::RightWide:
            m_anchor_min = glm::vec2(1.0f, 0.0f);
            m_anchor_max = glm::vec2(1.0f, 1.0f);
            break;
        case AnchorPreset::BottomWide:
            m_anchor_min = glm::vec2(0.0f, 1.0f);
            m_anchor_max = glm::vec2(1.0f, 1.0f);
            break;
        case AnchorPreset::VCenterWide:
            m_anchor_min = glm::vec2(0.0f, 0.5f);
            m_anchor_max = glm::vec2(1.0f, 0.5f);
            break;
        case AnchorPreset::HCenterWide:
            m_anchor_min = glm::vec2(0.5f, 0.0f);
            m_anchor_max = glm::vec2(0.5f, 1.0f);
            break;
        case AnchorPreset::FullRect:
            m_anchor_min = glm::vec2(0.0f, 0.0f);
            m_anchor_max = glm::vec2(1.0f, 1.0f);
            break;
    }
}

void Control::SetMargins(float left, float top, float right, float bottom) {
    m_margin_left = left;
    m_margin_top = top;
    m_margin_right = right;
    m_margin_bottom = bottom;
}

glm::vec2 Control::GetParentSize() const {
    if (!m_parent) {
        // TODO: Get actual screen size from engine/renderer
        return glm::vec2(1920.0f, 1080.0f); // Default screen size
    }
    
    // Check if parent is also a Control
    if (auto* parent_control = dynamic_cast<const Control*>(m_parent)) {
        return parent_control->GetSize();
    }
    
    // If parent is not Control, use default screen size
    return glm::vec2(1920.0f, 1080.0f);
}

glm::vec4 Control::GetGlobalRect() const {
    glm::vec2 parent_size = GetParentSize();
    
    // Calculate anchor positions
    glm::vec2 anchor_min_pos = m_anchor_min * parent_size;
    glm::vec2 anchor_max_pos = m_anchor_max * parent_size;
    
    // Calculate final position and size
    glm::vec2 final_pos = anchor_min_pos + glm::vec2(m_margin_left, m_margin_top) + m_position;
    glm::vec2 final_size = m_size;
    
    // If anchors are different, use margin-based sizing
    if (m_anchor_min != m_anchor_max) {
        final_size = anchor_max_pos - anchor_min_pos - glm::vec2(m_margin_left + m_margin_right, m_margin_top + m_margin_bottom);
    }
    
    return glm::vec4(final_pos.x, final_pos.y, final_size.x, final_size.y);
}

bool Control::ContainsPoint(const glm::vec2& point) const {
    glm::vec4 rect = GetGlobalRect();
    return point.x >= rect.x && point.x <= rect.x + rect.z &&
           point.y >= rect.y && point.y <= rect.y + rect.w;
}

void Control::CopyTypeSpecificProperties(Node* target) const {
    // Call base class implementation first
    Node::CopyTypeSpecificProperties(target);

    // Copy Control-specific properties
    if (auto* target_control = dynamic_cast<Control*>(target)) {
        target_control->SetPosition(m_position);
        target_control->SetSize(m_size);
        target_control->SetAnchorMin(m_anchor_min);
        target_control->SetAnchorMax(m_anchor_max);
        target_control->SetMarginLeft(m_margin_left);
        target_control->SetMarginTop(m_margin_top);
        target_control->SetMarginRight(m_margin_right);
        target_control->SetMarginBottom(m_margin_bottom);
        target_control->SetWorldSpace(m_world_space);
    }
}

} // namespace Lupine
