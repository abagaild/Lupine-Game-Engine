#include "lupine/components/ProgressBar.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Control.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/rendering/Renderer.h"
#include <iostream>
#include <algorithm>

namespace Lupine {

ProgressBar::ProgressBar()
    : Component("ProgressBar")
    , m_value(0.0f)
    , m_min_value(0.0f)
    , m_max_value(100.0f)
    , m_fill_direction(FillDirection::LeftToRight)
    , m_background_color(0.15f, 0.17f, 0.20f, 1.0f) // Dark background like Godot
    , m_fill_color(0.26f, 0.59f, 0.98f, 1.0f) // Godot's blue accent color
    , m_border_color(0.10f, 0.12f, 0.15f, 1.0f) // Darker border
    , m_border_width(1.0f)
    , m_corner_radius(4.0f) // More rounded like Godot
    , m_show_percentage(true)
    , m_custom_text("")
    , m_text_color(0.875f, 0.875f, 0.875f, 1.0f) // Light gray text like Godot
    , m_font_size(12)
    , m_font_path(FontPath("Arial", true, "Regular"))
    , m_font_handle(nullptr)
    , m_font_loaded(false) {

    // Initialize export variables
    InitializeExportVariables();
}

void ProgressBar::SetValue(float value) {
    m_value = ClampValue(value);
    SetExportVariable("value", m_value);
}

void ProgressBar::SetMinValue(float min_value) {
    m_min_value = min_value;
    if (m_max_value < m_min_value) {
        m_max_value = m_min_value;
    }
    m_value = ClampValue(m_value);
    SetExportVariable("min_value", m_min_value);
    SetExportVariable("max_value", m_max_value);
    SetExportVariable("value", m_value);
}

void ProgressBar::SetMaxValue(float max_value) {
    m_max_value = max_value;
    if (m_min_value > m_max_value) {
        m_min_value = m_max_value;
    }
    m_value = ClampValue(m_value);
    SetExportVariable("min_value", m_min_value);
    SetExportVariable("max_value", m_max_value);
    SetExportVariable("value", m_value);
}

float ProgressBar::GetProgress() const {
    if (m_max_value <= m_min_value) {
        return 0.0f;
    }
    return (m_value - m_min_value) / (m_max_value - m_min_value);
}

void ProgressBar::SetProgress(float progress) {
    progress = std::clamp(progress, 0.0f, 1.0f);
    m_value = m_min_value + progress * (m_max_value - m_min_value);
    SetExportVariable("value", m_value);
}

void ProgressBar::SetFillDirection(FillDirection direction) {
    m_fill_direction = direction;
    SetExportVariable("fill_direction", static_cast<int>(direction));
}

void ProgressBar::SetBackgroundColor(const glm::vec4& color) {
    m_background_color = color;
    SetExportVariable("background_color", color);
}

void ProgressBar::SetFillColor(const glm::vec4& color) {
    m_fill_color = color;
    SetExportVariable("fill_color", color);
}

void ProgressBar::SetBorderColor(const glm::vec4& color) {
    m_border_color = color;
    SetExportVariable("border_color", color);
}

void ProgressBar::SetBorderWidth(float width) {
    m_border_width = std::max(0.0f, width);
    SetExportVariable("border_width", m_border_width);
}

void ProgressBar::SetCornerRadius(float radius) {
    m_corner_radius = std::max(0.0f, radius);
    SetExportVariable("corner_radius", m_corner_radius);
}

void ProgressBar::SetShowPercentage(bool show) {
    m_show_percentage = show;
    SetExportVariable("show_percentage", show);
}

void ProgressBar::SetCustomText(const std::string& text) {
    m_custom_text = text;
    SetExportVariable("custom_text", text);
}

void ProgressBar::SetTextColor(const glm::vec4& color) {
    m_text_color = color;
    SetExportVariable("text_color", color);
}

void ProgressBar::SetFontSize(int size) {
    if (m_font_size != std::max(1, size)) {
        m_font_size = std::max(1, size);
        SetExportVariable("font_size", m_font_size);
        m_font_loaded = false; // Force reload
        LoadFont();
    }
}

void ProgressBar::SetFontPath(const FontPath& path) {
    if (m_font_path != path) {
        m_font_path = path;
        SetExportVariable("font_path", path);
        m_font_loaded = false; // Force reload
        LoadFont();
    }
}

void ProgressBar::OnReady() {
    UpdateFromExportVariables();
    LoadFont();
}

void ProgressBar::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if export variables have changed
    UpdateFromExportVariables();

