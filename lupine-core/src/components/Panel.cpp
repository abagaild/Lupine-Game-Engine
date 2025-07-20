#include "lupine/components/Panel.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Control.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/rendering/Renderer.h"
#include <iostream>

namespace Lupine {

Panel::Panel()
    : Component("Panel")
    , m_background_color(0.2f, 0.2f, 0.2f, 1.0f)
    , m_border_color(0.5f, 0.5f, 0.5f, 1.0f)
    , m_border_width(1.0f)
    , m_border_enabled(false)
    , m_corner_radius(0.0f) {

    // Initialize export variables
    InitializeExportVariables();
}

void Panel::SetBackgroundColor(const glm::vec4& color) {
    m_background_color = color;
    SetExportVariable("background_color", color);
}

void Panel::SetBorderColor(const glm::vec4& color) {
    m_border_color = color;
    SetExportVariable("border_color", color);
}

void Panel::SetBorderWidth(float width) {
    m_border_width = std::max(0.0f, width);
    SetExportVariable("border_width", m_border_width);
}

void Panel::SetBorderEnabled(bool enabled) {
    m_border_enabled = enabled;
    SetExportVariable("border_enabled", enabled);
}

void Panel::SetCornerRadius(float radius) {
    m_corner_radius = std::max(0.0f, radius);
    SetExportVariable("corner_radius", m_corner_radius);
}

void Panel::OnReady() {
    UpdateFromExportVariables();
}

void Panel::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if export variables have changed
    UpdateFromExportVariables();

    // Check rendering context - Panel should only render in 2D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor3D) {
        return; // Don't render 2D panels in 3D editor view
    }

    // Render the panel
    if (auto* control = dynamic_cast<Control*>(GetOwner())) {
        RenderPanel(control);
    } else if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
        RenderPanelNode2D(node2d);
    }
}

void Panel::InitializeExportVariables() {
    AddExportVariable("background_color", m_background_color, "Panel background color (RGBA)", ExportVariableType::Color);
    AddExportVariable("border_color", m_border_color, "Panel border color (RGBA)", ExportVariableType::Color);
    AddExportVariable("border_width", m_border_width, "Border width in pixels", ExportVariableType::Float);
    AddExportVariable("border_enabled", m_border_enabled, "Enable panel border", ExportVariableType::Bool);
    AddExportVariable("corner_radius", m_corner_radius, "Corner radius for rounded corners", ExportVariableType::Float);
}

void Panel::UpdateExportVariables() {
    SetExportVariable("background_color", m_background_color);
    SetExportVariable("border_color", m_border_color);
    SetExportVariable("border_width", m_border_width);
    SetExportVariable("border_enabled", m_border_enabled);
    SetExportVariable("corner_radius", m_corner_radius);
}

void Panel::UpdateFromExportVariables() {
    m_background_color = GetExportVariableValue<glm::vec4>("background_color", m_background_color);
    m_border_color = GetExportVariableValue<glm::vec4>("border_color", m_border_color);
    m_border_width = GetExportVariableValue<float>("border_width", m_border_width);
    m_border_enabled = GetExportVariableValue<bool>("border_enabled", m_border_enabled);
    m_corner_radius = GetExportVariableValue<float>("corner_radius", m_corner_radius);
}

void Panel::RenderPanel(Control* control) {
    if (!control) return;

    glm::vec2 position = control->GetPosition();
    glm::vec2 size = control->GetSize();

    // Create transform matrix for background quad
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(position.x, position.y, 0.0f));
    transform = glm::scale(transform, glm::vec3(size.x, size.y, 1.0f));

    // Render background quad
    Renderer::RenderQuad(transform, m_background_color);

    // Render border if enabled
    if (m_border_enabled && m_border_width > 0.0f) {
        RenderBorder(position, size);
    }
}

void Panel::RenderPanelNode2D(Node2D* node2d) {
    if (!node2d) return;

    glm::vec2 position = node2d->GetGlobalPosition();
    glm::vec2 scale = node2d->GetScale();
    
    // For Node2D, use a default size that can be scaled
    glm::vec2 default_size(100.0f, 100.0f);
    glm::vec2 size = default_size * scale;

    // Create transform matrix for background quad
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(position.x, position.y, 0.0f));
    transform = glm::scale(transform, glm::vec3(size.x, size.y, 1.0f));

    // Render background quad
    Renderer::RenderQuad(transform, m_background_color);

    // Render border if enabled
    if (m_border_enabled && m_border_width > 0.0f) {
        RenderBorder(position, size);
    }
}

void Panel::RenderBorder(const glm::vec2& position, const glm::vec2& size) {
    // Render border as four separate quads (top, bottom, left, right)
    
    // Top border
    glm::mat4 top_transform = glm::mat4(1.0f);
    top_transform = glm::translate(top_transform, glm::vec3(position.x, position.y, 0.0f));
    top_transform = glm::scale(top_transform, glm::vec3(size.x, m_border_width, 1.0f));
    Renderer::RenderQuad(top_transform, m_border_color);

    // Bottom border
    glm::mat4 bottom_transform = glm::mat4(1.0f);
    bottom_transform = glm::translate(bottom_transform, glm::vec3(position.x, position.y + size.y - m_border_width, 0.0f));
    bottom_transform = glm::scale(bottom_transform, glm::vec3(size.x, m_border_width, 1.0f));
    Renderer::RenderQuad(bottom_transform, m_border_color);

    // Left border
    glm::mat4 left_transform = glm::mat4(1.0f);
    left_transform = glm::translate(left_transform, glm::vec3(position.x, position.y, 0.0f));
    left_transform = glm::scale(left_transform, glm::vec3(m_border_width, size.y, 1.0f));
    Renderer::RenderQuad(left_transform, m_border_color);

    // Right border
    glm::mat4 right_transform = glm::mat4(1.0f);
    right_transform = glm::translate(right_transform, glm::vec3(position.x + size.x - m_border_width, position.y, 0.0f));
    right_transform = glm::scale(right_transform, glm::vec3(m_border_width, size.y, 1.0f));
    Renderer::RenderQuad(right_transform, m_border_color);
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(Panel, "UI", "Basic rectangular background panel component")
