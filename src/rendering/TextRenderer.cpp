#include "lupine/rendering/TextRenderer.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/resources/ResourceManager.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace Lupine {

// Static member initialization
std::unordered_map<std::string, TextLayout> TextRenderer::s_layout_cache;
TextBatch TextRenderer::s_current_batch;
bool TextRenderer::s_batching_enabled = false;

TextLayout TextRenderer::CalculateLayout(const TextRenderParams& params) {
    // Check cache first
    std::string cache_key = GenerateCacheKey(params);
    auto cache_it = s_layout_cache.find(cache_key);
    if (cache_it != s_layout_cache.end()) {
        return cache_it->second;
    }
    
    TextLayout layout;
    layout.cache_key = cache_key;
    
    if (params.text.empty()) {
        layout.is_valid = true;
        return layout;
    }
    
    // Load font and generate glyph atlas
    Font font = ResourceManager::GetFont(params.font_path, params.font_size);
    if (!font.IsValid()) {
        return layout;
    }
    
    auto glyphs = ResourceManager::GenerateGlyphAtlas(font);
    if (glyphs.empty()) {
        return layout;
    }
    
    // Calculate line height with context-aware scaling
    float scale_factor = GetContextAwareScaleFactor(font);
    layout.line_height = font.height * scale_factor * params.line_spacing;
    
    // Split text into lines
    std::vector<std::string> line_texts = SplitTextIntoLines(
        params.text, font, glyphs, params.bounds, params.word_wrap, params.kerning);
    
    // Calculate layout for each line
    float max_width = 0.0f;
    for (size_t i = 0; i < line_texts.size(); ++i) {
        TextLine line;
        line.text = line_texts[i];
        line.width = CalculateLineWidth(line.text, font, glyphs, params.kerning);
        line.y_offset = static_cast<float>(i) * layout.line_height;
        
        // Calculate horizontal alignment offset
        if (params.bounds.x > 0.0f) {
            switch (params.horizontal_align) {
                case TextAlignment::CENTER:
                    line.x_offset = (params.bounds.x - line.width) * 0.5f;
                    break;
                case TextAlignment::RIGHT:
                    line.x_offset = params.bounds.x - line.width;
                    break;
                case TextAlignment::JUSTIFY:
                    // Justify only if not the last line and line has multiple words
                    if (i < line_texts.size() - 1 && line.text.find(' ') != std::string::npos) {
                        line.x_offset = 0.0f; // Will be handled during rendering
                    } else {
                        line.x_offset = 0.0f; // Left align last line
                    }
                    break;
                default: // LEFT
                    line.x_offset = 0.0f;
                    break;
            }
        }
        
        layout.lines.push_back(line);
        max_width = std::max(max_width, line.width);
    }
    
    // Calculate total size
    layout.total_size.x = params.bounds.x > 0.0f ? params.bounds.x : max_width;
    layout.total_size.y = layout.lines.size() * layout.line_height;
    layout.is_valid = true;
    
    // Cache the layout
    if (s_layout_cache.size() >= MAX_CACHE_SIZE) {
        s_layout_cache.clear(); // Simple cache eviction
    }
    s_layout_cache[cache_key] = layout;
    
    return layout;
}

