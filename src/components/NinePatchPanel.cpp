#include "lupine/components/NinePatchPanel.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Control.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/rendering/Renderer.h"
#include <iostream>

namespace Lupine {

NinePatchPanel::NinePatchPanel()
    : Component("NinePatchPanel")
    , m_texture_path("")
    , m_modulate_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_left_border(8.0f)
    , m_right_border(8.0f)
    , m_top_border(8.0f)
    , m_bottom_border(8.0f)
    , m_texture_id(0)
    , m_texture_loaded(false)
    , m_texture_size(0.0f, 0.0f) {

    // Initialize export variables
    InitializeExportVariables();
}

void NinePatchPanel::SetTexturePath(const std::string& path) {
    if (m_texture_path != path) {
        m_texture_path = path;
        SetExportVariable("texture_path", path);
        m_texture_loaded = false;
        LoadTexture();
    }
}

void NinePatchPanel::SetModulateColor(const glm::vec4& color) {
    m_modulate_color = color;
    SetExportVariable("modulate_color", color);
}

void NinePatchPanel::SetLeftBorder(float size) {
    m_left_border = std::max(0.0f, size);
    SetExportVariable("left_border", m_left_border);
}

void NinePatchPanel::SetRightBorder(float size) {
    m_right_border = std::max(0.0f, size);
    SetExportVariable("right_border", m_right_border);
}

void NinePatchPanel::SetTopBorder(float size) {
    m_top_border = std::max(0.0f, size);
    SetExportVariable("top_border", m_top_border);
}

void NinePatchPanel::SetBottomBorder(float size) {
    m_bottom_border = std::max(0.0f, size);
    SetExportVariable("bottom_border", m_bottom_border);
}

void NinePatchPanel::SetBorders(float left, float top, float right, float bottom) {
    SetLeftBorder(left);
    SetTopBorder(top);
    SetRightBorder(right);
    SetBottomBorder(bottom);
}

void NinePatchPanel::OnReady() {
    UpdateFromExportVariables();
    LoadTexture();
}

void NinePatchPanel::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if export variables have changed
    UpdateFromExportVariables();

    // Check rendering context - NinePatchPanel should only render in 2D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor3D) {
        return; // Don't render 2D panels in 3D editor view
    }

    // Render the nine-patch panel if texture is loaded
    if (m_texture_loaded && m_texture_id != 0) {
        if (auto* control = dynamic_cast<Control*>(GetOwner())) {
            RenderNinePatch(control);
        } else if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
            RenderNinePatchNode2D(node2d);
        }
    }
}

void NinePatchPanel::InitializeExportVariables() {
    AddExportVariable("texture_path", m_texture_path, "Path to nine-patch texture file", ExportVariableType::FilePath);
    AddExportVariable("modulate_color", m_modulate_color, "Color modulation (RGBA)", ExportVariableType::Color);
    AddExportVariable("left_border", m_left_border, "Left border size in pixels", ExportVariableType::Float);
    AddExportVariable("right_border", m_right_border, "Right border size in pixels", ExportVariableType::Float);
    AddExportVariable("top_border", m_top_border, "Top border size in pixels", ExportVariableType::Float);
    AddExportVariable("bottom_border", m_bottom_border, "Bottom border size in pixels", ExportVariableType::Float);
}

void NinePatchPanel::UpdateExportVariables() {
    SetExportVariable("texture_path", m_texture_path);
    SetExportVariable("modulate_color", m_modulate_color);
    SetExportVariable("left_border", m_left_border);
    SetExportVariable("right_border", m_right_border);
    SetExportVariable("top_border", m_top_border);
    SetExportVariable("bottom_border", m_bottom_border);
}

void NinePatchPanel::UpdateFromExportVariables() {
    std::string new_texture_path = GetExportVariableValue<std::string>("texture_path", m_texture_path);
    if (new_texture_path != m_texture_path) {
        SetTexturePath(new_texture_path);
    }
    
    m_modulate_color = GetExportVariableValue<glm::vec4>("modulate_color", m_modulate_color);
    m_left_border = GetExportVariableValue<float>("left_border", m_left_border);
    m_right_border = GetExportVariableValue<float>("right_border", m_right_border);
    m_top_border = GetExportVariableValue<float>("top_border", m_top_border);
    m_bottom_border = GetExportVariableValue<float>("bottom_border", m_bottom_border);
}

void NinePatchPanel::LoadTexture() {
    if (m_texture_path.empty()) {
        m_texture_loaded = false;
        m_texture_id = 0;
        return;
    }

    // Initialize ResourceManager if not already done
    if (!ResourceManager::IsInitialized()) {
        ResourceManager::Initialize();
    }

    // Load texture using ResourceManager
    Texture texture = ResourceManager::LoadTexture(m_texture_path);
    if (texture.IsValid()) {
        m_texture_id = texture.id;
        m_texture_size = glm::vec2(texture.width, texture.height);
        m_texture_loaded = true;
        std::cout << "NinePatchPanel: Loaded texture " << m_texture_path
                  << " (" << texture.width << "x" << texture.height << ")" << std::endl;
    } else {
        m_texture_id = 0;
        m_texture_loaded = false;
        std::cerr << "NinePatchPanel: Failed to load texture " << m_texture_path << std::endl;
    }
}

