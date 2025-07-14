#include "lupine/components/Label.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Control.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/TextRenderer.h"
#include "lupine/localization/LocalizationManager.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>

namespace Lupine {

Label::Label()
    : Component("Label")
    , m_text("Label")
    , m_font_path(FontPath("Arial", true, "Regular")) // Default to Arial system font
    , m_font_size(16)
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_text_align(TextAlign::Left)
    , m_vertical_align(VerticalAlign::Top)
    , m_word_wrap(false)
    , m_line_spacing(1.0f)
    , m_kerning(0.0f) // Default kerning adjustment
    , m_outline_enabled(false)
    , m_outline_color(0.0f, 0.0f, 0.0f, 1.0f)
    , m_outline_width(1.0f)
    , m_shadow_enabled(false)
    , m_shadow_color(0.0f, 0.0f, 0.0f, 0.5f)
    , m_shadow_offset(2.0f, 2.0f)
    , m_shadow_blur(0.0f)
    , m_use_localization_key(false)
    , m_localization_key("")
    , m_font_handle(nullptr)
    , m_font_loaded(false)
    , m_is_being_destroyed(false) {

    // Initialize export variables
    InitializeExportVariables();
}

void Label::SetText(const std::string& text) {
    m_text = text;
    SetExportVariable("text", text);
}

void Label::SetFontPath(const FontPath& path) {
    m_font_path = path;
    SetExportVariable("font_path", path);
    m_font_loaded = false;
    LoadFont();
}

void Label::SetFontSize(int size) {
    m_font_size = size;
    SetExportVariable("font_size", size);
    m_font_loaded = false;
    LoadFont();
}

void Label::SetColor(const glm::vec4& color) {
    m_color = color;
    SetExportVariable("color", color);
}

void Label::SetTextAlign(TextAlign align) {
    m_text_align = align;
    SetExportVariable("text_align", TextAlignToInt(align));
}

void Label::SetVerticalAlign(VerticalAlign align) {
    m_vertical_align = align;
    SetExportVariable("vertical_align", VerticalAlignToInt(align));
}

void Label::SetWordWrap(bool wrap) {
    m_word_wrap = wrap;
    SetExportVariable("word_wrap", wrap);
}

void Label::SetLineSpacing(float spacing) {
    m_line_spacing = spacing;
    SetExportVariable("line_spacing", spacing);
}

void Label::SetOutlineEnabled(bool enabled) {
    m_outline_enabled = enabled;
    SetExportVariable("outline_enabled", enabled);
}

void Label::SetOutlineColor(const glm::vec4& color) {
    m_outline_color = color;
    SetExportVariable("outline_color", color);
}

void Label::SetOutlineWidth(float width) {
    m_outline_width = width;
    SetExportVariable("outline_width", width);
}

void Label::SetShadowEnabled(bool enabled) {
    m_shadow_enabled = enabled;
    SetExportVariable("shadow_enabled", enabled);
}

void Label::SetShadowColor(const glm::vec4& color) {
    m_shadow_color = color;
    SetExportVariable("shadow_color", color);
}

void Label::SetShadowOffset(const glm::vec2& offset) {
    m_shadow_offset = offset;
    SetExportVariable("shadow_offset", offset);
}

void Label::SetKerning(float kerning) {
    m_kerning = kerning;
    SetExportVariable("kerning", kerning);
}

void Label::SetUseLocalizationKey(bool use_localization) {
    m_use_localization_key = use_localization;
    SetExportVariable("use_localization_key", use_localization);
}

void Label::SetLocalizationKey(const std::string& key) {
    m_localization_key = key;
    SetExportVariable("localization_key", key);
}

std::string Label::GetDisplayText() const {
    if (m_use_localization_key && !m_localization_key.empty()) {
        return LocalizationManager::Instance().GetLocalizedString(m_localization_key, m_text);
    }
    return m_text;
}

float Label::GetKerningBetweenChars(char left, char right) const {
    if (!m_font_loaded || !m_font_handle) {
        return 0.0f;
    }

    // Get font from ResourceManager
    Font font = ResourceManager::GetFont(m_font_path, m_font_size);
    if (!font.IsValid()) {
        return 0.0f;
    }

    TTF_Font* ttf_font = static_cast<TTF_Font*>(font.font_data);
    if (!ttf_font) {
        return 0.0f;
    }

    // Check if kerning is enabled for this font
    if (!TTF_GetFontKerning(ttf_font)) {
        return 0.0f;
    }

    // Get kerning between the two characters using 32-bit glyph function
    int kerning = TTF_GetFontKerningSizeGlyphs32(ttf_font, static_cast<Uint32>(left), static_cast<Uint32>(right));

    // Convert to pixels and apply DPI scaling
    float scale_factor = 1.0f / font.dpi_scale;
    return kerning * scale_factor;
}

glm::vec2 Label::CalculateTextSize() const {
    if (m_text.empty()) {
        return glm::vec2(0.0f, 0.0f);
    }

    // Use TextRenderer for accurate text measurement with line spacing
    TextRenderParams params;
    params.text = m_text;
    params.font_path = m_font_path;
    params.font_size = m_font_size;
    params.line_spacing = m_line_spacing;
    params.kerning = m_kerning;

    return TextRenderer::CalculateTextSize(params);
}

void Label::OnReady() {
    UpdateFromExportVariables();
    LoadFont();
}

void Label::OnUpdate(float delta_time) {
    (void)delta_time;

    // Don't update if being destroyed
    if (m_is_being_destroyed) {
        return;
    }

    // Check if owner is still valid
    Node* owner = GetOwner();
    if (!owner || !owner->IsValidNode()) {
        return;
    }

    // Check if export variables have changed
    UpdateFromExportVariables();

    // Check rendering context - Label should only render in 2D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor3D) {
        return; // Don't render 2D labels in 3D editor view
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
        std::cout << "Label: Setting default font to " << m_font_path.path << std::endl;
        LoadFont();
    }