    // Check rendering context - ProgressBar should only render in 2D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor3D) {
        return; // Don't render 2D progress bars in 3D editor view
    }

    // Load default font if no font is set
    if (m_font_path.path.empty()) {
        // Set default system font
        #ifdef _WIN32
        m_font_path = FontPath("Arial", true, "Regular");
        #elif defined(__APPLE__)
        m_font_path = FontPath("Helvetica", true, "Regular");
        #else
        m_font_path = FontPath("DejaVu Sans", true, "Regular");
        #endif
        LoadFont();
    }

    // Render the progress bar
    if (auto* control = dynamic_cast<Control*>(GetOwner())) {
        RenderBackground(control);
        RenderFill(control);
        if ((m_show_percentage || !m_custom_text.empty()) && m_font_loaded && m_font_handle) {
            RenderText(control);
        }
    } else if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
        RenderBackgroundNode2D(node2d);
        RenderFillNode2D(node2d);
        if ((m_show_percentage || !m_custom_text.empty()) && m_font_loaded && m_font_handle) {
            RenderTextNode2D(node2d);
        }
    }
}

void ProgressBar::InitializeExportVariables() {
    AddExportVariable("value", m_value, "Current progress value", ExportVariableType::Float);
    AddExportVariable("min_value", m_min_value, "Minimum progress value", ExportVariableType::Float);
    AddExportVariable("max_value", m_max_value, "Maximum progress value", ExportVariableType::Float);
    AddExportVariable("fill_direction", static_cast<int>(m_fill_direction), "Fill direction (0=L2R, 1=R2L, 2=T2B, 3=B2T)", ExportVariableType::Int);
    AddExportVariable("background_color", m_background_color, "Background color (RGBA)", ExportVariableType::Color);
    AddExportVariable("fill_color", m_fill_color, "Fill color (RGBA)", ExportVariableType::Color);
    AddExportVariable("border_color", m_border_color, "Border color (RGBA)", ExportVariableType::Color);
    AddExportVariable("border_width", m_border_width, "Border width in pixels", ExportVariableType::Float);
    AddExportVariable("corner_radius", m_corner_radius, "Corner radius in pixels", ExportVariableType::Float);
    AddExportVariable("show_percentage", m_show_percentage, "Show percentage text", ExportVariableType::Bool);
    AddExportVariable("custom_text", m_custom_text, "Custom text to display (overrides percentage)", ExportVariableType::String);
    AddExportVariable("text_color", m_text_color, "Text color (RGBA)", ExportVariableType::Color);
    AddExportVariable("font_size", m_font_size, "Font size in pixels", ExportVariableType::Int);
    AddExportVariable("font_path", m_font_path, "Font selection (system fonts + file browser)", ExportVariableType::FontPath);
}

void ProgressBar::UpdateExportVariables() {
    SetExportVariable("value", m_value);
    SetExportVariable("min_value", m_min_value);
    SetExportVariable("max_value", m_max_value);
    SetExportVariable("fill_direction", static_cast<int>(m_fill_direction));
    SetExportVariable("background_color", m_background_color);
    SetExportVariable("fill_color", m_fill_color);
    SetExportVariable("border_color", m_border_color);
    SetExportVariable("border_width", m_border_width);
    SetExportVariable("corner_radius", m_corner_radius);
    SetExportVariable("show_percentage", m_show_percentage);
    SetExportVariable("custom_text", m_custom_text);
    SetExportVariable("text_color", m_text_color);
    SetExportVariable("font_size", m_font_size);
    SetExportVariable("font_path", m_font_path);
}

