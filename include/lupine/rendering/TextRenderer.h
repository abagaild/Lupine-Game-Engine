#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "lupine/core/Component.h"
#include "lupine/resources/ResourceManager.h"

// Forward declarations to avoid circular dependencies
struct FontPath;

namespace Lupine {

// Forward declarations
class GraphicsTexture;

/**
 * @brief Glyph instance for batched rendering
 */
struct GlyphInstance {
    glm::mat4 transform;
    glm::vec4 color;
    std::shared_ptr<GraphicsTexture> texture;
};

/**
 * @brief Text batch for optimized rendering
 */
struct TextBatch {
    std::vector<GlyphInstance> glyphs;
    FontPath font_path;
    int font_size;

    void Clear() { glyphs.clear(); }
    bool IsEmpty() const { return glyphs.empty(); }
    size_t Size() const { return glyphs.size(); }
};

/**
 * @brief Text alignment options
 */
enum class TextAlignment {
    LEFT,
    CENTER,
    RIGHT,
    JUSTIFY
};

/**
 * @brief Vertical text alignment options
 */
enum class VerticalAlignment {
    TOP,
    CENTER,
    BOTTOM,
    BASELINE
};

/**
 * @brief Text layout information for a single line
 */
struct TextLine {
    std::string text;
    float width = 0.0f;
    float x_offset = 0.0f;  // Horizontal offset for alignment
    float y_offset = 0.0f;  // Vertical offset from baseline
};

/**
 * @brief Complete text layout information
 */
struct TextLayout {
    std::vector<TextLine> lines;
    glm::vec2 total_size = glm::vec2(0.0f);
    float line_height = 0.0f;
    bool is_valid = false;
    
    // Cache key for layout invalidation
    std::string cache_key;
};

/**
 * @brief Text rendering parameters
 */
struct TextRenderParams {
    std::string text;
    FontPath font_path;
    int font_size = 16;
    glm::vec4 color = glm::vec4(1.0f);
    TextAlignment horizontal_align = TextAlignment::LEFT;
    VerticalAlignment vertical_align = VerticalAlignment::TOP;
    float line_spacing = 1.0f;
    float kerning = 0.0f;
    glm::vec2 bounds = glm::vec2(0.0f);  // 0 means no bounds
    bool word_wrap = false;
};

/**
 * @brief Unified text rendering utility class
 * 
 * This class consolidates all text rendering functionality from Button and Label components,
 * providing a streamlined interface for high-quality text rendering with proper kerning,
 * alignment, and multi-line support.
 */
class TextRenderer {
public:
    /**
     * @brief Calculate text layout for given parameters
     * @param params Text rendering parameters
     * @return Complete text layout information
     */
    static TextLayout CalculateLayout(const TextRenderParams& params);
    
    /**
     * @brief Render text using pre-calculated layout
     * @param layout Pre-calculated text layout
     * @param position Base position for rendering
     * @param params Original text parameters
     */
    static void RenderLayout(const TextLayout& layout, const glm::vec2& position, const TextRenderParams& params);
    
    /**
     * @brief Render text directly (convenience method)
     * @param params Text rendering parameters
     * @param position Position to render at
     */
    static void RenderText(const TextRenderParams& params, const glm::vec2& position);
    
    /**
     * @brief Calculate text size without rendering
     * @param params Text rendering parameters
     * @return Text size in pixels
     */
    static glm::vec2 CalculateTextSize(const TextRenderParams& params);
    
    /**
     * @brief Get kerning between two characters
     * @param left Left character
     * @param right Right character
     * @param font_path Font to use
     * @param font_size Font size
     * @return Kerning adjustment in pixels
     */
    static float GetKerning(char left, char right, const FontPath& font_path, int font_size);
    
    /**
     * @brief Clear cached layouts (call when fonts change)
     */
    static void ClearCache();