    // Always try to load font if not loaded yet
    if (!m_font_loaded || !m_font_handle) {
        std::cout << "Label: Font not loaded, attempting to load..." << std::endl;
        LoadFont();
    }

    // Debug output
    if (!m_text.empty()) {
        if (!m_font_loaded) {
            std::cerr << "Label: Font not loaded for text '" << m_text << "'" << std::endl;
        }
        if (!m_font_handle) {
            std::cerr << "Label: Font handle is null for text '" << m_text << "'" << std::endl;
        }
    }

    // Render the text if font is loaded and we have text
    std::string display_text = GetDisplayText();
    if (m_font_loaded && m_font_handle && !display_text.empty()) {
        // Try Control first, then Node2D using safe casting
        if (auto* control = owner->SafeCast<Control>()) {
            std::cout << "Label: Rendering text '" << display_text << "' on Control at (" << control->GetPosition().x << ", " << control->GetPosition().y << ")" << std::endl;
            RenderTextControl(control);
        } else if (auto* node2d = owner->SafeCast<Node2D>()) {
            std::cout << "Label: Rendering text '" << display_text << "' on Node2D at (" << node2d->GetPosition().x << ", " << node2d->GetPosition().y << ")" << std::endl;
            RenderTextNode2D(node2d);
        }
    }
}

void Label::InitializeExportVariables() {
    AddExportVariable("text", m_text, "Text content to display", ExportVariableType::String);
    AddExportVariable("font_path", m_font_path, "Font selection (system fonts + file browser)", ExportVariableType::FontPath);
    AddExportVariable("font_size", m_font_size, "Font size in pixels", ExportVariableType::Int);
    AddExportVariable("color", m_color, "Text color (RGBA)", ExportVariableType::Color);
    AddExportVariable("text_align", TextAlignToInt(m_text_align), "Text alignment (0=Left, 1=Center, 2=Right, 3=Justify)", ExportVariableType::Int);
    AddExportVariable("vertical_align", VerticalAlignToInt(m_vertical_align), "Vertical alignment (0=Top, 1=Center, 2=Bottom)", ExportVariableType::Int);
    AddExportVariable("word_wrap", m_word_wrap, "Enable word wrapping", ExportVariableType::Bool);
    AddExportVariable("line_spacing", m_line_spacing, "Line spacing multiplier", ExportVariableType::Float);
    AddExportVariable("kerning", m_kerning, "Kerning adjustment (additional spacing between characters)", ExportVariableType::Float);

    // Advanced text rendering properties
    AddExportVariable("outline_enabled", m_outline_enabled, "Enable text outline", ExportVariableType::Bool);
    AddExportVariable("outline_color", m_outline_color, "Outline color (RGBA)", ExportVariableType::Color);
    AddExportVariable("outline_width", m_outline_width, "Outline width in pixels", ExportVariableType::Float);
    AddExportVariable("shadow_enabled", m_shadow_enabled, "Enable text shadow", ExportVariableType::Bool);
    AddExportVariable("shadow_color", m_shadow_color, "Shadow color (RGBA)", ExportVariableType::Color);
    AddExportVariable("shadow_offset", m_shadow_offset, "Shadow offset (X, Y)", ExportVariableType::Vec2);

    // Localization properties
    AddExportVariable("use_localization_key", m_use_localization_key, "Use localization key instead of direct text", ExportVariableType::Bool);
    AddExportVariable("localization_key", m_localization_key, "Localization key for text", ExportVariableType::String);
}