void ProgressBar::UpdateFromExportVariables() {
    float new_value = GetExportVariableValue<float>("value", m_value);
    float new_min = GetExportVariableValue<float>("min_value", m_min_value);
    float new_max = GetExportVariableValue<float>("max_value", m_max_value);
    
    m_min_value = new_min;
    m_max_value = new_max;
    m_value = ClampValue(new_value);
    
    m_fill_direction = static_cast<FillDirection>(GetExportVariableValue<int>("fill_direction", static_cast<int>(m_fill_direction)));
    m_background_color = GetExportVariableValue<glm::vec4>("background_color", m_background_color);
    m_fill_color = GetExportVariableValue<glm::vec4>("fill_color", m_fill_color);
    m_border_color = GetExportVariableValue<glm::vec4>("border_color", m_border_color);
    m_border_width = GetExportVariableValue<float>("border_width", m_border_width);
    m_corner_radius = GetExportVariableValue<float>("corner_radius", m_corner_radius);
    m_show_percentage = GetExportVariableValue<bool>("show_percentage", m_show_percentage);
    m_custom_text = GetExportVariableValue<std::string>("custom_text", m_custom_text);
    m_text_color = GetExportVariableValue<glm::vec4>("text_color", m_text_color);

    FontPath new_font_path = GetExportVariableValue<FontPath>("font_path", m_font_path);
    int new_font_size = GetExportVariableValue<int>("font_size", m_font_size);

    if (new_font_path != m_font_path || new_font_size != m_font_size) {
        m_font_path = new_font_path;
        m_font_size = new_font_size;
        m_font_loaded = false;
        LoadFont();
    }
}

float ProgressBar::ClampValue(float value) const {
    return std::clamp(value, m_min_value, m_max_value);
}

void ProgressBar::LoadFont() {
    if (m_font_size <= 0) {
        m_font_loaded = false;
        m_font_handle = nullptr;
        return;
    }

    // Initialize ResourceManager if not already done
    if (!ResourceManager::IsInitialized()) {
        ResourceManager::Initialize();
    }

    // Load font using ResourceManager with FontPath support
    Font font = ResourceManager::LoadFont(m_font_path, m_font_size);
    if (font.IsValid()) {
        m_font_handle = font.font_data;
        m_font_loaded = true;
        std::cout << "ProgressBar: Loaded font " << m_font_path.GetDisplayName() << " (size: " << m_font_size << ")" << std::endl;
    } else {
        m_font_handle = nullptr;
        m_font_loaded = false;
        std::cerr << "ProgressBar: Failed to load font " << m_font_path.GetDisplayName() << std::endl;

        // Try fallback to default system font
        FontPath fallback_font("Arial", true, "Regular");
        font = ResourceManager::LoadFont(fallback_font, m_font_size);
        if (font.IsValid()) {
            m_font_handle = font.font_data;
            m_font_loaded = true;
            std::cout << "ProgressBar: Loaded fallback font Arial (size: " << m_font_size << ")" << std::endl;
        }
    }
}

void ProgressBar::RenderBackground(Control* control) {
    if (!control) return;

    glm::vec2 position = control->GetPosition();
    glm::vec2 size = control->GetSize();

    // Render subtle shadow/inset effect
    glm::mat4 shadow_transform = glm::mat4(1.0f);
    shadow_transform = glm::translate(shadow_transform, glm::vec3(position.x + 1.0f, position.y + 1.0f, 0.0f));
    shadow_transform = glm::scale(shadow_transform, glm::vec3(size.x - 2.0f, size.y - 2.0f, 1.0f));

    glm::vec4 shadow_color = glm::vec4(m_background_color.r * 0.7f, m_background_color.g * 0.7f, m_background_color.b * 0.7f, m_background_color.a);
    Renderer::RenderQuad(shadow_transform, shadow_color);

    // Create transform matrix for background quad
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(position.x, position.y, 0.0f));
    transform = glm::scale(transform, glm::vec3(size.x, size.y, 1.0f));

    // Render background quad with slightly lighter color for depth
    glm::vec4 bg_color = glm::vec4(m_background_color.r * 1.05f, m_background_color.g * 1.05f, m_background_color.b * 1.05f, m_background_color.a);
    Renderer::RenderQuad(transform, bg_color);

    // Render border if enabled
    if (m_border_width > 0.0f) {
        RenderBorder(position, size);
    }
}