void TextRenderer::RenderLayout(const TextLayout& layout, const glm::vec2& position, const TextRenderParams& params) {
    if (!layout.is_valid || layout.lines.empty()) {
        return;
    }
    
    // Load font and generate glyph atlas
    Font font = ResourceManager::GetFont(params.font_path, params.font_size);
    if (!font.IsValid()) {
        return;
    }
    
    auto glyphs = ResourceManager::GenerateGlyphAtlas(font);
    if (glyphs.empty()) {
        return;
    }
    
    float scale_factor = GetContextAwareScaleFactor(font);
    
    // Calculate vertical alignment offset
    float vertical_offset = 0.0f;
    switch (params.vertical_align) {
        case VerticalAlignment::CENTER:
            // Center the text block vertically within the bounds
            if (params.bounds.y > 0.0f) {
                vertical_offset = (params.bounds.y - layout.total_size.y) * 0.5f;
            }
            break;
        case VerticalAlignment::BOTTOM:
            // Align text to bottom of bounds
            if (params.bounds.y > 0.0f) {
                vertical_offset = params.bounds.y - layout.total_size.y;
            }
            break;
        case VerticalAlignment::BASELINE:
            // Position at baseline (mainly for single line text)
            vertical_offset = 0.0f;
            break;
        default: // TOP
            // Text starts at the top, no additional offset needed
            vertical_offset = 0.0f;
            break;
    }
    
    // Render each line
    for (const auto& line : layout.lines) {
        if (line.text.empty()) continue;

        float line_x = position.x + line.x_offset;
        // Fix baseline calculation - position.y should be the baseline position
        // For proper text rendering, we need to position relative to the baseline
        // The baseline is the reference line where most characters sit
        float baseline_y = position.y + vertical_offset + line.y_offset;
        
        // Calculate justification spacing if needed
        float extra_space_per_char = 0.0f;
        if (params.horizontal_align == TextAlignment::JUSTIFY && 
            params.bounds.x > 0.0f && 
            line.text.find(' ') != std::string::npos &&
            &line != &layout.lines.back()) { // Don't justify last line
            
            int space_count = static_cast<int>(std::count(line.text.begin(), line.text.end(), ' '));
            if (space_count > 0) {
                extra_space_per_char = (params.bounds.x - line.width) / space_count;
            }
        }
        
        // Render each character in the line
        float current_x = line_x;
        for (size_t char_idx = 0; char_idx < line.text.length(); ++char_idx) {
            char c = line.text[char_idx];
            
            auto glyph_it = glyphs.find(c);
            if (glyph_it == glyphs.end()) {
                continue;
            }
            
            const Glyph& glyph = glyph_it->second;
            
            // Calculate glyph position
            // bearing.x is the horizontal offset from the current position to the left edge of the glyph
            // This can be negative for characters like 'A' that overhang to the left
            float glyph_x = current_x + (glyph.bearing.x * scale_factor);

            // bearing.y (maxy) is the vertical offset from baseline to top of glyph (positive = above baseline)
            // baseline_to_bottom (miny) is the vertical offset from baseline to bottom of glyph (negative for descenders)
            // The glyph texture spans from miny to maxy vertically
            // In OpenGL coordinate system (Y up), we position the texture's bottom-left corner
            // If the texture bottom is at glyph_y, then baseline is at glyph_y - miny
            // So to place baseline at baseline_y: glyph_y = baseline_y + miny
            // For descenders, add a small adjustment to better align the main body
            float descender_adjustment = 0.0f;
            if (glyph.baseline_to_bottom < -2) { // Significant descender
                descender_adjustment = -glyph.baseline_to_bottom * 0.25f * scale_factor; // 25% of descender depth
            }
            float glyph_y = baseline_y + (glyph.baseline_to_bottom * scale_factor) + descender_adjustment;

            // Round positions to integers for pixel-perfect rendering
            glyph_x = std::round(glyph_x);
            glyph_y = std::round(glyph_y);

            // Render the glyph
            RenderGlyph(glyph, glm::vec2(glyph_x, glyph_y), scale_factor, params.color);
            
            // Advance to next character
            // The advance value is in 26.6 fixed point format, so we need to divide by 64
            float advance = (glyph.advance >> 6) * scale_factor;
            float kerning_adjustment = params.kerning;

            // Add kerning between character pairs
            if (char_idx + 1 < line.text.length()) {
                char next_char = line.text[char_idx + 1];
                float pair_kerning = GetKerning(c, next_char, params.font_path, params.font_size);
                kerning_adjustment += pair_kerning;
            }

            // Add justification spacing for spaces
            if (c == ' ' && extra_space_per_char > 0.0f) {
                kerning_adjustment += extra_space_per_char;
            }

            current_x += advance + kerning_adjustment;
        }
    }
}

void TextRenderer::RenderText(const TextRenderParams& params, const glm::vec2& position) {
    TextLayout layout = CalculateLayout(params);
    RenderLayout(layout, position, params);
}

glm::vec2 TextRenderer::CalculateTextSize(const TextRenderParams& params) {
    TextLayout layout = CalculateLayout(params);
    return layout.total_size;
}

float TextRenderer::GetKerning(char left, char right, const FontPath& font_path, int font_size) {
    Font font = ResourceManager::GetFont(font_path, font_size);
    if (!font.IsValid()) {
        return 0.0f;
    }
    
    TTF_Font* ttf_font = static_cast<TTF_Font*>(font.font_data);
    if (!ttf_font || !TTF_GetFontKerning(ttf_font)) {
        return 0.0f;
    }
    
    int kerning = TTF_GetFontKerningSizeGlyphs32(ttf_font, static_cast<Uint32>(left), static_cast<Uint32>(right));
    float scale_factor = GetContextAwareScaleFactor(font);
    return kerning * scale_factor;
}