void NinePatchPanel::RenderNinePatch(Control* control) {
    if (!control || !m_texture_loaded || m_texture_id == 0) return;

    glm::vec2 position = control->GetPosition();
    glm::vec2 size = control->GetSize();

    // Calculate UV coordinates for the nine patches
    float left_uv = m_left_border / m_texture_size.x;
    float right_uv = (m_texture_size.x - m_right_border) / m_texture_size.x;
    float top_uv = m_top_border / m_texture_size.y;
    float bottom_uv = (m_texture_size.y - m_bottom_border) / m_texture_size.y;

    // Calculate patch positions and sizes
    float center_width = size.x - m_left_border - m_right_border;
    float center_height = size.y - m_top_border - m_bottom_border;

    // Render the nine patches
    // Top-left corner
    RenderPatch(position, glm::vec2(m_left_border, m_top_border), 
                glm::vec2(0.0f, 0.0f), glm::vec2(left_uv, top_uv));

    // Top edge
    RenderPatch(glm::vec2(position.x + m_left_border, position.y), 
                glm::vec2(center_width, m_top_border),
                glm::vec2(left_uv, 0.0f), glm::vec2(right_uv, top_uv));

    // Top-right corner
    RenderPatch(glm::vec2(position.x + size.x - m_right_border, position.y), 
                glm::vec2(m_right_border, m_top_border),
                glm::vec2(right_uv, 0.0f), glm::vec2(1.0f, top_uv));

    // Left edge
    RenderPatch(glm::vec2(position.x, position.y + m_top_border), 
                glm::vec2(m_left_border, center_height),
                glm::vec2(0.0f, top_uv), glm::vec2(left_uv, bottom_uv));

    // Center
    RenderPatch(glm::vec2(position.x + m_left_border, position.y + m_top_border), 
                glm::vec2(center_width, center_height),
                glm::vec2(left_uv, top_uv), glm::vec2(right_uv, bottom_uv));

    // Right edge
    RenderPatch(glm::vec2(position.x + size.x - m_right_border, position.y + m_top_border), 
                glm::vec2(m_right_border, center_height),
                glm::vec2(right_uv, top_uv), glm::vec2(1.0f, bottom_uv));

    // Bottom-left corner
    RenderPatch(glm::vec2(position.x, position.y + size.y - m_bottom_border), 
                glm::vec2(m_left_border, m_bottom_border),
                glm::vec2(0.0f, bottom_uv), glm::vec2(left_uv, 1.0f));

    // Bottom edge
    RenderPatch(glm::vec2(position.x + m_left_border, position.y + size.y - m_bottom_border), 
                glm::vec2(center_width, m_bottom_border),
                glm::vec2(left_uv, bottom_uv), glm::vec2(right_uv, 1.0f));

    // Bottom-right corner
    RenderPatch(glm::vec2(position.x + size.x - m_right_border, position.y + size.y - m_bottom_border), 
                glm::vec2(m_right_border, m_bottom_border),
                glm::vec2(right_uv, bottom_uv), glm::vec2(1.0f, 1.0f));
}

void NinePatchPanel::RenderNinePatchNode2D(Node2D* node2d) {
    if (!node2d || !m_texture_loaded || m_texture_id == 0) return;

    glm::vec2 position = node2d->GetGlobalPosition();
    glm::vec2 scale = node2d->GetScale();
    
    // For Node2D, use a default size that can be scaled
    glm::vec2 default_size(100.0f, 100.0f);
    glm::vec2 size = default_size * scale;

    // Use the same rendering logic as Control, but with Node2D position and size
    // This is a simplified version - in a real implementation, you might want to
    // create a helper function to avoid code duplication
    
    // For now, render as a simple textured quad
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(position.x, position.y, 0.0f));
    transform = glm::scale(transform, glm::vec3(size.x, size.y, 1.0f));

    // Render as a simple textured quad (not true nine-patch for Node2D)
    Renderer::RenderQuad(transform, m_modulate_color, m_texture_id);
}

void NinePatchPanel::RenderPatch(const glm::vec2& position, const glm::vec2& size, 
                                 const glm::vec2& uv_min, const glm::vec2& uv_max) {
    if (size.x <= 0.0f || size.y <= 0.0f) return;

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(position.x, position.y, 0.0f));
    transform = glm::scale(transform, glm::vec3(size.x, size.y, 1.0f));

    // Create texture region from UV coordinates
    glm::vec4 texture_region(uv_min.x, uv_min.y, uv_max.x - uv_min.x, uv_max.y - uv_min.y);

    // Render patch with specific UV region
    Renderer::RenderQuad(transform, m_modulate_color, m_texture_id, texture_region);
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(NinePatchPanel, "UI", "Nine-patch scalable panel component")
