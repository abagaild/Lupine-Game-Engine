#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace Lupine {

// Forward declaration
struct FontPath;

/**
 * @brief Texture filtering modes
 */
enum class TextureFilter {
    Nearest,    // GL_NEAREST
    Bilinear,   // GL_LINEAR
    Bicubic     // GL_LINEAR with special handling
};

/**
 * @brief Texture resource structure
 */
struct Texture {
    unsigned int id = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
    std::string path;

    bool IsValid() const { return id != 0; }
};

/**
 * @brief Font resource structure
 */
struct Font {
    void* font_data = nullptr;  // TTF_Font* or similar
    int size = 0;               // Original requested size
    int scaled_size = 0;        // DPI-scaled size used for rendering
    std::string path;
    std::string family_name;    // Font family name (e.g., "Arial", "Times New Roman")
    std::string style_name;     // Font style (e.g., "Regular", "Bold", "Italic")
    bool is_system_font = false; // True if this is a system font

    // Font metrics for proper text alignment (in DPI-scaled units)
    int ascent = 0;             // Distance from baseline to top
    int descent = 0;            // Distance from baseline to bottom (negative)
    int line_skip = 0;          // Recommended line spacing
    int height = 0;             // Total font height

    // DPI scaling information
    float dpi_scale = 1.0f;     // DPI scale factor used for this font

    bool IsValid() const { return font_data != nullptr; }
};

/**
 * @brief System font information
 */
struct SystemFont {
    std::string family_name;    // Font family name
    std::string style_name;     // Font style
    std::string file_path;      // Full path to font file
    bool is_bold = false;
    bool is_italic = false;

    std::string GetDisplayName() const {
        if (style_name == "Regular" || style_name.empty()) {
            return family_name;
        }
        return family_name + " " + style_name;
    }
};

/**
 * @brief Glyph information for text rendering
 */
struct Glyph {
    unsigned int texture_id = 0;
    glm::ivec2 size;
    glm::ivec2 bearing;         // x = horizontal offset, y = distance from baseline to top (maxy)
    int baseline_to_bottom = 0; // Distance from baseline to bottom of glyph (miny, usually negative)
    unsigned int advance = 0;
    float dpi_scale = 1.0f;     // DPI scale factor for this glyph
};

/**
 * @brief Resource manager for loading and caching textures, fonts, and other assets
 */
class ResourceManager {
public:
    /**
     * @brief Initialize the resource manager
     * @return True if successful
     */
    static bool Initialize();

    /**
     * @brief Shutdown the resource manager
     */
    static void Shutdown();

    /**
     * @brief Load a texture from file
     * @param path Path to texture file
     * @param flip_vertically Whether to flip texture vertically (default: true for OpenGL)
     * @return Texture resource
     */
    static Texture LoadTexture(const std::string& path, bool flip_vertically = true);

    /**
     * @brief Set global texture filter mode
     * @param filter Texture filter mode to use for new textures
     */
    static void SetTextureFilter(TextureFilter filter);

    /**
     * @brief Get current global texture filter mode
     * @return Current texture filter mode
     */
    static TextureFilter GetTextureFilter();

    /**
     * @brief Load a font from file
     * @param path Path to font file
     * @param size Font size in pixels
     * @return Font resource
     */
    static Font LoadFont(const std::string& path, int size);

    /**
     * @brief Load a font using FontPath (supports both system fonts and file paths)
     * @param font_path FontPath structure containing font information
     * @param size Font size in pixels
     * @return Font resource
     */
    static Font LoadFont(const FontPath& font_path, int size);

    /**
     * @brief Get cached texture
     * @param path Path to texture file
     * @return Texture resource or invalid texture if not found
     */
    static Texture GetTexture(const std::string& path);

    /**
     * @brief Get cached font
     * @param path Path to font file
     * @param size Font size
     * @return Font resource or invalid font if not found
     */
    static Font GetFont(const std::string& path, int size);

    /**
     * @brief Get cached font using FontPath
     * @param font_path FontPath structure
     * @param size Font size
     * @return Font resource or invalid font if not found
     */
    static Font GetFont(const FontPath& font_path, int size);