void TextRenderer::ClearCache() {
    s_layout_cache.clear();
}

float TextRenderer::CalculateLineWidth(const std::string& text, const Font& font,
                                      const std::unordered_map<char, Glyph>& glyphs, float kerning) {
    if (text.empty()) {
        return 0.0f;
    }
    
    float scale_factor = GetContextAwareScaleFactor(font);
    float width = 0.0f;
    
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        auto glyph_it = glyphs.find(c);
        if (glyph_it != glyphs.end()) {
            float advance = (glyph_it->second.advance >> 6) * scale_factor;
            float kerning_adjustment = kerning;
            
            // Add kerning between character pairs
            if (i + 1 < text.length()) {
                char next_char = text[i + 1];
                TTF_Font* ttf_font = static_cast<TTF_Font*>(font.font_data);
                if (ttf_font && TTF_GetFontKerning(ttf_font)) {
                    int kern = TTF_GetFontKerningSizeGlyphs32(ttf_font, static_cast<Uint32>(c), static_cast<Uint32>(next_char));
                    kerning_adjustment += kern * scale_factor;
                }
            }
            
            width += advance + kerning_adjustment;
        }
    }
    
    return width;
}

std::vector<std::string> TextRenderer::SplitTextIntoLines(const std::string& text, const Font& font,
                                                         const std::unordered_map<char, Glyph>& glyphs,
                                                         const glm::vec2& bounds, bool word_wrap, float kerning) {
    std::vector<std::string> lines;

    if (text.empty()) {
        return lines;
    }

    // If no bounds or word wrap disabled, split only on newlines
    if (bounds.x <= 0.0f || !word_wrap) {
        // Split only on explicit newline characters
        size_t start = 0;
        size_t pos = 0;
        while ((pos = text.find('\n', start)) != std::string::npos) {
            lines.push_back(text.substr(start, pos - start));
            start = pos + 1;
        }
        // Add the last line (or the entire text if no newlines)
        lines.push_back(text.substr(start));

        // Remove empty line at the end if text ends with newline
        if (!lines.empty() && lines.back().empty() && text.back() == '\n') {
            lines.pop_back();
        }

        // Ensure we have at least one line
        if (lines.empty()) {
            lines.push_back("");
        }
        return lines;
    }

    // Word wrap enabled - split text respecting word boundaries
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line, '\n')) {
        if (line.empty()) {
            lines.push_back("");
            continue;
        }

        std::string current_line;
        std::stringstream word_stream(line);
        std::string word;

        while (word_stream >> word) {
            std::string test_line = current_line.empty() ? word : current_line + " " + word;
            float test_width = CalculateLineWidth(test_line, font, glyphs, kerning);

            if (test_width <= bounds.x) {
                current_line = test_line;
            } else {
                // Word doesn't fit, start new line
                if (!current_line.empty()) {
                    lines.push_back(current_line);
                    current_line = word;
                } else {
                    // Single word is too long, force it on its own line
                    lines.push_back(word);
                }
            }
        }

        if (!current_line.empty()) {
            lines.push_back(current_line);
        }
    }

    return lines;
}

void TextRenderer::RenderGlyph(const Glyph& glyph, const glm::vec2& position, float scale, const glm::vec4& color) {
    if (s_batching_enabled) {
        // Add to current batch instead of immediate rendering
        GlyphInstance instance;
        instance.transform = glm::mat4(1.0f);
        instance.transform = glm::translate(instance.transform, glm::vec3(position.x, position.y, 0.0f));
        instance.transform = glm::scale(instance.transform, glm::vec3(glyph.size.x * scale, glyph.size.y * scale, 1.0f));
        instance.color = color;
        instance.texture_id = glyph.texture_id;
        s_current_batch.glyphs.push_back(instance);
    } else {
        // Immediate rendering
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(position.x, position.y, 0.0f));
        transform = glm::scale(transform, glm::vec3(glyph.size.x * scale, glyph.size.y * scale, 1.0f));

        // Render using the high-quality text shader
        Renderer::RenderTextGlyph(transform, color, glyph.texture_id);
    }
}

