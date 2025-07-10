#include "lupine/components/TextureRectangle.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Control.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/rendering/Renderer.h"
#include <iostream>

namespace Lupine {

TextureRectangle::TextureRectangle()
    : Component("TextureRectangle")
    , m_texture_path("")
    , m_modulate_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_stretch_mode(StretchMode::Stretch)
    , m_flip_h(false)
    , m_flip_v(false)
    , m_texture_region(0.0f, 0.0f, 1.0f, 1.0f)
    , m_texture_id(0)
    , m_texture_loaded(false)
    , m_texture_size(0.0f, 0.0f) {

    // Initialize export variables
    InitializeExportVariables();
}

void TextureRectangle::SetTexturePath(const std::string& path) {
    if (m_texture_path != path) {
        m_texture_path = path;
        SetExportVariable("texture_path", path);
        m_texture_loaded = false;
        LoadTexture();
    }
}

void TextureRectangle::SetModulateColor(const glm::vec4& color) {
    m_modulate_color = color;
    SetExportVariable("modulate_color", color);
}

void TextureRectangle::SetStretchMode(StretchMode mode) {
    m_stretch_mode = mode;
    SetExportVariable("stretch_mode", StretchModeToInt(mode));
}

void TextureRectangle::SetFlippedH(bool flip) {
    m_flip_h = flip;
    SetExportVariable("flip_h", flip);
}

void TextureRectangle::SetFlippedV(bool flip) {
    m_flip_v = flip;
    SetExportVariable("flip_v", flip);
}

void TextureRectangle::SetTextureRegion(const glm::vec4& region) {
    m_texture_region = region;
    SetExportVariable("texture_region", region);
}

void TextureRectangle::OnReady() {
    UpdateFromExportVariables();
    LoadTexture();
}

void TextureRectangle::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if export variables have changed
    UpdateFromExportVariables();

    // Check rendering context - TextureRectangle should only render in 2D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor3D) {
        return; // Don't render 2D textures in 3D editor view
    }

    // Render the texture if loaded
    if (m_texture_loaded && m_texture_id != 0) {
        if (auto* control = dynamic_cast<Control*>(GetOwner())) {
            RenderTexture(control);
        } else if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
            RenderTextureNode2D(node2d);
        }
    }
}

void TextureRectangle::InitializeExportVariables() {
    // Create enum options for stretch mode
    std::vector<std::string> stretchModeOptions = {
        "Stretch", "Tile", "KeepAspect", "KeepAspectCentered", "KeepAspectCovered"
    };

    AddExportVariable("texture_path", m_texture_path, "Path to texture file", ExportVariableType::FilePath);
    AddExportVariable("modulate_color", m_modulate_color, "Color modulation (RGBA)", ExportVariableType::Color);
    AddEnumExportVariable("stretch_mode", StretchModeToInt(m_stretch_mode), "Texture stretch mode", stretchModeOptions);
    AddExportVariable("flip_h", m_flip_h, "Flip texture horizontally", ExportVariableType::Bool);
    AddExportVariable("flip_v", m_flip_v, "Flip texture vertically", ExportVariableType::Bool);
    AddExportVariable("texture_region", m_texture_region, "Texture region (x, y, width, height) in normalized coordinates", ExportVariableType::Vec4);
}

void TextureRectangle::UpdateExportVariables() {
    SetExportVariable("texture_path", m_texture_path);
    SetExportVariable("modulate_color", m_modulate_color);
    SetExportVariable("stretch_mode", StretchModeToInt(m_stretch_mode));
    SetExportVariable("flip_h", m_flip_h);
    SetExportVariable("flip_v", m_flip_v);
    SetExportVariable("texture_region", m_texture_region);
}

void TextureRectangle::UpdateFromExportVariables() {
    std::string new_texture_path = GetExportVariableValue<std::string>("texture_path", m_texture_path);
    if (new_texture_path != m_texture_path) {
        SetTexturePath(new_texture_path);
    }
    
    m_modulate_color = GetExportVariableValue<glm::vec4>("modulate_color", m_modulate_color);
    m_stretch_mode = IntToStretchMode(GetExportVariableValue<int>("stretch_mode", StretchModeToInt(m_stretch_mode)));
    m_flip_h = GetExportVariableValue<bool>("flip_h", m_flip_h);
    m_flip_v = GetExportVariableValue<bool>("flip_v", m_flip_v);
    m_texture_region = GetExportVariableValue<glm::vec4>("texture_region", m_texture_region);
}