void Label::LoadFont() {
    std::cout << "Label: LoadFont called with font: '" << m_font_path.GetDisplayName() << "', size: " << m_font_size << std::endl;

    if (m_font_size <= 0) {
        std::cout << "Label: Invalid font size, skipping font load" << std::endl;
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
        std::cout << "Label: Loaded font " << m_font_path.GetDisplayName() << " (size: " << m_font_size << ")" << std::endl;
    } else {
        m_font_handle = nullptr;
        m_font_loaded = false;
        std::cerr << "Label: Failed to load font " << m_font_path.GetDisplayName() << std::endl;

        // Try fallback to default system font
        FontPath fallback_font("Arial", true, "Regular");
        font = ResourceManager::LoadFont(fallback_font, m_font_size);
        if (font.IsValid()) {
            m_font_handle = font.font_data;
            m_font_loaded = true;
            std::cout << "Label: Loaded fallback font Arial (size: " << m_font_size << ")" << std::endl;
        }
    }
}

void Label::UpdateExportVariables() {
    SetExportVariable("text", m_text);
    SetExportVariable("font_path", m_font_path);
    SetExportVariable("font_size", m_font_size);
    SetExportVariable("color", m_color);
    SetExportVariable("text_align", TextAlignToInt(m_text_align));
    SetExportVariable("vertical_align", VerticalAlignToInt(m_vertical_align));
    SetExportVariable("word_wrap", m_word_wrap);
    SetExportVariable("line_spacing", m_line_spacing);
}

void Label::UpdateFromExportVariables() {
    m_text = GetExportVariableValue<std::string>("text", m_text);

    FontPath new_font_path = GetExportVariableValue<FontPath>("font_path", m_font_path);
    int new_font_size = GetExportVariableValue<int>("font_size", m_font_size);

    if (new_font_path != m_font_path || new_font_size != m_font_size) {
        m_font_path = new_font_path;
        m_font_size = new_font_size;
        m_font_loaded = false;
        LoadFont();
    }

    m_color = GetExportVariableValue<glm::vec4>("color", m_color);
    m_text_align = IntToTextAlign(GetExportVariableValue<int>("text_align", TextAlignToInt(m_text_align)));
    m_vertical_align = IntToVerticalAlign(GetExportVariableValue<int>("vertical_align", VerticalAlignToInt(m_vertical_align)));
    m_word_wrap = GetExportVariableValue<bool>("word_wrap", m_word_wrap);
    m_line_spacing = GetExportVariableValue<float>("line_spacing", m_line_spacing);

    // Advanced text rendering properties
    m_outline_enabled = GetExportVariableValue<bool>("outline_enabled", m_outline_enabled);
    m_outline_color = GetExportVariableValue<glm::vec4>("outline_color", m_outline_color);
    m_outline_width = GetExportVariableValue<float>("outline_width", m_outline_width);
    m_shadow_enabled = GetExportVariableValue<bool>("shadow_enabled", m_shadow_enabled);
    m_shadow_color = GetExportVariableValue<glm::vec4>("shadow_color", m_shadow_color);
    m_shadow_offset = GetExportVariableValue<glm::vec2>("shadow_offset", m_shadow_offset);
}