std::string TextRenderer::GenerateCacheKey(const TextRenderParams& params) {
    std::stringstream ss;
    ss << params.text << "|"
       << params.font_path.path << "|"
       << params.font_path.is_system_font << "|"
       << params.font_path.style_name << "|"
       << params.font_size << "|"
       << static_cast<int>(params.horizontal_align) << "|"
       << static_cast<int>(params.vertical_align) << "|"
       << params.line_spacing << "|"
       << params.kerning << "|"
       << params.bounds.x << "|"
       << params.bounds.y << "|"
       << params.word_wrap;
    return ss.str();
}

glm::vec3 TextRenderer::GetFontMetrics(const FontPath& font_path, int font_size) {
    Font font = ResourceManager::GetFont(font_path, font_size);
    if (!font.IsValid()) {
        return glm::vec3(0.0f);
    }

    float scale_factor = GetContextAwareScaleFactor(font);
    return glm::vec3(
        font.ascent * scale_factor,   // x = ascent
        font.descent * scale_factor,  // y = descent
        font.height * scale_factor    // z = line height
    );
}

glm::vec2 TextRenderer::MeasureText(const std::string& text, const FontPath& font_path, int font_size, float kerning) {
    if (text.empty()) {
        return glm::vec2(0.0f);
    }

    Font font = ResourceManager::GetFont(font_path, font_size);
    if (!font.IsValid()) {
        return glm::vec2(0.0f);
    }

    auto glyphs = ResourceManager::GenerateGlyphAtlas(font);
    if (glyphs.empty()) {
        return glm::vec2(0.0f);
    }

    float scale_factor = GetContextAwareScaleFactor(font);
    float max_width = 0.0f;
    float current_width = 0.0f;
    int lines = 1;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];

        if (c == '\n') {
            max_width = std::max(max_width, current_width);
            current_width = 0.0f;
            lines++;
            continue;
        }

        auto glyph_it = glyphs.find(c);
        if (glyph_it != glyphs.end()) {
            float advance = (glyph_it->second.advance >> 6) * scale_factor;
            float kerning_adjustment = kerning;

            // Add kerning between character pairs
            if (i + 1 < text.length() && text[i + 1] != '\n') {
                char next_char = text[i + 1];
                kerning_adjustment += GetKerning(c, next_char, font_path, font_size);
            }

            current_width += advance + kerning_adjustment;
        }
    }

    max_width = std::max(max_width, current_width);
    float height = lines * font.height * scale_factor;

    return glm::vec2(max_width, height);
}

bool TextRenderer::IsLayoutCached(const TextRenderParams& params) {
    std::string cache_key = GenerateCacheKey(params);
    return s_layout_cache.find(cache_key) != s_layout_cache.end();
}

void TextRenderer::BeginBatch(const FontPath& font_path, int font_size) {
    // Flush any existing batch
    if (s_batching_enabled) {
        FlushBatch();
    }

    s_current_batch.Clear();
    s_current_batch.font_path = font_path;
    s_current_batch.font_size = font_size;
    s_batching_enabled = true;
}