void ProgressBar::RenderBackgroundNode2D(Node2D* node2d) {
    if (!node2d) return;

    glm::vec2 position = node2d->GetGlobalPosition();
    glm::vec2 scale = node2d->GetScale();

    // For Node2D, use a default size that can be scaled
    glm::vec2 default_size(100.0f, 20.0f);
    glm::vec2 size = default_size * scale;

    // Create transform matrix for background quad
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(position.x, position.y, 0.0f));
    transform = glm::scale(transform, glm::vec3(size.x, size.y, 1.0f));

    // Render background quad
    Renderer::RenderQuad(transform, m_background_color);

    // Render border if enabled
    if (m_border_width > 0.0f) {
        RenderBorder(position, size);
    }
}

void ProgressBar::RenderFill(Control* control) {
    if (!control) return;

    glm::vec2 position = control->GetPosition();
    glm::vec2 size = control->GetSize();
    glm::vec4 fill_rect = CalculateFillRect(position, size);

    if (fill_rect.z > 0.0f && fill_rect.w > 0.0f) {
        // Render gradient fill effect
        // Top part (lighter)
        glm::mat4 top_transform = glm::mat4(1.0f);
        top_transform = glm::translate(top_transform, glm::vec3(fill_rect.x, fill_rect.y, 0.0f));
        top_transform = glm::scale(top_transform, glm::vec3(fill_rect.z, fill_rect.w * 0.5f, 1.0f));

        glm::vec4 top_fill_color = glm::vec4(m_fill_color.r * 1.2f, m_fill_color.g * 1.2f, m_fill_color.b * 1.2f, m_fill_color.a);
        // Clamp to prevent oversaturation
        top_fill_color.r = std::min(top_fill_color.r, 1.0f);
        top_fill_color.g = std::min(top_fill_color.g, 1.0f);
        top_fill_color.b = std::min(top_fill_color.b, 1.0f);
        Renderer::RenderQuad(top_transform, top_fill_color);

        // Bottom part (darker)
        glm::mat4 bottom_transform = glm::mat4(1.0f);
        bottom_transform = glm::translate(bottom_transform, glm::vec3(fill_rect.x, fill_rect.y + fill_rect.w * 0.5f, 0.0f));
        bottom_transform = glm::scale(bottom_transform, glm::vec3(fill_rect.z, fill_rect.w * 0.5f, 1.0f));

        glm::vec4 bottom_fill_color = glm::vec4(m_fill_color.r * 0.8f, m_fill_color.g * 0.8f, m_fill_color.b * 0.8f, m_fill_color.a);
        Renderer::RenderQuad(bottom_transform, bottom_fill_color);

        // Add subtle highlight at the top
        glm::mat4 highlight_transform = glm::mat4(1.0f);
        highlight_transform = glm::translate(highlight_transform, glm::vec3(fill_rect.x, fill_rect.y, 0.0f));
        highlight_transform = glm::scale(highlight_transform, glm::vec3(fill_rect.z, 1.0f, 1.0f));

        glm::vec4 highlight_color(1.0f, 1.0f, 1.0f, 0.3f); // Semi-transparent white highlight
        Renderer::RenderQuad(highlight_transform, highlight_color);
    }
}

