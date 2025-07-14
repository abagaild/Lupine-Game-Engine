#include "lupine/components/Button.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Control.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/TextRenderer.h"
#include "lupine/localization/LocalizationManager.h"
#include "lupine/input/InputManager.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_events.h>
#include <iostream>

namespace Lupine {

Button::Button()
    : Component("Button")
    , m_text("Button")
    , m_font_path(FontPath("Arial", true, "Regular"))
    , m_font_size(16)
    , m_text_color(0.875f, 0.875f, 0.875f, 1.0f) // Light gray text like Godot
    , m_background_color(0.24f, 0.26f, 0.31f, 1.0f) // Godot-like dark blue-gray
    , m_hover_color(0.28f, 0.30f, 0.36f, 1.0f) // Lighter on hover
    , m_pressed_color(0.20f, 0.22f, 0.26f, 1.0f) // Darker when pressed
    , m_disabled_color(0.15f, 0.16f, 0.18f, 1.0f) // Darker for disabled
    , m_disabled(false)
    , m_current_state(ButtonState::Normal)
    , m_hover_modulation(1.2f) // 20% brighter on hover
    , m_click_modulation(0.8f) // 20% darker on click
    , m_disabled_modulation(0.5f) // 50% darker when disabled
    , m_tween_params()
    , m_tween_time(0.0f)
    , m_target_state(ButtonState::Normal)
    , m_current_scale(1.0f, 1.0f)
    , m_current_offset(0.0f, 0.0f)
    , m_start_scale(1.0f, 1.0f)
    , m_start_offset(0.0f, 0.0f)
    , m_target_scale(1.0f, 1.0f)
    , m_target_offset(0.0f, 0.0f)
    , m_corner_radius(6.0f) // More rounded like Godot
    , m_border_width(1.0f) // Subtle border
    , m_border_color(0.15f, 0.17f, 0.20f, 1.0f) // Dark border
    , m_text_align(TextAlign::Center)
    , m_vertical_align(VerticalAlign::Center)
    , m_icon_path("")
    , m_icon_size(16.0f, 16.0f)
    , m_padding(12.0f, 6.0f, 12.0f, 6.0f) // More generous padding like Godot
    , m_use_localization_key(false)
    , m_localization_key("")
    , m_font_handle(nullptr)
    , m_font_loaded(false)
    , m_icon_texture_id(0)
    , m_icon_loaded(false)
    , m_is_mouse_over(false)
    , m_is_pressed(false)
    , m_is_being_destroyed(false)
    , m_is_focused(false) {

    // Initialize export variables
    InitializeExportVariables();
}

void Button::SetText(const std::string& text) {
    m_text = text;
    SetExportVariable("text", text);
}

void Button::SetFontPath(const FontPath& path) {
    if (m_font_path != path) {
        m_font_path = path;
        SetExportVariable("font_path", path);
        m_font_loaded = false;
        LoadFont();
    }
}

void Button::SetFontSize(int size) {
    if (m_font_size != size) {
        m_font_size = size;
        SetExportVariable("font_size", size);
        m_font_loaded = false;
        LoadFont();
    }
}

void Button::SetTextColor(const glm::vec4& color) {
    m_text_color = color;
    SetExportVariable("text_color", color);
}

void Button::SetBackgroundColor(const glm::vec4& color) {
    m_background_color = color;
    SetExportVariable("background_color", color);
}

void Button::SetHoverColor(const glm::vec4& color) {
    m_hover_color = color;
    SetExportVariable("hover_color", color);
}

void Button::SetPressedColor(const glm::vec4& color) {
    m_pressed_color = color;
    SetExportVariable("pressed_color", color);
}

void Button::SetDisabledColor(const glm::vec4& color) {
    m_disabled_color = color;
    SetExportVariable("disabled_color", color);
}

void Button::SetDisabled(bool disabled) {
    m_disabled = disabled;
    SetExportVariable("disabled", disabled);
    if (disabled) {
        m_current_state = ButtonState::Disabled;
    }
}

void Button::SetFocused(bool focused) {
    m_is_focused = focused;
    if (focused && !m_disabled) {
        m_current_state = ButtonState::Focused;
    } else if (!focused && m_current_state == ButtonState::Focused) {
        m_current_state = ButtonState::Normal;
    }
}

void Button::SetOnClickCallback(std::function<void()> callback) {
    m_on_click_callback = callback;
}

void Button::SetCornerRadius(float radius) {
    m_corner_radius = std::max(0.0f, radius);
    SetExportVariable("corner_radius", m_corner_radius);
}

void Button::SetBorderWidth(float width) {
    m_border_width = std::max(0.0f, width);
    SetExportVariable("border_width", m_border_width);
}

void Button::SetBorderColor(const glm::vec4& color) {
    m_border_color = color;
    SetExportVariable("border_color", color);
}



void Button::SetHoverModulation(float modulation) {
    m_hover_modulation = modulation;
    SetExportVariable("hover_modulation", modulation);
}

void Button::SetClickModulation(float modulation) {
    m_click_modulation = modulation;
    SetExportVariable("click_modulation", modulation);
}

void Button::SetDisabledModulation(float modulation) {
    m_disabled_modulation = modulation;
    SetExportVariable("disabled_modulation", modulation);
}

void Button::SetTweenParams(const TweenParams& params) {
    m_tween_params = params;
    // Export individual tween parameters
    SetExportVariable("tween_duration", params.duration);
    SetExportVariable("tween_scale_enabled", params.scale_enabled);
    SetExportVariable("tween_position_enabled", params.position_enabled);
    SetExportVariable("hover_scale", params.hover_scale);
    SetExportVariable("pressed_scale", params.pressed_scale);
    SetExportVariable("hover_offset", params.hover_offset);
    SetExportVariable("pressed_offset", params.pressed_offset);
}

void Button::SetTextAlign(TextAlign align) {
    m_text_align = align;
    SetExportVariable("text_align", static_cast<int>(align));
}

void Button::SetVerticalAlign(VerticalAlign align) {
    m_vertical_align = align;
    SetExportVariable("vertical_align", static_cast<int>(align));
}

void Button::SetIconPath(const std::string& path) {
    m_icon_path = path;
    SetExportVariable("icon_path", path);
    m_icon_loaded = false; // Force reload
}

void Button::SetIconSize(const glm::vec2& size) {
    m_icon_size = glm::max(size, glm::vec2(1.0f, 1.0f));
    SetExportVariable("icon_size", size);
}

void Button::SetPadding(const glm::vec4& padding) {
    m_padding = glm::max(padding, glm::vec4(0.0f));
    SetExportVariable("padding", padding);
}

void Button::SetUseLocalizationKey(bool use_localization) {
    m_use_localization_key = use_localization;
    SetExportVariable("use_localization_key", use_localization);
}

void Button::SetLocalizationKey(const std::string& key) {
    m_localization_key = key;
    SetExportVariable("localization_key", key);
}

std::string Button::GetDisplayText() const {
    if (m_use_localization_key && !m_localization_key.empty()) {
        return LocalizationManager::Instance().GetLocalizedString(m_localization_key, m_text);
    }
    return m_text;
}

void Button::OnReady() {
    UpdateFromExportVariables();
    LoadFont();
    LoadIcon();
}

void Button::OnUpdate(float delta_time) {
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

    // Update tweening animation
    UpdateTweening(delta_time);

    // Check rendering context - Button should only render in 2D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor3D) {
        return; // Don't render 2D buttons in 3D editor view
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

    // Load icon if needed
    if (!m_icon_path.empty() && !m_icon_loaded) {
        LoadIcon();
    }

    // Update button state
    UpdateButtonState();

    // Render the button using safe casting
    if (auto* control = owner->SafeCast<Control>()) {
        RenderBackground(control);
        if (m_icon_loaded && m_icon_texture_id != 0) {
            RenderIcon(control);
        }
        if (m_font_loaded && m_font_handle && !GetDisplayText().empty()) {
            RenderTextControl(control);
        }
    } else if (auto* node2d = owner->SafeCast<Node2D>()) {
        RenderBackgroundNode2D(node2d);
        if (m_icon_loaded && m_icon_texture_id != 0) {
            RenderIconNode2D(node2d);
        }
        if (m_font_loaded && m_font_handle && !GetDisplayText().empty()) {
            RenderTextNode2D(node2d);
        }
    }
}

void Button::OnDestroy() {
    // Mark as being destroyed to prevent further rendering
    m_is_being_destroyed = true;

    // Clean up resources
    if (m_font_handle) {
        // Font cleanup is handled by ResourceManager
        m_font_handle = nullptr;
        m_font_loaded = false;
    }

    if (m_icon_texture_id != 0) {
        // Texture cleanup is handled by ResourceManager
        m_icon_texture_id = 0;
        m_icon_loaded = false;
    }

    // Clear callbacks to prevent dangling function pointers
    m_on_click_callback = nullptr;
    m_on_hover_callback = nullptr;
    m_on_focus_callback = nullptr;
}

void Button::OnInput(const void* event) {
    if (m_disabled || !event || m_is_being_destroyed) return;

    // Cast to SDL_Event for input handling
    const SDL_Event* sdl_event = static_cast<const SDL_Event*>(event);

    switch (sdl_event->type) {
        case SDL_KEYDOWN:
            if (m_is_focused) {
                if (sdl_event->key.keysym.sym == SDLK_RETURN || sdl_event->key.keysym.sym == SDLK_SPACE) {
                    // Trigger button click on Enter or Space
                    if (m_on_click_callback) {
                        m_on_click_callback();
                    }
                }
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (sdl_event->button.button == SDL_BUTTON_LEFT) {
                glm::vec2 mouse_pos(sdl_event->button.x, sdl_event->button.y);
                if (IsPointInside(mouse_pos)) {
                    m_is_pressed = true;
                    StartTween(ButtonState::Pressed);
                }
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (sdl_event->button.button == SDL_BUTTON_LEFT && m_is_pressed) {
                glm::vec2 mouse_pos(sdl_event->button.x, sdl_event->button.y);
                if (IsPointInside(mouse_pos)) {
                    // Trigger click callback
                    if (m_on_click_callback) {
                        m_on_click_callback();
                    }
                    StartTween(ButtonState::Hover);
                } else {
                    StartTween(ButtonState::Normal);
                }
                m_is_pressed = false;
            }
            break;

        case SDL_MOUSEMOTION: {
            glm::vec2 mouse_pos(sdl_event->motion.x, sdl_event->motion.y);
            UpdateMouseHover(mouse_pos);
            break;
        }
    }
}

void Button::InitializeExportVariables() {
    AddExportVariable("text", m_text, "Button text to display", ExportVariableType::String);
    AddExportVariable("font_path", m_font_path, "Font selection (system fonts + file browser)", ExportVariableType::FontPath);
    AddExportVariable("font_size", m_font_size, "Font size in pixels", ExportVariableType::Int);
    AddExportVariable("text_color", m_text_color, "Text color (RGBA)", ExportVariableType::Color);
    AddExportVariable("background_color", m_background_color, "Background color (RGBA)", ExportVariableType::Color);
    AddExportVariable("hover_color", m_hover_color, "Hover background color (RGBA)", ExportVariableType::Color);
    AddExportVariable("pressed_color", m_pressed_color, "Pressed background color (RGBA)", ExportVariableType::Color);
    AddExportVariable("disabled_color", m_disabled_color, "Disabled background color (RGBA)", ExportVariableType::Color);
    AddExportVariable("disabled", m_disabled, "Disable button interaction", ExportVariableType::Bool);

    // Modulation properties
    AddExportVariable("hover_modulation", m_hover_modulation, "Hover color modulation factor", ExportVariableType::Float);
    AddExportVariable("click_modulation", m_click_modulation, "Click color modulation factor", ExportVariableType::Float);
    AddExportVariable("disabled_modulation", m_disabled_modulation, "Disabled color modulation factor", ExportVariableType::Float);

    // Tweening properties
    AddExportVariable("tween_duration", m_tween_params.duration, "Tween animation duration in seconds", ExportVariableType::Float);
    AddExportVariable("tween_scale_enabled", m_tween_params.scale_enabled, "Enable scale tweening", ExportVariableType::Bool);
    AddExportVariable("tween_position_enabled", m_tween_params.position_enabled, "Enable position tweening", ExportVariableType::Bool);
    AddExportVariable("hover_scale", m_tween_params.hover_scale, "Scale factor on hover", ExportVariableType::Vec2);
    AddExportVariable("pressed_scale", m_tween_params.pressed_scale, "Scale factor when pressed", ExportVariableType::Vec2);
    AddExportVariable("hover_offset", m_tween_params.hover_offset, "Position offset on hover", ExportVariableType::Vec2);
    AddExportVariable("pressed_offset", m_tween_params.pressed_offset, "Position offset when pressed", ExportVariableType::Vec2);

    // Enhanced properties
    AddExportVariable("corner_radius", m_corner_radius, "Corner radius in pixels", ExportVariableType::Float);
    AddExportVariable("border_width", m_border_width, "Border width in pixels", ExportVariableType::Float);
    AddExportVariable("border_color", m_border_color, "Border color (RGBA)", ExportVariableType::Color);
    AddExportVariable("text_align", static_cast<int>(m_text_align), "Text alignment (0=Left, 1=Center, 2=Right)", ExportVariableType::Int);
    AddExportVariable("vertical_align", static_cast<int>(m_vertical_align), "Vertical alignment (0=Top, 1=Center, 2=Bottom)", ExportVariableType::Int);
    AddExportVariable("icon_path", m_icon_path, "Path to icon texture", ExportVariableType::FilePath);
    AddExportVariable("icon_size", m_icon_size, "Icon size in pixels", ExportVariableType::Vec2);
    AddExportVariable("padding", m_padding, "Padding (left, top, right, bottom)", ExportVariableType::Vec4);

    // Localization properties
    AddExportVariable("use_localization_key", m_use_localization_key, "Use localization key instead of direct text", ExportVariableType::Bool);
    AddExportVariable("localization_key", m_localization_key, "Localization key for text", ExportVariableType::String);
}

void Button::UpdateExportVariables() {
    SetExportVariable("text", m_text);
    SetExportVariable("font_path", m_font_path);
    SetExportVariable("font_size", m_font_size);
    SetExportVariable("text_color", m_text_color);
    SetExportVariable("background_color", m_background_color);
    SetExportVariable("hover_color", m_hover_color);
    SetExportVariable("pressed_color", m_pressed_color);
    SetExportVariable("disabled_color", m_disabled_color);
    SetExportVariable("disabled", m_disabled);

    // Modulation properties
    SetExportVariable("hover_modulation", m_hover_modulation);
    SetExportVariable("click_modulation", m_click_modulation);
    SetExportVariable("disabled_modulation", m_disabled_modulation);

    // Tweening properties
    SetExportVariable("tween_duration", m_tween_params.duration);
    SetExportVariable("tween_scale_enabled", m_tween_params.scale_enabled);
    SetExportVariable("tween_position_enabled", m_tween_params.position_enabled);
    SetExportVariable("hover_scale", m_tween_params.hover_scale);
    SetExportVariable("pressed_scale", m_tween_params.pressed_scale);
    SetExportVariable("hover_offset", m_tween_params.hover_offset);
    SetExportVariable("pressed_offset", m_tween_params.pressed_offset);

    // Enhanced properties
    SetExportVariable("corner_radius", m_corner_radius);
    SetExportVariable("border_width", m_border_width);
    SetExportVariable("border_color", m_border_color);
    SetExportVariable("text_align", static_cast<int>(m_text_align));
    SetExportVariable("vertical_align", static_cast<int>(m_vertical_align));
    SetExportVariable("icon_path", m_icon_path);
    SetExportVariable("icon_size", m_icon_size);
    SetExportVariable("padding", m_padding);

    // Localization properties
    SetExportVariable("use_localization_key", m_use_localization_key);
    SetExportVariable("localization_key", m_localization_key);
}

void Button::UpdateFromExportVariables() {
    m_text = GetExportVariableValue<std::string>("text", m_text);

    FontPath new_font_path = GetExportVariableValue<FontPath>("font_path", m_font_path);
    int new_font_size = GetExportVariableValue<int>("font_size", m_font_size);

    if (new_font_path != m_font_path || new_font_size != m_font_size) {
        m_font_path = new_font_path;
        m_font_size = new_font_size;
        m_font_loaded = false;
        LoadFont();
    }

    m_text_color = GetExportVariableValue<glm::vec4>("text_color", m_text_color);
    m_background_color = GetExportVariableValue<glm::vec4>("background_color", m_background_color);
    m_hover_color = GetExportVariableValue<glm::vec4>("hover_color", m_hover_color);
    m_pressed_color = GetExportVariableValue<glm::vec4>("pressed_color", m_pressed_color);
    m_disabled_color = GetExportVariableValue<glm::vec4>("disabled_color", m_disabled_color);

    // Modulation properties
    m_hover_modulation = GetExportVariableValue<float>("hover_modulation", m_hover_modulation);
    m_click_modulation = GetExportVariableValue<float>("click_modulation", m_click_modulation);
    m_disabled_modulation = GetExportVariableValue<float>("disabled_modulation", m_disabled_modulation);

    // Tweening properties
    m_tween_params.duration = GetExportVariableValue<float>("tween_duration", m_tween_params.duration);
    m_tween_params.scale_enabled = GetExportVariableValue<bool>("tween_scale_enabled", m_tween_params.scale_enabled);
    m_tween_params.position_enabled = GetExportVariableValue<bool>("tween_position_enabled", m_tween_params.position_enabled);
    m_tween_params.hover_scale = GetExportVariableValue<glm::vec2>("hover_scale", m_tween_params.hover_scale);
    m_tween_params.pressed_scale = GetExportVariableValue<glm::vec2>("pressed_scale", m_tween_params.pressed_scale);
    m_tween_params.hover_offset = GetExportVariableValue<glm::vec2>("hover_offset", m_tween_params.hover_offset);
    m_tween_params.pressed_offset = GetExportVariableValue<glm::vec2>("pressed_offset", m_tween_params.pressed_offset);

    bool new_disabled = GetExportVariableValue<bool>("disabled", m_disabled);
    if (new_disabled != m_disabled) {
        SetDisabled(new_disabled);
    }

    // Enhanced properties
    m_corner_radius = GetExportVariableValue<float>("corner_radius", m_corner_radius);
    m_border_width = GetExportVariableValue<float>("border_width", m_border_width);
    m_border_color = GetExportVariableValue<glm::vec4>("border_color", m_border_color);
    m_text_align = static_cast<TextAlign>(GetExportVariableValue<int>("text_align", static_cast<int>(m_text_align)));
    m_vertical_align = static_cast<VerticalAlign>(GetExportVariableValue<int>("vertical_align", static_cast<int>(m_vertical_align)));

    std::string new_icon_path = GetExportVariableValue<std::string>("icon_path", m_icon_path);
    if (new_icon_path != m_icon_path) {
        m_icon_path = new_icon_path;
        m_icon_loaded = false;
        LoadIcon();
    }

    m_icon_size = GetExportVariableValue<glm::vec2>("icon_size", m_icon_size);
    m_padding = GetExportVariableValue<glm::vec4>("padding", m_padding);

    // Localization properties
    m_use_localization_key = GetExportVariableValue<bool>("use_localization_key", m_use_localization_key);
    m_localization_key = GetExportVariableValue<std::string>("localization_key", m_localization_key);
}

void Button::LoadFont() {
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
        std::cout << "Button: Loaded font " << m_font_path.GetDisplayName() << " (size: " << m_font_size << ")" << std::endl;
    } else {
        m_font_handle = nullptr;
        m_font_loaded = false;
        std::cerr << "Button: Failed to load font " << m_font_path.GetDisplayName() << std::endl;

        // Try fallback to default system font
        FontPath fallback_font("Arial", true, "Regular");
        font = ResourceManager::LoadFont(fallback_font, m_font_size);
        if (font.IsValid()) {
            m_font_handle = font.font_data;
            m_font_loaded = true;
            std::cout << "Button: Loaded fallback font Arial (size: " << m_font_size << ")" << std::endl;
        }
    }
}

std::string Button::GetTruncatedText(const std::string& text, float max_width) const {
    if (max_width <= 0.0f || !m_font_loaded || !m_font_handle) {
        return text;
    }

    // Use TextRenderer to measure text
    glm::vec2 text_size = TextRenderer::MeasureText(text, m_font_path, m_font_size);

    if (text_size.x <= max_width) {
        return text; // No truncation needed
    }

    // Text is too wide, truncate with ellipsis
    std::string ellipsis = "...";
    glm::vec2 ellipsis_size = TextRenderer::MeasureText(ellipsis, m_font_path, m_font_size);

    if (ellipsis_size.x >= max_width) {
        return ellipsis; // Can only fit ellipsis
    }

    // Binary search to find the maximum text that fits
    size_t left = 0;
    size_t right = text.length();
    std::string best_fit = ellipsis;

    while (left <= right && right > 0) {
        size_t mid = left + (right - left) / 2;
        std::string test_text = text.substr(0, mid) + ellipsis;
        glm::vec2 test_size = TextRenderer::MeasureText(test_text, m_font_path, m_font_size);

        if (test_size.x <= max_width) {
            best_fit = test_text;
            left = mid + 1;
        } else {
            if (mid == 0) break;
            right = mid - 1;
        }
    }

    return best_fit;
}

float Button::GetKerningBetweenChars(char left, char right) const {
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

void Button::LoadIcon() {
    if (m_icon_path.empty()) {
        m_icon_loaded = false;
        m_icon_texture_id = 0;
        return;
    }

    // Initialize ResourceManager if not already done
    if (!ResourceManager::IsInitialized()) {
        ResourceManager::Initialize();
    }

    // Load texture using ResourceManager
    Texture texture = ResourceManager::LoadTexture(m_icon_path);
    if (texture.IsValid()) {
        m_icon_texture_id = texture.id;
        m_icon_loaded = true;
        std::cout << "Button: Loaded icon " << m_icon_path << std::endl;
    } else {
        m_icon_texture_id = 0;
        m_icon_loaded = false;
        std::cerr << "Button: Failed to load icon " << m_icon_path << std::endl;
    }
}

void Button::RenderBackground(Control* control) {
    if (!control) return;

    glm::vec2 base_position = control->GetPosition();
    glm::vec2 base_size = control->GetSize();

    // Apply current scale and offset from tweening
    glm::vec2 position = base_position + m_current_offset;
    glm::vec2 size = base_size * m_current_scale;

    // Adjust position to keep button centered when scaling
    glm::vec2 scale_offset = (base_size - size) * 0.5f;
    position += scale_offset;

    // Get current background color based on state
    glm::vec4 bg_color = GetCurrentBackgroundColor();

    // Render shadow first (slightly offset and darker)
    if (m_current_state != ButtonState::Pressed) {
        glm::vec2 shadow_offset(2.0f, 2.0f);
        glm::mat4 shadow_transform = glm::mat4(1.0f);
        shadow_transform = glm::translate(shadow_transform, glm::vec3(position.x + shadow_offset.x, position.y + shadow_offset.y, 0.0f));
        shadow_transform = glm::scale(shadow_transform, glm::vec3(size.x, size.y, 1.0f));

        glm::vec4 shadow_color(0.0f, 0.0f, 0.0f, 0.3f); // Semi-transparent black shadow
        if (m_corner_radius > 0.0f) {
            Renderer::RenderRoundedQuad(shadow_transform, shadow_color, m_corner_radius, size);
        } else {
            Renderer::RenderQuad(shadow_transform, shadow_color);
        }
    }

    // Render main background
    glm::mat4 bg_transform = glm::mat4(1.0f);
    bg_transform = glm::translate(bg_transform, glm::vec3(position.x, position.y, 0.0f));
    bg_transform = glm::scale(bg_transform, glm::vec3(size.x, size.y, 1.0f));

    if (m_corner_radius > 0.0f) {
        Renderer::RenderRoundedQuad(bg_transform, bg_color, m_corner_radius, size);
    } else {
        Renderer::RenderQuad(bg_transform, bg_color);
    }

    // Render border if enabled
    if (m_border_width > 0.0f) {
        RenderBorder(position, size);
    }

    // Add highlight effect for hover state
    if (m_current_state == ButtonState::Hover) {
        glm::mat4 highlight_transform = glm::mat4(1.0f);
        highlight_transform = glm::translate(highlight_transform, glm::vec3(position.x, position.y, 0.0f));
        highlight_transform = glm::scale(highlight_transform, glm::vec3(size.x, 2.0f, 1.0f)); // Thin highlight line at top

        glm::vec4 highlight_color(1.0f, 1.0f, 1.0f, 0.2f); // Semi-transparent white highlight
        Renderer::RenderQuad(highlight_transform, highlight_color);
    }

    // Add focus outline for focused state
    if (m_current_state == ButtonState::Focused || m_is_focused) {
        float outline_width = 2.0f;
        glm::vec4 focus_color(0.4f, 0.7f, 1.0f, 0.8f); // Blue focus outline like Godot

        // Top outline
        glm::mat4 top_outline = glm::mat4(1.0f);
        top_outline = glm::translate(top_outline, glm::vec3(position.x - outline_width, position.y - outline_width, 0.0f));
        top_outline = glm::scale(top_outline, glm::vec3(size.x + 2 * outline_width, outline_width, 1.0f));
        Renderer::RenderQuad(top_outline, focus_color);

        // Bottom outline
        glm::mat4 bottom_outline = glm::mat4(1.0f);
        bottom_outline = glm::translate(bottom_outline, glm::vec3(position.x - outline_width, position.y + size.y, 0.0f));
        bottom_outline = glm::scale(bottom_outline, glm::vec3(size.x + 2 * outline_width, outline_width, 1.0f));
        Renderer::RenderQuad(bottom_outline, focus_color);

        // Left outline
        glm::mat4 left_outline = glm::mat4(1.0f);
        left_outline = glm::translate(left_outline, glm::vec3(position.x - outline_width, position.y, 0.0f));
        left_outline = glm::scale(left_outline, glm::vec3(outline_width, size.y, 1.0f));
        Renderer::RenderQuad(left_outline, focus_color);

        // Right outline
        glm::mat4 right_outline = glm::mat4(1.0f);
        right_outline = glm::translate(right_outline, glm::vec3(position.x + size.x, position.y, 0.0f));
        right_outline = glm::scale(right_outline, glm::vec3(outline_width, size.y, 1.0f));
        Renderer::RenderQuad(right_outline, focus_color);
    }
}

void Button::RenderBackgroundNode2D(Node2D* node2d) {
    if (!node2d) return;

    glm::vec2 base_position = node2d->GetGlobalPosition();
    glm::vec2 node_scale = node2d->GetScale();

    // For Node2D, use a default size that can be scaled
    glm::vec2 default_size(100.0f, 30.0f);
    glm::vec2 base_size = default_size * node_scale;

    // Apply current scale and offset from tweening
    glm::vec2 position = base_position + m_current_offset;
    glm::vec2 size = base_size * m_current_scale;

    // Adjust position to keep button centered when scaling
    glm::vec2 scale_offset = (base_size - size) * 0.5f;
    position += scale_offset;

    // Get current background color based on state
    glm::vec4 bg_color = GetCurrentBackgroundColor();

    // Render shadow first (slightly offset and darker)
    if (m_current_state != ButtonState::Pressed) {
        glm::vec2 shadow_offset(2.0f * node_scale.x, 2.0f * node_scale.y);
        glm::mat4 shadow_transform = glm::mat4(1.0f);
        shadow_transform = glm::translate(shadow_transform, glm::vec3(position.x + shadow_offset.x, position.y + shadow_offset.y, 0.0f));
        shadow_transform = glm::scale(shadow_transform, glm::vec3(size.x, size.y, 1.0f));

        glm::vec4 shadow_color(0.0f, 0.0f, 0.0f, 0.3f); // Semi-transparent black shadow
        if (m_corner_radius > 0.0f) {
            Renderer::RenderRoundedQuad(shadow_transform, shadow_color, m_corner_radius, size);
        } else {
            Renderer::RenderQuad(shadow_transform, shadow_color);
        }
    }

    // Render main background
    glm::mat4 bg_transform = glm::mat4(1.0f);
    bg_transform = glm::translate(bg_transform, glm::vec3(position.x, position.y, 0.0f));
    bg_transform = glm::scale(bg_transform, glm::vec3(size.x, size.y, 1.0f));

    if (m_corner_radius > 0.0f) {
        Renderer::RenderRoundedQuad(bg_transform, bg_color, m_corner_radius, size);
    } else {
        Renderer::RenderQuad(bg_transform, bg_color);
    }

    // Render border if enabled
    if (m_border_width > 0.0f) {
        RenderBorder(position, size);
    }

    // Add highlight effect for hover state
    if (m_current_state == ButtonState::Hover) {
        glm::mat4 highlight_transform = glm::mat4(1.0f);
        highlight_transform = glm::translate(highlight_transform, glm::vec3(position.x, position.y, 0.0f));
        highlight_transform = glm::scale(highlight_transform, glm::vec3(size.x, 2.0f * node_scale.y, 1.0f)); // Thin highlight line at top

        glm::vec4 highlight_color(1.0f, 1.0f, 1.0f, 0.2f); // Semi-transparent white highlight
        Renderer::RenderQuad(highlight_transform, highlight_color);
    }
}

void Button::RenderTextControl(Control* control) {
    std::string display_text = GetDisplayText();
    if (!control || !m_font_handle || display_text.empty()) {
        return;
    }

    // Get control's position and size for text layout
    glm::vec2 position = control->GetPosition();
    glm::vec2 size = control->GetSize();

    // Apply padding to get content area
    glm::vec2 content_pos = position + glm::vec2(m_padding.x, m_padding.y);
    glm::vec2 content_size = size - glm::vec2(m_padding.x + m_padding.z, m_padding.y + m_padding.w);

    // Handle text truncation if needed
    display_text = GetTruncatedText(display_text, content_size.x);

    // Convert Button alignment enums to TextRenderer enums
    TextAlignment horizontal_align = TextAlignment::LEFT;
    switch (m_text_align) {
        case TextAlign::Center: horizontal_align = TextAlignment::CENTER; break;
        case TextAlign::Right: horizontal_align = TextAlignment::RIGHT; break;
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
    params.color = m_text_color;
    params.horizontal_align = horizontal_align;
    params.vertical_align = vertical_align;
    params.bounds = content_size;
    params.word_wrap = false; // Buttons typically don't word wrap

    // Render text using the unified text renderer
    TextRenderer::RenderText(params, content_pos);
}

void Button::RenderTextNode2D(Node2D* node2d) {
    std::string display_text = GetDisplayText();
    if (!node2d || !m_font_handle || display_text.empty()) {
        return;
    }

    // Get node's position for text layout
    glm::vec2 position = node2d->GetGlobalPosition();
    glm::vec2 scale = node2d->GetScale();

    // For Node2D, use a default size that can be scaled
    glm::vec2 default_size(100.0f, 30.0f);
    glm::vec2 size = default_size * scale;

    // Convert Button alignment enums to TextRenderer enums
    TextAlignment horizontal_align = TextAlignment::CENTER; // Default center for Node2D
    VerticalAlignment vertical_align = VerticalAlignment::CENTER; // Default center for Node2D

    // Set up text rendering parameters
    TextRenderParams params;
    params.text = display_text;
    params.font_path = m_font_path;
    params.font_size = m_font_size;
    params.color = m_text_color;
    params.horizontal_align = horizontal_align;
    params.vertical_align = vertical_align;
    params.bounds = size;
    params.word_wrap = false; // Node2D buttons typically don't word wrap

    // Render text using the unified text renderer
    TextRenderer::RenderText(params, position);
}

bool Button::IsPointInside(const glm::vec2& point) const {
    if (auto* control = dynamic_cast<Control*>(GetOwner())) {
        glm::vec2 position = control->GetPosition();
        glm::vec2 size = control->GetSize();
        return point.x >= position.x && point.x <= position.x + size.x &&
               point.y >= position.y && point.y <= position.y + size.y;
    } else if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
        glm::vec2 position = node2d->GetGlobalPosition();
        glm::vec2 scale = node2d->GetScale();
        glm::vec2 default_size(100.0f, 30.0f);
        glm::vec2 size = default_size * scale;
        return point.x >= position.x && point.x <= position.x + size.x &&
               point.y >= position.y && point.y <= position.y + size.y;
    }
    return false;
}

void Button::UpdateButtonState() {
    if (m_disabled) {
        m_current_state = ButtonState::Disabled;
        m_is_mouse_over = false;
        m_is_pressed = false;
        return;
    }

    // Continuously check mouse hover state using InputManager
    glm::vec2 mouse_pos = InputManager::GetMousePosition();
    UpdateMouseHover(mouse_pos);
}

glm::vec4 Button::GetCurrentBackgroundColor() const {
    glm::vec4 base_color;
    float modulation = 1.0f;

    switch (m_current_state) {
        case ButtonState::Hover:
            base_color = m_hover_color;
            modulation = m_hover_modulation;
            break;
        case ButtonState::Pressed:
            base_color = m_pressed_color;
            modulation = m_click_modulation;
            break;
        case ButtonState::Focused:
            // Focused state - slightly brighter than normal with blue tint like Godot
            base_color = glm::vec4(m_background_color.r * 1.1f, m_background_color.g * 1.1f, m_background_color.b * 1.3f, m_background_color.a);
            break;
        case ButtonState::Disabled:
            base_color = m_disabled_color;
            modulation = m_disabled_modulation;
            break;
        case ButtonState::Normal:
        default:
            base_color = m_background_color;
            break;
    }

    // Apply modulation
    return glm::vec4(base_color.r * modulation, base_color.g * modulation, base_color.b * modulation, base_color.a);
}

void Button::RenderIcon(Control* control) {
    if (!control || !m_icon_loaded || m_icon_texture_id == 0) return;

    glm::vec2 position = control->GetPosition();
    glm::vec2 size = control->GetSize();
    glm::vec2 icon_pos = CalculateIconPosition(position, size);

    // Create transform matrix for icon quad
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(icon_pos.x, icon_pos.y, 0.0f));
    transform = glm::scale(transform, glm::vec3(m_icon_size.x, m_icon_size.y, 1.0f));

    // Render icon with white color (texture provides the color)
    Renderer::RenderQuad(transform, glm::vec4(1.0f), m_icon_texture_id);
}

void Button::RenderIconNode2D(Node2D* node2d) {
    if (!node2d || !m_icon_loaded || m_icon_texture_id == 0) return;

    glm::vec2 position = node2d->GetGlobalPosition();
    glm::vec2 scale = node2d->GetScale();
    glm::vec2 default_size(100.0f, 30.0f);
    glm::vec2 size = default_size * scale;
    glm::vec2 icon_pos = CalculateIconPosition(position, size);

    // Create transform matrix for icon quad
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(icon_pos.x, icon_pos.y, 0.0f));
    transform = glm::scale(transform, glm::vec3(m_icon_size.x * scale.x, m_icon_size.y * scale.y, 1.0f));

    // Render icon with white color (texture provides the color)
    Renderer::RenderQuad(transform, glm::vec4(1.0f), m_icon_texture_id);
}

void Button::RenderBorder(const glm::vec2& position, const glm::vec2& size) {
    if (m_border_width <= 0.0f) return;

    // Top border (full width)
    glm::mat4 top_transform = glm::mat4(1.0f);
    top_transform = glm::translate(top_transform, glm::vec3(position.x, position.y, 0.0f));
    top_transform = glm::scale(top_transform, glm::vec3(size.x, m_border_width, 1.0f));
    Renderer::RenderQuad(top_transform, m_border_color);

    // Bottom border (full width)
    glm::mat4 bottom_transform = glm::mat4(1.0f);
    bottom_transform = glm::translate(bottom_transform, glm::vec3(position.x, position.y + size.y - m_border_width, 0.0f));
    bottom_transform = glm::scale(bottom_transform, glm::vec3(size.x, m_border_width, 1.0f));
    Renderer::RenderQuad(bottom_transform, m_border_color);

    // Left border (excluding corners to prevent overlap)
    glm::mat4 left_transform = glm::mat4(1.0f);
    left_transform = glm::translate(left_transform, glm::vec3(position.x, position.y + m_border_width, 0.0f));
    left_transform = glm::scale(left_transform, glm::vec3(m_border_width, size.y - 2.0f * m_border_width, 1.0f));
    Renderer::RenderQuad(left_transform, m_border_color);

    // Right border (excluding corners to prevent overlap)
    glm::mat4 right_transform = glm::mat4(1.0f);
    right_transform = glm::translate(right_transform, glm::vec3(position.x + size.x - m_border_width, position.y + m_border_width, 0.0f));
    right_transform = glm::scale(right_transform, glm::vec3(m_border_width, size.y - 2.0f * m_border_width, 1.0f));
    Renderer::RenderQuad(right_transform, m_border_color);
}

glm::vec2 Button::CalculateTextPosition(const glm::vec2& container_pos, const glm::vec2& container_size, const glm::vec2& text_size) const {
    glm::vec2 content_pos = container_pos + glm::vec2(m_padding.x, m_padding.y);
    glm::vec2 content_size = container_size - glm::vec2(m_padding.x + m_padding.z, m_padding.y + m_padding.w);

    glm::vec2 text_pos = content_pos;

    // Horizontal alignment
    switch (m_text_align) {
        case TextAlign::Left:
            text_pos.x = content_pos.x;
            break;
        case TextAlign::Center:
            text_pos.x = content_pos.x + (content_size.x - text_size.x) * 0.5f;
            break;
        case TextAlign::Right:
            text_pos.x = content_pos.x + content_size.x - text_size.x;
            break;
    }

    // Vertical alignment
    switch (m_vertical_align) {
        case VerticalAlign::Top:
            text_pos.y = content_pos.y;
            break;
        case VerticalAlign::Center:
            text_pos.y = content_pos.y + (content_size.y - text_size.y) * 0.5f;
            break;
        case VerticalAlign::Bottom:
            text_pos.y = content_pos.y + content_size.y - text_size.y;
            break;
    }

    return text_pos;
}

glm::vec2 Button::CalculateIconPosition(const glm::vec2& container_pos, const glm::vec2& container_size) const {
    glm::vec2 content_pos = container_pos + glm::vec2(m_padding.x, m_padding.y);
    glm::vec2 content_size = container_size - glm::vec2(m_padding.x + m_padding.z, m_padding.y + m_padding.w);

    // For now, center the icon. In the future, we could add icon alignment options
    glm::vec2 icon_pos;
    icon_pos.x = content_pos.x + (content_size.x - m_icon_size.x) * 0.5f;
    icon_pos.y = content_pos.y + (content_size.y - m_icon_size.y) * 0.5f;

    return icon_pos;
}

void Button::UpdateTweening(float delta_time) {
    if (m_current_state == m_target_state) {
        return; // No tweening needed
    }

    m_tween_time += delta_time;
    float t = m_tween_time / m_tween_params.duration;

    if (t >= 1.0f) {
        // Tween complete
        t = 1.0f;
        m_current_state = m_target_state;
    }

    // Apply easing
    float eased_t = ApplyEasing(t);

    // Interpolate scale
    if (m_tween_params.scale_enabled) {
        m_current_scale = m_start_scale + (m_target_scale - m_start_scale) * eased_t;
    }

    // Interpolate position offset
    if (m_tween_params.position_enabled) {
        m_current_offset = m_start_offset + (m_target_offset - m_start_offset) * eased_t;
    }
}

void Button::StartTween(ButtonState target_state) {
    if (target_state == m_current_state) {
        return; // Already in target state
    }

    m_target_state = target_state;
    m_tween_time = 0.0f;

    // Store current values as start values
    m_start_scale = m_current_scale;
    m_start_offset = m_current_offset;

    // Set target values
    m_target_scale = GetScaleForState(target_state);
    m_target_offset = GetOffsetForState(target_state);
}

float Button::ApplyEasing(float t) const {
    // Smooth step easing (ease in-out)
    return t * t * (3.0f - 2.0f * t);
}

glm::vec2 Button::GetScaleForState(ButtonState state) const {
    switch (state) {
        case ButtonState::Hover:
            return m_tween_params.hover_scale;
        case ButtonState::Pressed:
            return m_tween_params.pressed_scale;
        case ButtonState::Normal:
        case ButtonState::Disabled:
        case ButtonState::Focused:
        default:
            return glm::vec2(1.0f, 1.0f);
    }
}

glm::vec2 Button::GetOffsetForState(ButtonState state) const {
    switch (state) {
        case ButtonState::Hover:
            return m_tween_params.hover_offset;
        case ButtonState::Pressed:
            return m_tween_params.pressed_offset;
        case ButtonState::Normal:
        case ButtonState::Disabled:
        case ButtonState::Focused:
        default:
            return glm::vec2(0.0f, 0.0f);
    }
}

void Button::UpdateMouseHover(const glm::vec2& mouse_pos) {
    bool was_hovering = m_is_mouse_over;
    m_is_mouse_over = IsPointInside(mouse_pos);

    if (!m_is_pressed) {
        if (m_is_mouse_over && !was_hovering) {
            // Mouse entered
            StartTween(ButtonState::Hover);
        } else if (!m_is_mouse_over && was_hovering) {
            // Mouse left
            StartTween(ButtonState::Normal);
        }
    }
}

glm::vec2 Button::ScreenToLocal(const glm::vec2& screen_pos) const {
    // For now, assume screen coordinates are the same as local coordinates
    // In a more complex system, this would transform from screen space to local space
    return screen_pos;
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(Button, "UI", "Interactive button component with text and click handling")