void TextRenderer::AddToBatch(const TextRenderParams& params, const glm::vec2& position) {
    if (!s_batching_enabled) {
        // Fall back to immediate rendering
        RenderText(params, position);
        return;
    }

    // Check if font matches current batch
    if (params.font_path.path != s_current_batch.font_path.path ||
        params.font_size != s_current_batch.font_size) {
        // Flush current batch and start new one
        FlushBatch();
        s_current_batch.font_path = params.font_path;
        s_current_batch.font_size = params.font_size;
    }

    // Calculate layout and add glyphs to batch
    TextLayout layout = CalculateLayout(params);
    if (!layout.is_valid || layout.lines.empty()) {
        return;
    }

    // Load font and generate glyph atlas
    Font font = ResourceManager::GetFont(params.font_path, params.font_size);
    if (!font.IsValid()) {
        return;
    }

    auto glyphs = ResourceManager::GenerateGlyphAtlas(font);
    if (glyphs.empty()) {
        return;
    }

    float scale_factor = GetContextAwareScaleFactor(font);

    // Calculate vertical alignment offset (consistent with RenderLayout)
    float vertical_offset = 0.0f;
    switch (params.vertical_align) {
        case VerticalAlignment::CENTER:
            // Center the text block vertically within the bounds
            if (params.bounds.y > 0.0f) {
                vertical_offset = (params.bounds.y - layout.total_size.y) * 0.5f;
            }
            break;
        case VerticalAlignment::BOTTOM:
            // Align text to bottom of bounds
            if (params.bounds.y > 0.0f) {
                vertical_offset = params.bounds.y - layout.total_size.y;
            }
            break;
        case VerticalAlignment::BASELINE:
            // Position at baseline (mainly for single line text)
            vertical_offset = 0.0f;
            break;
        default: // TOP
            // Text starts at the top, no additional offset needed
            vertical_offset = 0.0f;
            break;
    }

    // Add each glyph to the batch
    for (const auto& line : layout.lines) {
        if (line.text.empty()) continue;

        float line_x = position.x + line.x_offset;
        float baseline_y = position.y + line.y_offset + vertical_offset;

        // Calculate justification spacing if needed
        float extra_space_per_char = 0.0f;
        if (params.horizontal_align == TextAlignment::JUSTIFY &&
            params.bounds.x > 0.0f &&
            line.text.find(' ') != std::string::npos &&
            &line != &layout.lines.back()) {

            int space_count = static_cast<int>(std::count(line.text.begin(), line.text.end(), ' '));
            if (space_count > 0) {
                extra_space_per_char = (params.bounds.x - line.width) / space_count;
            }
        }

        // Add each character to the batch
        float current_x = line_x;
        for (size_t char_idx = 0; char_idx < line.text.length(); ++char_idx) {
            char c = line.text[char_idx];

            auto glyph_it = glyphs.find(c);
            if (glyph_it == glyphs.end()) {
                continue;
            }

            const Glyph& glyph = glyph_it->second;

            // Calculate glyph position (consistent with RenderLayout)
            float glyph_x = current_x + (glyph.bearing.x * scale_factor);
            // Apply same descender adjustment as in RenderLayout
            float descender_adjustment = 0.0f;
            if (glyph.baseline_to_bottom < -2) { // Significant descender
                descender_adjustment = -glyph.baseline_to_bottom * 0.25f * scale_factor; // 25% of descender depth
            }
            float glyph_y = baseline_y + (glyph.baseline_to_bottom * scale_factor) + descender_adjustment;

            // Round positions to integers for pixel-perfect rendering
            glyph_x = std::round(glyph_x);
            glyph_y = std::round(glyph_y);

            // Create glyph instance
            GlyphInstance instance;
            instance.transform = glm::mat4(1.0f);
            instance.transform = glm::translate(instance.transform, glm::vec3(glyph_x, glyph_y, 0.0f));
            instance.transform = glm::scale(instance.transform, glm::vec3(glyph.size.x * scale_factor, glyph.size.y * scale_factor, 1.0f));
            instance.color = params.color;
            instance.texture_id = glyph.texture_id;

            s_current_batch.glyphs.push_back(instance);

            // Advance to next character
            float advance = (glyph.advance >> 6) * scale_factor;
            float kerning_adjustment = params.kerning;

            // Add kerning between character pairs
            if (char_idx + 1 < line.text.length()) {
                char next_char = line.text[char_idx + 1];
                kerning_adjustment += GetKerning(c, next_char, params.font_path, params.font_size);
            }

            // Add justification spacing for spaces
            if (c == ' ' && extra_space_per_char > 0.0f) {
                kerning_adjustment += extra_space_per_char;
            }

            current_x += advance + kerning_adjustment;
        }
    }
}

void TextRenderer::FlushBatch() {
    if (!s_batching_enabled || s_current_batch.IsEmpty()) {
        return;
    }

    // Group glyphs by texture for optimal rendering
    std::unordered_map<unsigned int, std::vector<GlyphInstance*>> texture_groups;
    for (auto& glyph : s_current_batch.glyphs) {
        texture_groups[glyph.texture_id].push_back(&glyph);
    }

    // Render each texture group to minimize texture binding
    for (const auto& group : texture_groups) {
        // Sort by color to potentially batch similar colors together
        std::vector<GlyphInstance*> sorted_glyphs = group.second;
        std::sort(sorted_glyphs.begin(), sorted_glyphs.end(),
                 [](const GlyphInstance* a, const GlyphInstance* b) {
                     // Simple color comparison for batching
                     return (a->color.r + a->color.g + a->color.b + a->color.a) <
                            (b->color.r + b->color.g + b->color.b + b->color.a);
                 });

        for (const auto& glyph : sorted_glyphs) {
            Renderer::RenderTextGlyph(glyph->transform, glyph->color, glyph->texture_id);
        }
    }

    s_current_batch.Clear();
}