    /**
     * @brief Get text metrics for a font
     * @param font_path Font to use
     * @param font_size Font size
     * @return Font metrics (ascent, descent, line height)
     */
    static glm::vec3 GetFontMetrics(const FontPath& font_path, int font_size);

    /**
     * @brief Measure text without full layout calculation (faster for simple cases)
     * @param text Text to measure
     * @param font_path Font to use
     * @param font_size Font size
     * @param kerning Additional kerning
     * @return Text size in pixels
     */
    static glm::vec2 MeasureText(const std::string& text, const FontPath& font_path, int font_size, float kerning = 0.0f);

    /**
     * @brief Check if a layout is cached
     * @param params Text parameters
     * @return True if layout is cached
     */
    static bool IsLayoutCached(const TextRenderParams& params);

    /**
     * @brief Begin a text batch for optimized rendering
     * @param font_path Font to use for this batch
     * @param font_size Font size for this batch
     */
    static void BeginBatch(const FontPath& font_path, int font_size);

    /**
     * @brief Add text to the current batch
     * @param params Text rendering parameters
     * @param position Position to render at
     */
    static void AddToBatch(const TextRenderParams& params, const glm::vec2& position);

    /**
     * @brief Render and clear the current batch
     */
    static void FlushBatch();

    /**
     * @brief End batching and render any remaining glyphs
     */
    static void EndBatch();

    /**
     * @brief Pre-warm glyph atlas for a font with common characters
     * @param font_path Font to pre-warm
     * @param font_size Font size
     * @param characters Characters to pre-load (empty for default set)
     */
    static void PrewarmGlyphAtlas(const FontPath& font_path, int font_size,
                                 const std::string& characters = "");

    /**
     * @brief Get cache statistics for debugging
     * @return String with cache information
     */
    static std::string GetCacheStats();

    /**
     * @brief Debug text rendering by printing layout information
     * @param params Text rendering parameters
     * @param position Position to render at
     */
    static void DebugTextLayout(const TextRenderParams& params, const glm::vec2& position);

    /**
     * @brief Get context-aware scale factor for text rendering
     * @param font Font to get scale factor for
     * @return Scale factor adjusted for current rendering context
     */
    static float GetContextAwareScaleFactor(const Font& font);

private:
    /**
     * @brief Calculate width of a text line
     * @param text Text to measure
     * @param font Font to use
     * @param glyphs Glyph atlas
     * @param kerning Additional kerning
     * @return Line width in pixels
     */
    static float CalculateLineWidth(const std::string& text, const Font& font, 
                                   const std::unordered_map<char, Glyph>& glyphs, float kerning);
    
    /**
     * @brief Split text into lines based on bounds and word wrap
     * @param text Text to split
     * @param font Font to use
     * @param glyphs Glyph atlas
     * @param bounds Bounding box (0 means no bounds)
     * @param word_wrap Whether to wrap words
     * @param kerning Additional kerning
     * @return Vector of text lines
     */
    static std::vector<std::string> SplitTextIntoLines(const std::string& text, const Font& font,
                                                      const std::unordered_map<char, Glyph>& glyphs,
                                                      const glm::vec2& bounds, bool word_wrap, float kerning);
    
    /**
     * @brief Render a single glyph
     * @param glyph Glyph to render
     * @param position Position to render at
     * @param scale Scale factor
     * @param color Text color
     */
    static void RenderGlyph(const Glyph& glyph, const glm::vec2& position, float scale, const glm::vec4& color);
    
    /**
     * @brief Generate cache key for layout
     * @param params Text parameters
     * @return Cache key string
     */
    static std::string GenerateCacheKey(const TextRenderParams& params);

    // Layout cache for performance
    static std::unordered_map<std::string, TextLayout> s_layout_cache;
    static const size_t MAX_CACHE_SIZE = 100;

    // Batching system for optimized rendering
    static TextBatch s_current_batch;
    static bool s_batching_enabled;
};

} // namespace Lupine