void ProgressBar::RenderFillNode2D(Node2D* node2d) {
    if (!node2d) return;

    glm::vec2 position = node2d->GetGlobalPosition();
    glm::vec2 scale = node2d->GetScale();
    glm::vec2 default_size(100.0f, 20.0f);
    glm::vec2 size = default_size * scale;
    glm::vec4 fill_rect = CalculateFillRect(position, size);

    if (fill_rect.z > 0.0f && fill_rect.w > 0.0f) {
        // Create transform matrix for fill quad
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(fill_rect.x, fill_rect.y, 0.0f));
        transform = glm::scale(transform, glm::vec3(fill_rect.z, fill_rect.w, 1.0f));

        // Render fill quad
        Renderer::RenderQuad(transform, m_fill_color);
    }
}

void ProgressBar::RenderBorder(const glm::vec2& position, const glm::vec2& size) {
    if (m_border_width <= 0.0f) return;

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

void ProgressBar::RenderText(Control* control) {
    if (!control || !m_font_loaded || !m_font_handle) return;

    glm::vec2 position = control->GetPosition();
    glm::vec2 size = control->GetSize();

    // Calculate percentage text
    float percentage = GetProgress() * 100.0f;
    std::string text = std::to_string(static_cast<int>(percentage)) + "%";

    // Get font from ResourceManager to access glyph atlas
    Font font = ResourceManager::GetFont(m_font_path, m_font_size);
    if (!font.IsValid()) {
        return;
    }

    // Generate glyph atlas if needed
    auto glyphs = ResourceManager::GenerateGlyphAtlas(font);
    if (glyphs.empty()) {
        return;
    }

    // Calculate text size for centering
    float text_width = 0.0f;
    for (char c : text) {
        auto glyph_it = glyphs.find(c);
        if (glyph_it != glyphs.end()) {
            text_width += glyph_it->second.advance >> 6;
        }
    }

    // Center text in progress bar
    float text_x = position.x + (size.x - text_width) * 0.5f;
    float text_y = position.y + (size.y - m_font_size) * 0.5f;

    // Render each character
    float current_x = text_x;
    for (char c : text) {
        auto glyph_it = glyphs.find(c);
        if (glyph_it == glyphs.end()) {
            continue;
        }

        const Glyph& glyph = glyph_it->second;

        // Calculate glyph position using proper baseline alignment with DPI scaling
        float glyph_scale_factor = 1.0f / font.dpi_scale;
        float glyph_x = current_x + (glyph.bearing.x * glyph_scale_factor);
        // Position glyph relative to a consistent baseline
        // bearing.y (maxy) is the distance from baseline to top of glyph (positive upward)
        // baseline_to_bottom (miny) is the distance from baseline to bottom of glyph (negative for descenders)
        // We use text_y as the baseline reference point
        // Position the texture's bottom-left corner at baseline + miny
        // For descenders, add a small adjustment to better align the main body
        float descender_adjustment = 0.0f;
        if (glyph.baseline_to_bottom < -2) { // Significant descender
            descender_adjustment = -glyph.baseline_to_bottom * 0.25f * glyph_scale_factor; // 25% of descender depth
        }
        float glyph_y = text_y + (glyph.baseline_to_bottom * glyph_scale_factor) + descender_adjustment;

        // Create transform matrix for this glyph with proper scaling
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(glyph_x, glyph_y, 0.0f));
        transform = glm::scale(transform, glm::vec3(glyph.size.x * glyph_scale_factor, glyph.size.y * glyph_scale_factor, 1.0f));

        // Render glyph quad with high-quality text shader
        // TODO: Convert glyph.texture_id to GraphicsTexture - for now use white texture
        Renderer::RenderTextGlyph(transform, m_text_color, Renderer::GetWhiteTexture());

        // Advance to next character position with proper scaling
        current_x += (glyph.advance >> 6) * glyph_scale_factor;
    }
}