int Label::TextAlignToInt(TextAlign align) const {
    return static_cast<int>(align);
}

Label::TextAlign Label::IntToTextAlign(int align) const {
    if (align >= 0 && align <= 3) {
        return static_cast<TextAlign>(align);
    }
    return TextAlign::Left;
}

int Label::VerticalAlignToInt(VerticalAlign align) const {
    return static_cast<int>(align);
}

Label::VerticalAlign Label::IntToVerticalAlign(int align) const {
    if (align >= 0 && align <= 2) {
        return static_cast<VerticalAlign>(align);
    }
    return VerticalAlign::Top;
}

void Label::RenderTextControl(Control* control) {
    std::string display_text = GetDisplayText();
    if (!m_font_handle || display_text.empty()) {
        return;
    }

    // Get control's position and size for text layout
    glm::vec2 position = control->GetPosition();
    glm::vec2 size = control->GetSize();

    // Convert Label alignment enums to TextRenderer enums
    TextAlignment horizontal_align = TextAlignment::LEFT;
    switch (m_text_align) {
        case TextAlign::Center: horizontal_align = TextAlignment::CENTER; break;
        case TextAlign::Right: horizontal_align = TextAlignment::RIGHT; break;
        case TextAlign::Justify: horizontal_align = TextAlignment::JUSTIFY; break;
        default: horizontal_align = TextAlignment::LEFT; break;
    }

    VerticalAlignment vertical_align = VerticalAlignment::TOP;
    switch (m_vertical_align) {
        case VerticalAlign::Center: vertical_align = VerticalAlignment::CENTER; break;
        case VerticalAlign::Bottom: vertical_align = VerticalAlignment::BOTTOM; break;
        default: vertical_align = VerticalAlignment::TOP; break;
    }

    // Set up text rendering parameters
    TextRenderParams params;
    params.text = display_text;
    params.font_path = m_font_path;
    params.font_size = m_font_size;
    params.color = m_color;
    params.horizontal_align = horizontal_align;
    params.vertical_align = vertical_align;
    params.line_spacing = m_line_spacing;
    params.kerning = m_kerning;
    // Only set bounds and enable word wrap if the label explicitly has word wrap enabled
    if (m_word_wrap && size.x > 0.0f && size.y > 0.0f) {
        params.bounds = size;
        params.word_wrap = true;
    } else {
        params.bounds = glm::vec2(0.0f); // No bounds
        params.word_wrap = false;
    }

    // Render text using the unified text renderer
    TextRenderer::RenderText(params, position);
}

void Label::RenderTextNode2D(Node2D* node2d) {
    std::string display_text = GetDisplayText();
    if (!m_font_handle || display_text.empty()) {
        return;
    }

    // Get node's position for text layout
    glm::vec2 position = node2d->GetGlobalPosition();

    // Convert Label alignment enums to TextRenderer enums
    TextAlignment horizontal_align = TextAlignment::LEFT;
    switch (m_text_align) {
        case TextAlign::Center: horizontal_align = TextAlignment::CENTER; break;
        case TextAlign::Right: horizontal_align = TextAlignment::RIGHT; break;
        case TextAlign::Justify: horizontal_align = TextAlignment::JUSTIFY; break;
        default: horizontal_align = TextAlignment::LEFT; break;
    }

    VerticalAlignment vertical_align = VerticalAlignment::TOP;
    switch (m_vertical_align) {
        case VerticalAlign::Center: vertical_align = VerticalAlignment::CENTER; break;
        case VerticalAlign::Bottom: vertical_align = VerticalAlignment::BOTTOM; break;
        default: vertical_align = VerticalAlignment::TOP; break;
    }

    // Set up text rendering parameters
    TextRenderParams params;
    params.text = display_text;
    params.font_path = m_font_path;
    params.font_size = m_font_size;
    params.color = m_color;
    params.horizontal_align = horizontal_align;
    params.vertical_align = vertical_align;
    params.line_spacing = m_line_spacing;
    params.kerning = m_kerning;
    params.bounds = glm::vec2(0.0f); // No bounds for Node2D
    params.word_wrap = false; // No word wrap for Node2D

    // Render text using the unified text renderer
    TextRenderer::RenderText(params, position);
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(Label, "UI", "Text label component for displaying text")