void TextureRectangle::LoadTexture() {
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
        std::cout << "TextureRectangle: Loaded texture " << m_texture_path
                  << " (" << texture.width << "x" << texture.height << ")" << std::endl;
    } else {
        m_texture_id = 0;
        m_texture_loaded = false;
        std::cerr << "TextureRectangle: Failed to load texture " << m_texture_path << std::endl;
    }
}

void TextureRectangle::RenderTexture(Control* control) {
    if (!control || !m_texture_loaded || m_texture_id == 0) return;

    glm::vec2 position = control->GetPosition();
    glm::vec2 size = control->GetSize();

    // Calculate texture transform based on stretch mode
    glm::mat4 transform = CalculateTextureTransform(position, size);

    // Render texture quad
    Renderer::RenderQuad(transform, m_modulate_color, m_texture_id, m_texture_region, m_flip_h, m_flip_v);
}

void TextureRectangle::RenderTextureNode2D(Node2D* node2d) {
    if (!node2d || !m_texture_loaded || m_texture_id == 0) return;

    glm::vec2 position = node2d->GetGlobalPosition();
    glm::vec2 scale = node2d->GetScale();
    
    // For Node2D, use texture size as default size, scaled by node scale
    glm::vec2 size = m_texture_size * scale;

    // Calculate texture transform based on stretch mode
    glm::mat4 transform = CalculateTextureTransform(position, size);

    // Render texture quad
    Renderer::RenderQuad(transform, m_modulate_color, m_texture_id, m_texture_region, m_flip_h, m_flip_v);
}

glm::mat4 TextureRectangle::CalculateTextureTransform(const glm::vec2& position, const glm::vec2& rect_size) const {
    glm::mat4 transform = glm::mat4(1.0f);
    glm::vec2 render_size = rect_size;
    glm::vec2 render_position = position;

    switch (m_stretch_mode) {
        case StretchMode::Stretch:
            // Use rect size as-is
            break;
            
        case StretchMode::KeepAspect:
        case StretchMode::KeepAspectCentered: {
            if (m_texture_size.x > 0 && m_texture_size.y > 0) {
                float texture_aspect = m_texture_size.x / m_texture_size.y;
                float rect_aspect = rect_size.x / rect_size.y;
                
                if (texture_aspect > rect_aspect) {
                    // Texture is wider, fit to width
                    render_size.x = rect_size.x;
                    render_size.y = rect_size.x / texture_aspect;
                } else {
                    // Texture is taller, fit to height
                    render_size.y = rect_size.y;
                    render_size.x = rect_size.y * texture_aspect;
                }
                
                if (m_stretch_mode == StretchMode::KeepAspectCentered) {
                    render_position.x += (rect_size.x - render_size.x) * 0.5f;
                    render_position.y += (rect_size.y - render_size.y) * 0.5f;
                }
            }
            break;
        }
        
        case StretchMode::KeepAspectCovered: {
            if (m_texture_size.x > 0 && m_texture_size.y > 0) {
                float texture_aspect = m_texture_size.x / m_texture_size.y;
                float rect_aspect = rect_size.x / rect_size.y;
                
                if (texture_aspect > rect_aspect) {
                    // Texture is wider, fit to height
                    render_size.y = rect_size.y;
                    render_size.x = rect_size.y * texture_aspect;
                } else {
                    // Texture is taller, fit to width
                    render_size.x = rect_size.x;
                    render_size.y = rect_size.x / texture_aspect;
                }
                
                // Center the oversized texture
                render_position.x += (rect_size.x - render_size.x) * 0.5f;
                render_position.y += (rect_size.y - render_size.y) * 0.5f;
            }
            break;
        }
        
        case StretchMode::Tile:
            // For tiling, we'll use the rect size but the renderer should handle tiling
            // This would require additional renderer support for texture tiling
            break;
    }

    transform = glm::translate(transform, glm::vec3(render_position.x, render_position.y, 0.0f));
    transform = glm::scale(transform, glm::vec3(render_size.x, render_size.y, 1.0f));

    return transform;
}

int TextureRectangle::StretchModeToInt(StretchMode mode) const {
    return static_cast<int>(mode);
}

TextureRectangle::StretchMode TextureRectangle::IntToStretchMode(int mode) const {
    if (mode >= 0 && mode <= 4) {
        return static_cast<StretchMode>(mode);
    }
    return StretchMode::Stretch;
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(TextureRectangle, "UI", "Texture display component with various stretch modes")