void ProgressBar::RenderTextNode2D(Node2D* node2d) {
    if (!node2d || !m_font_loaded || !m_font_handle) return;

    glm::vec2 position = node2d->GetGlobalPosition();
    glm::vec2 scale = node2d->GetScale();
    glm::vec2 default_size(100.0f, 20.0f);
    glm::vec2 size = default_size * scale;

    // Calculate percentage text
    float percentage = GetProgress() * 100.0f;
    std::string text = std::to_string(static_cast<int>(percentage)) + "%";

    // Get font from ResourceManager to access glyph atlas
    Font font = ResourceManager::GetFont(m_font_path, m_font_size);
    if (!font.IsValid()) {
        return;
    }

    // Generate glyph atlas if needed
    auto glyphs = ResourceManager::GenerateGlyphAtlas(font);
    if (glyphs.empty()) {
        return;
    }

    // Calculate text size for centering
    float text_width = 0.0f;
    for (char c : text) {
        auto glyph_it = glyphs.find(c);
        if (glyph_it != glyphs.end()) {
            text_width += glyph_it->second.advance >> 6;
        }
    }

    // Center text in progress bar
    float text_x = position.x + (size.x - text_width * scale.x) * 0.5f;
    float text_y = position.y + (size.y - m_font_size * scale.y) * 0.5f;

    // Render each character
    float current_x = text_x;
    for (char c : text) {
        auto glyph_it = glyphs.find(c);
        if (glyph_it == glyphs.end()) {
            continue;
        }

        const Glyph& glyph = glyph_it->second;

        // Calculate glyph position using proper baseline alignment with DPI scaling
        float glyph_scale_factor = 1.0f / font.dpi_scale;
        float glyph_x = current_x + (glyph.bearing.x * glyph_scale_factor * scale.x);
        // Position glyph relative to a consistent baseline
        // bearing.y (maxy) is the distance from baseline to top of glyph (positive upward)
        // baseline_to_bottom (miny) is the distance from baseline to bottom of glyph (negative for descenders)
        // We use text_y as the baseline reference point
        // Position the texture's bottom-left corner at baseline + miny
        // For descenders, add a small adjustment to better align the main body
        float descender_adjustment = 0.0f;
        if (glyph.baseline_to_bottom < -2) { // Significant descender
            descender_adjustment = -glyph.baseline_to_bottom * 0.25f * glyph_scale_factor * scale.y; // 25% of descender depth
        }
        float glyph_y = text_y + (glyph.baseline_to_bottom * glyph_scale_factor * scale.y) + descender_adjustment;

        // Create transform matrix for this glyph with proper scaling
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(glyph_x, glyph_y, 0.0f));
        transform = glm::scale(transform, glm::vec3(glyph.size.x * glyph_scale_factor * scale.x, glyph.size.y * glyph_scale_factor * scale.y, 1.0f));

        // Render glyph quad with high-quality text shader
        // TODO: Convert glyph.texture_id to GraphicsTexture - for now use white texture
        Renderer::RenderTextGlyph(transform, m_text_color, Renderer::GetWhiteTexture());

        // Advance to next character position with proper scaling
        current_x += (glyph.advance >> 6) * glyph_scale_factor * scale.x;
    }
}

glm::vec4 ProgressBar::CalculateFillRect(const glm::vec2& container_pos, const glm::vec2& container_size) const {
    float progress = GetProgress();
    if (progress <= 0.0f) {
        return glm::vec4(0.0f); // No fill
    }

    glm::vec2 fill_pos = container_pos;
    glm::vec2 fill_size = container_size;

    switch (m_fill_direction) {
        case FillDirection::LeftToRight:
            fill_size.x *= progress;
            break;
        case FillDirection::RightToLeft:
            fill_size.x *= progress;
            fill_pos.x += container_size.x - fill_size.x;
            break;
        case FillDirection::TopToBottom:
            fill_size.y *= progress;
            break;
        case FillDirection::BottomToTop:
            fill_size.y *= progress;
            fill_pos.y += container_size.y - fill_size.y;
            break;
    }

    return glm::vec4(fill_pos.x, fill_pos.y, fill_size.x, fill_size.y);
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(ProgressBar, "UI", "Progress bar component for displaying progress")