    /**
     * @brief Generate glyph atlas for a font
     * @param font Font to generate atlas for
     * @return Map of character to glyph info
     */
    static std::unordered_map<char, Glyph> GenerateGlyphAtlas(const Font& font);

    /**
     * @brief Enumerate all available system fonts
     * @return Vector of system font information
     */
    static std::vector<SystemFont> EnumerateSystemFonts();

    /**
     * @brief Get cached system fonts (call EnumerateSystemFonts first)
     * @return Vector of system font information
     */
    static const std::vector<SystemFont>& GetSystemFonts();

    /**
     * @brief Load a system font by family name
     * @param family_name Font family name (e.g., "Arial")
     * @param size Font size in pixels
     * @param style_name Optional style name (e.g., "Bold", "Italic")
     * @return Font resource
     */
    static Font LoadSystemFont(const std::string& family_name, int size, const std::string& style_name = "Regular");

    /**
     * @brief Find system font by family name
     * @param family_name Font family name to search for
     * @param style_name Optional style name
     * @return Pointer to SystemFont if found, nullptr otherwise
     */
    static const SystemFont* FindSystemFont(const std::string& family_name, const std::string& style_name = "Regular");

    /**
     * @brief Clear all cached resources
     */
    static void ClearCache();

    /**
     * @brief Check if resource manager is initialized
     * @return True if initialized
     */
    static bool IsInitialized() { return s_initialized; }

private:
    static bool s_initialized;
    static TextureFilter s_texture_filter;
    static std::unordered_map<std::string, Texture> s_texture_cache;
    static std::unordered_map<std::string, Font> s_font_cache;
    static std::unordered_map<std::string, std::unordered_map<char, Glyph>> s_glyph_cache;
    static std::vector<SystemFont> s_system_fonts;
    static bool s_system_fonts_enumerated;

    /**
     * @brief Create OpenGL texture from image data
     * @param data Image data
     * @param width Image width
     * @param height Image height
     * @param channels Number of channels
     * @return OpenGL texture ID
     */
    static unsigned int CreateGLTexture(unsigned char* data, int width, int height, int channels);

    /**
     * @brief Create OpenGL texture optimized for glyph rendering
     * @param data RGBA pixel data
     * @param width Texture width
     * @param height Texture height
     * @return OpenGL texture ID
     */
    static unsigned int CreateGlyphTexture(unsigned char* data, int width, int height);

    /**
     * @brief Create high-quality OpenGL texture optimized for glyph rendering
     * @param data RGBA pixel data
     * @param width Texture width
     * @param height Texture height
     * @param pitch Row stride in bytes (0 = tightly packed)
     * @return OpenGL texture ID
     */
    static unsigned int CreateHighQualityGlyphTexture(unsigned char* data, int width, int height, int pitch = 0);

    /**
     * @brief Generate cache key for font
     * @param path Font path
     * @param size Font size
     * @return Cache key
     */
    static std::string GetFontCacheKey(const std::string& path, int size);

    /**
     * @brief Generate cache key for system font
     * @param family_name Font family name
     * @param style_name Font style name
     * @param size Font size
     * @return Cache key
     */
    static std::string GetSystemFontCacheKey(const std::string& family_name, const std::string& style_name, int size);

    /**
     * @brief Get current DPI scale factor
     * @return DPI scale factor (1.0 = 96 DPI, 2.0 = 192 DPI, etc.)
     */
    static float GetDPIScale();

    /**
     * @brief Set DPI scale factor override
     * @param scale DPI scale factor (1.0 = 96 DPI, 2.0 = 192 DPI, etc.)
     */
    static void SetDPIScale(float scale);

    /**
     * @brief Platform-specific system font enumeration
     */
    static void EnumerateSystemFontsImpl();

    /**
     * @brief Extract font metadata from file
     * @param file_path Path to font file
     * @return SystemFont with metadata, or empty if failed
     */
    static SystemFont ExtractFontMetadata(const std::string& file_path);
};

} // namespace Lupine