void TextRenderer::EndBatch() {
    if (s_batching_enabled) {
        FlushBatch();
        s_batching_enabled = false;
    }
}

void TextRenderer::PrewarmGlyphAtlas(const FontPath& font_path, int font_size, const std::string& characters) {
    Font font = ResourceManager::GetFont(font_path, font_size);
    if (!font.IsValid()) {
        return;
    }

    // Use provided characters or default common set
    std::string chars_to_load = characters;
    if (chars_to_load.empty()) {
        // Common English characters, numbers, and punctuation
        chars_to_load = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
                       " .,!?;:'\"-()[]{}@#$%^&*+=<>/\\|`~_";
    }

    // Pre-generate glyph atlas with these characters
    auto glyphs = ResourceManager::GenerateGlyphAtlas(font);

    std::cout << "TextRenderer: Pre-warmed glyph atlas for font " << font_path.path
              << " (size: " << font_size << ") with " << chars_to_load.length()
              << " characters, generated " << glyphs.size() << " glyphs" << std::endl;
}

std::string TextRenderer::GetCacheStats() {
    std::stringstream ss;
    ss << "TextRenderer Cache Statistics:\n";
    ss << "  Layout cache entries: " << s_layout_cache.size() << "/" << MAX_CACHE_SIZE << "\n";
    ss << "  Batching enabled: " << (s_batching_enabled ? "Yes" : "No") << "\n";
    if (s_batching_enabled) {
        ss << "  Current batch size: " << s_current_batch.Size() << " glyphs\n";
        ss << "  Current batch font: " << s_current_batch.font_path.path
           << " (size: " << s_current_batch.font_size << ")\n";
    }
    return ss.str();
}

void TextRenderer::DebugTextLayout(const TextRenderParams& params, const glm::vec2& position) {
    std::cout << "=== TextRenderer Debug Layout ===" << std::endl;
    std::cout << "Text: \"" << params.text << "\"" << std::endl;
    std::cout << "Font: " << params.font_path.path << " (size: " << params.font_size << ")" << std::endl;
    std::cout << "Position: (" << position.x << ", " << position.y << ")" << std::endl;
    std::cout << "Bounds: (" << params.bounds.x << ", " << params.bounds.y << ")" << std::endl;
    std::cout << "Word wrap: " << (params.word_wrap ? "Yes" : "No") << std::endl;

    TextLayout layout = CalculateLayout(params);
    std::cout << "Layout valid: " << (layout.is_valid ? "Yes" : "No") << std::endl;
    std::cout << "Total size: (" << layout.total_size.x << ", " << layout.total_size.y << ")" << std::endl;
    std::cout << "Line height: " << layout.line_height << std::endl;
    std::cout << "Number of lines: " << layout.lines.size() << std::endl;

    for (size_t i = 0; i < layout.lines.size(); ++i) {
        const auto& line = layout.lines[i];
        std::cout << "Line " << i << ": \"" << line.text << "\" (width: " << line.width
                  << ", x_offset: " << line.x_offset << ", y_offset: " << line.y_offset << ")" << std::endl;
    }
    std::cout << "=================================" << std::endl;
}

float TextRenderer::GetContextAwareScaleFactor(const Font& font) {
    // Get the current rendering context
    RenderingContext context = Renderer::GetRenderingContext();

    // Debug logging to understand what's happening
    std::cout << "GetContextAwareScaleFactor - Context: " << (int)context
              << ", font.dpi_scale: " << font.dpi_scale
              << ", font.size: " << font.size
              << ", font.scaled_size: " << font.scaled_size << std::endl;

    float scale_factor = 1.0f;

    if (context == RenderingContext::Runtime) {
        // Runtime: Use standard DPI compensation
        scale_factor = 1.0f / font.dpi_scale;
        std::cout << "Runtime scale_factor: " << scale_factor << std::endl;
    } else {
        // Editor: The problem might be that we need to completely disable DPI scaling for editor
        // since Qt already handles it, or we need a different approach
        scale_factor = 1.0f; // Try no DPI compensation for editor
        std::cout << "Editor scale_factor: " << scale_factor << std::endl;
    }

    return scale_factor;
}

} // namespace Lupine
