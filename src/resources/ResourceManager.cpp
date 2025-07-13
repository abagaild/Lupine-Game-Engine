#include "lupine/resources/ResourceManager.h"
#include "lupine/core/Component.h" // For FontPath
#include "lupine/rendering/GraphicsDevice.h"
#include "lupine/rendering/GraphicsTexture.h"
#include "lupine/rendering/GraphicsDeviceFactory.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <set>
#include <cstdlib>
#include <cstring>

// Use stb_image for image loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// SDL2_ttf for font loading
#include <SDL.h>
#include <SDL_ttf.h>

// Platform-specific includes for font enumeration
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <wingdi.h>
    #undef min
    #undef max
#elif defined(__linux__)
    #include <fontconfig/fontconfig.h>
#elif defined(__APPLE__)
    #include <CoreText/CoreText.h>
    #include <CoreFoundation/CoreFoundation.h>
#endif

namespace Lupine {

// Static member definitions
bool ResourceManager::s_initialized = false;
TextureFilter ResourceManager::s_texture_filter = TextureFilter::Bilinear;
std::shared_ptr<GraphicsDevice> ResourceManager::s_graphics_device = nullptr;
std::unordered_map<std::string, Texture> ResourceManager::s_texture_cache;
std::unordered_map<std::string, Font> ResourceManager::s_font_cache;
std::unordered_map<std::string, std::unordered_map<char, Glyph>> ResourceManager::s_glyph_cache;
std::vector<SystemFont> ResourceManager::s_system_fonts;
bool ResourceManager::s_system_fonts_enumerated = false;

bool ResourceManager::Initialize(std::shared_ptr<GraphicsDevice> graphics_device) {
    if (s_initialized) {
        return true;
    }

    std::cout << "Initializing ResourceManager..." << std::endl;

    // Use provided graphics device or get the default one
    if (graphics_device) {
        s_graphics_device = graphics_device;
    } else {
        s_graphics_device = GraphicsDeviceFactory::GetDevice();
    }

    if (!s_graphics_device) {
        std::cerr << "ResourceManager: No graphics device available" << std::endl;
        return false;
    }

    // Initialize SDL_ttf for font loading
    if (TTF_Init() == -1) {
        std::cerr << "Failed to initialize SDL_ttf: " << TTF_GetError() << std::endl;
        return false;
    }

    s_initialized = true;
    std::cout << "ResourceManager initialized successfully!" << std::endl;
    return true;
}

void ResourceManager::Shutdown() {
    if (!s_initialized) {
        return;
    }

    std::cout << "Shutting down ResourceManager..." << std::endl;

    // Clear all caches
    ClearCache();

    // Shutdown SDL_ttf
    TTF_Quit();

    // Clear graphics device reference
    s_graphics_device = nullptr;

    s_initialized = false;
    std::cout << "ResourceManager shutdown complete." << std::endl;
}

Texture ResourceManager::LoadTexture(const std::string& path, bool flip_vertically) {
    // Check if already cached
    auto it = s_texture_cache.find(path);
    if (it != s_texture_cache.end()) {
        return it->second;
    }

    Texture texture;
    texture.path = path;

    // Check if file exists
    if (!std::filesystem::exists(path)) {
        std::cerr << "Texture file not found: " << path << std::endl;
        return texture;
    }

    // Set stb_image to flip vertically if requested
    stbi_set_flip_vertically_on_load(flip_vertically);

    // Load image data
    unsigned char* data = stbi_load(path.c_str(), &texture.width, &texture.height, &texture.channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << " - " << stbi_failure_reason() << std::endl;
        return texture;
    }

    // Create graphics texture
    texture.graphics_texture = CreateTexture(data, texture.width, texture.height, texture.channels);

    // Free image data
    stbi_image_free(data);

    if (texture.IsValid()) {
        // Cache the texture
        s_texture_cache[path] = texture;
        std::cout << "Loaded texture: " << path << " (" << texture.width << "x" << texture.height << ", " << texture.channels << " channels)" << std::endl;
    }

    return texture;
}

Font ResourceManager::LoadFont(const std::string& path, int size) {
    // Apply DPI scaling to font size for high-quality rendering
    float dpi_scale = GetDPIScale();
    int scaled_size = static_cast<int>(size * dpi_scale);

    std::string cache_key = GetFontCacheKey(path, scaled_size);

    // Check if already cached
    auto it = s_font_cache.find(cache_key);
    if (it != s_font_cache.end()) {
        return it->second;
    }

    Font font;
    font.path = path;
    font.size = size; // Store original size
    font.scaled_size = scaled_size; // Store DPI-scaled size

    // Check if file exists
    if (!std::filesystem::exists(path)) {
        std::cerr << "Font file not found: " << path << std::endl;
        return font;
    }

    // Load font using SDL_ttf with DPI-scaled size for better quality
    TTF_Font* ttf_font = TTF_OpenFont(path.c_str(), scaled_size);
    if (!ttf_font) {
        std::cerr << "Failed to load font: " << path << " - " << TTF_GetError() << std::endl;
        return font;
    }

    // Enable font hinting for better quality
    TTF_SetFontHinting(ttf_font, TTF_HINTING_NORMAL);

    // Enable kerning for better character spacing
    TTF_SetFontKerning(ttf_font, 1);

    font.font_data = ttf_font;

    // Get font metrics for proper text alignment (scaled values)
    font.ascent = TTF_FontAscent(ttf_font);
    font.descent = TTF_FontDescent(ttf_font);
    font.line_skip = TTF_FontLineSkip(ttf_font);
    font.height = TTF_FontHeight(ttf_font);

    // Store DPI scale for later use in rendering
    font.dpi_scale = dpi_scale;

    std::cout << "Font metrics (DPI scaled " << dpi_scale << "x) - ascent: " << font.ascent
              << ", descent: " << font.descent
              << ", height: " << font.height
              << ", line_skip: " << font.line_skip << std::endl;

    // Cache the font
    s_font_cache[cache_key] = font;
    std::cout << "Loaded font: " << path << " (size: " << size << ", scaled: " << scaled_size << ")" << std::endl;

    return font;
}

Font ResourceManager::LoadFont(const FontPath& font_path, int size) {
    std::cout << "ResourceManager: Loading font - is_system: " << font_path.is_system_font
              << ", path: '" << font_path.path << "', style: '" << font_path.style_name
              << "', size: " << size << std::endl;

    // Apply DPI scaling for all font loading
    float dpi_scale = GetDPIScale();
    int scaled_size = static_cast<int>(size * dpi_scale);

    if (font_path.is_system_font) {
        // Load system font with DPI scaling
        Font font = LoadSystemFont(font_path.path, scaled_size, font_path.style_name);
        if (font.IsValid()) {
            font.size = size; // Store original size
            font.scaled_size = scaled_size;
            font.dpi_scale = dpi_scale;
        } else {
            std::cout << "ResourceManager: System font failed, trying fallback" << std::endl;
            // Try fallback to default system font
            #ifdef _WIN32
            font = LoadSystemFont("Arial", scaled_size, "Regular");
            #elif defined(__APPLE__)
            font = LoadSystemFont("Helvetica", scaled_size, "Regular");
            #else
            font = LoadSystemFont("DejaVu Sans", scaled_size, "Regular");
            #endif
            if (font.IsValid()) {
                font.size = size;
                font.scaled_size = scaled_size;
                font.dpi_scale = dpi_scale;
            }
        }
        return font;
    } else {
        // Load font from file path
        if (font_path.path.empty()) {
            std::cout << "ResourceManager: Empty file path, using default system font" << std::endl;
            // Try to load default system font as fallback
            Font font;
            #ifdef _WIN32
            font = LoadSystemFont("Arial", scaled_size, "Regular");
            #elif defined(__APPLE__)
            font = LoadSystemFont("Helvetica", scaled_size, "Regular");
            #else
            font = LoadSystemFont("DejaVu Sans", scaled_size, "Regular");
            #endif
            if (font.IsValid()) {
                font.size = size;
                font.scaled_size = scaled_size;
                font.dpi_scale = dpi_scale;
            }
            return font;
        }
        return LoadFont(font_path.path, size);
    }
}

Texture ResourceManager::GetTexture(const std::string& path) {
    auto it = s_texture_cache.find(path);
    if (it != s_texture_cache.end()) {
        return it->second;
    }

    // Return invalid texture
    return Texture{};
}

Font ResourceManager::GetFont(const std::string& path, int size) {
    std::string cache_key = GetFontCacheKey(path, size);
    auto it = s_font_cache.find(cache_key);
    if (it != s_font_cache.end()) {
        return it->second;
    }

    // Return invalid font
    return Font{};
}

Font ResourceManager::GetFont(const FontPath& font_path, int size) {
    if (font_path.is_system_font) {
        // For system fonts, create a cache key based on family and style
        std::string cache_key = font_path.path + "_" + font_path.style_name + "_" + std::to_string(size);
        auto it = s_font_cache.find(cache_key);
        if (it != s_font_cache.end()) {
            return it->second;
        }

        // Not in cache, try to load it
        return LoadFont(font_path, size);
    } else {
        // For file fonts, use the regular GetFont method
        return GetFont(font_path.path, size);
    }
}

std::string ResourceManager::GetSystemFontCacheKey(const std::string& family_name, const std::string& style_name, int size) {
    return family_name + "_" + style_name + "_" + std::to_string(size);
}

std::unordered_map<char, Glyph> ResourceManager::GenerateGlyphAtlas(const Font& font) {
    if (!font.IsValid()) {
        std::cout << "ResourceManager: GenerateGlyphAtlas - Invalid font" << std::endl;
        return {};
    }

    std::string cache_key;
    if (font.is_system_font) {
        cache_key = GetSystemFontCacheKey(font.family_name, font.style_name, font.scaled_size);
    } else {
        cache_key = GetFontCacheKey(font.path, font.scaled_size);
    }
    std::cout << "ResourceManager: GenerateGlyphAtlas - cache_key: " << cache_key << std::endl;

    // Check if already cached
    auto it = s_glyph_cache.find(cache_key);
    if (it != s_glyph_cache.end()) {
        std::cout << "ResourceManager: Using cached glyph atlas with " << it->second.size() << " glyphs" << std::endl;
        return it->second;
    }

    std::unordered_map<char, Glyph> glyphs;
    TTF_Font* ttf_font = static_cast<TTF_Font*>(font.font_data);

    if (!ttf_font) {
        std::cout << "ResourceManager: TTF_Font is null!" << std::endl;
        return {};
    }

    std::cout << "ResourceManager: Generating high-quality glyph atlas for font (DPI scale: " << font.dpi_scale << ")..." << std::endl;

    // Generate glyphs for printable ASCII characters
    for (unsigned char c = 32; c < 127; c++) {
        // Use high-quality LCD rendering for better text quality
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface* surface = TTF_RenderGlyph_Blended(ttf_font, c, white);
        if (!surface) {
            std::cout << "Failed to render glyph for character: " << (char)c << std::endl;
            continue;
        }

        Glyph glyph;
        glyph.size = glm::ivec2(surface->w, surface->h);
        glyph.dpi_scale = font.dpi_scale; // Store DPI scale for rendering

        // Get glyph metrics
        int minx, maxx, miny, maxy, advance;
        if (TTF_GlyphMetrics(ttf_font, c, &minx, &maxx, &miny, &maxy, &advance) == 0) {
            // Store bearing as (horizontal_offset, vertical_offset_from_baseline)
            // minx = horizontal offset from pen position to left edge of glyph
            // maxy = vertical offset from baseline to top of glyph (positive upward)
            // miny = vertical offset from baseline to bottom of glyph (negative for descenders)
            glyph.bearing = glm::ivec2(minx, maxy);
            glyph.baseline_to_bottom = miny;
            // SDL_ttf returns advance in pixels, convert to 1/64th pixels for consistency with FreeType
            glyph.advance = advance << 6;



            // Debug output for problematic characters
            if (advance == 0 || advance < 0) {
                std::cerr << "Warning: Character '" << c << "' has invalid advance: " << advance << std::endl;
                // Set a default advance based on glyph width
                glyph.advance = (maxx - minx) << 6; // Convert to 1/64th pixels
            }
        } else {
            // If TTF_GlyphMetrics fails, set default values
            std::cerr << "Warning: Failed to get metrics for character '" << c << "'" << std::endl;
            glyph.bearing = glm::ivec2(0, surface->h);
            glyph.baseline_to_bottom = 0; // Assume no descender
            glyph.advance = surface->w << 6; // Use surface width as advance, convert to 1/64th pixels
        }

        // Create OpenGL texture from surface with improved quality
        if (surface->w > 0 && surface->h > 0) {
            // Convert surface to RGBA format for better alpha handling
            SDL_Surface* rgba_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
            if (rgba_surface) {
                // Debug: Check if we have valid pixel data
                if (rgba_surface->pixels) {
                    std::cout << "Creating glyph texture for '" << (char)c << "' - size: "
                              << rgba_surface->w << "x" << rgba_surface->h << std::endl;

                    // Create high-quality texture with proper filtering and pitch handling
                    glyph.graphics_texture = CreateHighQualityGlyphTexture(
                        static_cast<unsigned char*>(rgba_surface->pixels),
                        rgba_surface->w, rgba_surface->h, rgba_surface->pitch
                    );
                } else {
                    std::cout << "No pixel data for character: " << (char)c << std::endl;
                }
                SDL_FreeSurface(rgba_surface);
            } else {
                std::cout << "Failed to convert surface to RGBA for character: " << (char)c << std::endl;
            }
        }

        SDL_FreeSurface(surface);

        if (glyph.graphics_texture && glyph.graphics_texture->IsValid()) {
            glyphs[c] = glyph;
            std::cout << "Successfully created glyph for '" << (char)c << "' with texture handle: " << glyph.graphics_texture->GetNativeHandle() << std::endl;
        } else {
            std::cout << "Failed to create texture for glyph '" << (char)c << "'" << std::endl;
        }
    }

    // Cache the glyph atlas
    s_glyph_cache[cache_key] = glyphs;
    std::cout << "Generated glyph atlas for font: " << font.path << " (size: " << font.size << ", " << glyphs.size() << " glyphs)" << std::endl;

    return glyphs;
}

void ResourceManager::ClearCache() {
    // Clean up graphics textures (automatic cleanup via shared_ptr)
    s_texture_cache.clear();

    // Clean up glyph textures (automatic cleanup via shared_ptr)
    s_glyph_cache.clear();

    // Clean up fonts
    for (auto& pair : s_font_cache) {
        if (pair.second.font_data) {
            TTF_CloseFont(static_cast<TTF_Font*>(pair.second.font_data));
        }
    }
    s_font_cache.clear();

    std::cout << "ResourceManager cache cleared." << std::endl;
}

std::shared_ptr<GraphicsTexture> ResourceManager::CreateTexture(unsigned char* data, int width, int height, int channels) {
    if (!s_graphics_device) {
        std::cerr << "ResourceManager: No graphics device available for texture creation" << std::endl;
        return nullptr;
    }

    // Determine texture format based on channels
    TextureFormat format;
    if (channels == 1) {
        // Note: R8 format not available in current enum, using RGBA8 for now
        format = TextureFormat::RGBA8;
        // TODO: Add R8 format to TextureFormat enum
    } else if (channels == 3) {
        format = TextureFormat::RGB8;
    } else if (channels == 4) {
        format = TextureFormat::RGBA8;
    } else {
        std::cerr << "ResourceManager: Unsupported channel count: " << channels << std::endl;
        return nullptr;
    }

    // Create texture using graphics device
    auto texture = s_graphics_device->CreateTexture2D(width, height, format, data);

    if (texture && texture->IsValid()) {
        // Set texture parameters based on current filter setting
        TextureParameters params;
        params.wrap_s = TextureWrap::ClampToEdge;
        params.wrap_t = TextureWrap::ClampToEdge;
        params.generate_mipmaps = true;

        // Note: Filter settings will be handled by the backend-specific implementation
        // The TextureFilter enum from ResourceManager may need to be mapped to backend-specific values

        texture->SetParameters(params);
        texture->GenerateMipmaps();
    }

    return texture;
}

std::shared_ptr<GraphicsTexture> ResourceManager::CreateGlyphTexture(unsigned char* data, int width, int height) {
    if (!s_graphics_device) {
        std::cerr << "ResourceManager: No graphics device available for glyph texture creation" << std::endl;
        return nullptr;
    }

    // Glyph textures are always RGBA format
    auto texture = s_graphics_device->CreateTexture2D(width, height, TextureFormat::RGBA8, data);

    if (texture && texture->IsValid()) {
        // Set texture parameters optimized for text rendering
        TextureParameters params;
        params.wrap_s = TextureWrap::ClampToEdge;
        params.wrap_t = TextureWrap::ClampToEdge;
        params.generate_mipmaps = false; // Don't generate mipmaps for small glyph textures

        texture->SetParameters(params);
    }

    return texture;
}

std::shared_ptr<GraphicsTexture> ResourceManager::CreateHighQualityGlyphTexture(unsigned char* data, int width, int height, int pitch) {
    if (!s_graphics_device) {
        std::cerr << "ResourceManager: No graphics device available for high-quality glyph texture creation" << std::endl;
        return nullptr;
    }

    // For now, we'll create the texture with the data as-is
    // TODO: Handle pitch properly in the graphics abstraction layer
    // The pitch handling will need to be implemented in the backend-specific texture creation

    auto texture = s_graphics_device->CreateTexture2D(width, height, TextureFormat::RGBA8, data);

    if (texture && texture->IsValid()) {
        // Set texture parameters optimized for high-quality text rendering
        TextureParameters params;
        params.wrap_s = TextureWrap::ClampToEdge;
        params.wrap_t = TextureWrap::ClampToEdge;
        params.generate_mipmaps = true; // Generate mipmaps for better quality at different scales

        texture->SetParameters(params);
        texture->GenerateMipmaps();

        std::cout << "Created high-quality glyph texture (size: " << width << "x" << height << ")" << std::endl;
    }

    return texture;
}

// Static variable for DPI scale caching
static float s_cached_dpi_scale = -1.0f;

float ResourceManager::GetDPIScale() {

    if (s_cached_dpi_scale < 0.0f) {
        // Try to detect system DPI
        float detected_scale = 1.0f;

        #ifdef _WIN32
        // Windows DPI detection
        HDC screen = GetDC(0);
        if (screen) {
            int dpi_x = GetDeviceCaps(screen, LOGPIXELSX);
            ReleaseDC(0, screen);
            detected_scale = dpi_x / 96.0f; // 96 DPI is standard
        }
        #elif defined(__APPLE__)
        // macOS DPI detection - typically handled by the system
        // For now, assume Retina displays use 2.0x scaling
        detected_scale = 2.0f;
        #else
        // Linux DPI detection - can be complex, use environment variable or default
        const char* scale_env = getenv("GDK_SCALE");
        if (scale_env) {
            detected_scale = std::max(1.0f, std::min(4.0f, static_cast<float>(atof(scale_env))));
        } else {
            detected_scale = 1.0f;
        }
        #endif

        // Clamp to reasonable values
        s_cached_dpi_scale = std::max(1.0f, std::min(4.0f, detected_scale));

        std::cout << "Detected DPI scale: " << s_cached_dpi_scale << "x" << std::endl;
    }

    return s_cached_dpi_scale;
}

void ResourceManager::SetDPIScale(float scale) {
    s_cached_dpi_scale = std::max(1.0f, std::min(4.0f, scale));
    std::cout << "DPI scale manually set to: " << s_cached_dpi_scale << "x" << std::endl;

    // Clear font and glyph caches to force regeneration with new scale
    s_font_cache.clear();
    s_glyph_cache.clear();
}

void ResourceManager::SetTextureFilter(TextureFilter filter) {
    s_texture_filter = filter;
    std::cout << "Texture filter set to: ";
    switch (filter) {
        case TextureFilter::Nearest: std::cout << "Nearest"; break;
        case TextureFilter::Bilinear: std::cout << "Bilinear"; break;
        case TextureFilter::Bicubic: std::cout << "Bicubic"; break;
    }
    std::cout << std::endl;
}

TextureFilter ResourceManager::GetTextureFilter() {
    return s_texture_filter;
}

std::string ResourceManager::GetFontCacheKey(const std::string& path, int size) {
    return path + "_" + std::to_string(size);
}

std::vector<SystemFont> ResourceManager::EnumerateSystemFonts() {
    if (!s_system_fonts_enumerated) {
        std::cout << "ResourceManager: Enumerating system fonts..." << std::endl;
        EnumerateSystemFontsImpl();
        s_system_fonts_enumerated = true;
        std::cout << "ResourceManager: Found " << s_system_fonts.size() << " system fonts" << std::endl;
    }
    return s_system_fonts;
}

const std::vector<SystemFont>& ResourceManager::GetSystemFonts() {
    if (!s_system_fonts_enumerated) {
        EnumerateSystemFonts();
    }
    return s_system_fonts;
}

Font ResourceManager::LoadSystemFont(const std::string& family_name, int size, const std::string& style_name) {
    std::cout << "ResourceManager: LoadSystemFont - family: '" << family_name
              << "', style: '" << style_name << "', size: " << size << std::endl;

    // Check if already cached with system font cache key
    std::string system_cache_key = GetSystemFontCacheKey(family_name, style_name, size);
    auto it = s_font_cache.find(system_cache_key);
    if (it != s_font_cache.end()) {
        std::cout << "ResourceManager: Using cached system font" << std::endl;
        return it->second;
    }

    const SystemFont* system_font = FindSystemFont(family_name, style_name);
    if (system_font) {
        std::cout << "ResourceManager: Found system font at: " << system_font->file_path << std::endl;
        Font font = LoadFont(system_font->file_path, size);
        if (font.IsValid()) {
            font.family_name = system_font->family_name;
            font.style_name = system_font->style_name;
            font.is_system_font = true;

            // Cache with system font cache key
            s_font_cache[system_cache_key] = font;

            std::cout << "ResourceManager: Successfully loaded and cached system font" << std::endl;
        } else {
            std::cout << "ResourceManager: Failed to load system font from file" << std::endl;
        }
        return font;
    } else {
        std::cout << "ResourceManager: System font not found in enumerated list" << std::endl;
    }

    // Return invalid font if not found
    return Font{};
}

const SystemFont* ResourceManager::FindSystemFont(const std::string& family_name, const std::string& style_name) {
    if (!s_system_fonts_enumerated) {
        EnumerateSystemFonts();
    }

    for (const auto& font : s_system_fonts) {
        if (font.family_name == family_name && font.style_name == style_name) {
            return &font;
        }
    }

    // Try to find with "Regular" style if specific style not found
    if (style_name != "Regular") {
        for (const auto& font : s_system_fonts) {
            if (font.family_name == family_name && font.style_name == "Regular") {
                return &font;
            }
        }
    }

    return nullptr;
}

void ResourceManager::EnumerateSystemFontsImpl() {
    s_system_fonts.clear();

#ifdef _WIN32
    // Windows font enumeration
    std::set<std::string> processed_fonts; // To avoid duplicates

    // Common Windows font directories
    std::vector<std::string> font_dirs = {
        "C:/Windows/Fonts/",
        "C:/Windows/System32/Fonts/"
    };

    for (const auto& font_dir : font_dirs) {
        if (std::filesystem::exists(font_dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(font_dir)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    if (ext == ".ttf" || ext == ".otf" || ext == ".ttc") {
                        std::string file_path = entry.path().string();
                        if (processed_fonts.find(file_path) == processed_fonts.end()) {
                            SystemFont font = ExtractFontMetadata(file_path);
                            if (!font.family_name.empty()) {
                                s_system_fonts.push_back(font);
                                processed_fonts.insert(file_path);
                            }
                        }
                    }
                }
            }
        }
    }

#elif defined(__linux__)
    // Linux font enumeration using fontconfig
    if (FcInit()) {
        FcPattern* pattern = FcPatternCreate();
        FcObjectSet* os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_WEIGHT, FC_SLANT, nullptr);
        FcFontSet* fs = FcFontList(nullptr, pattern, os);

        if (fs) {
            for (int i = 0; i < fs->nfont; i++) {
                FcPattern* font = fs->fonts[i];

                FcChar8* family = nullptr;
                FcChar8* style = nullptr;
                FcChar8* file = nullptr;
                int weight = FC_WEIGHT_NORMAL;
                int slant = FC_SLANT_ROMAN;

                if (FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch &&
                    FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch &&
                    FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {

                    FcPatternGetInteger(font, FC_WEIGHT, 0, &weight);
                    FcPatternGetInteger(font, FC_SLANT, 0, &slant);

                    SystemFont system_font;
                    system_font.family_name = reinterpret_cast<const char*>(family);
                    system_font.style_name = reinterpret_cast<const char*>(style);
                    system_font.file_path = reinterpret_cast<const char*>(file);
                    system_font.is_bold = (weight >= FC_WEIGHT_BOLD);
                    system_font.is_italic = (slant != FC_SLANT_ROMAN);

                    s_system_fonts.push_back(system_font);
                }
            }
            FcFontSetDestroy(fs);
        }

        FcObjectSetDestroy(os);
        FcPatternDestroy(pattern);
    }

#elif defined(__APPLE__)
    // macOS font enumeration using Core Text
    CTFontCollectionRef collection = CTFontCollectionCreateFromAvailableFonts(nullptr);
    if (collection) {
        CFArrayRef fonts = CTFontCollectionCreateMatchingFontDescriptors(collection);
        if (fonts) {
            CFIndex count = CFArrayGetCount(fonts);
            for (CFIndex i = 0; i < count; i++) {
                CTFontDescriptorRef descriptor = (CTFontDescriptorRef)CFArrayGetValueAtIndex(fonts, i);

                CFStringRef family_name = (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontFamilyNameAttribute);
                CFStringRef style_name = (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontStyleNameAttribute);
                CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontURLAttribute);

                if (family_name && style_name && url) {
                    char family_buffer[256];
                    char style_buffer[256];
                    char path_buffer[1024];

                    if (CFStringGetCString(family_name, family_buffer, sizeof(family_buffer), kCFStringEncodingUTF8) &&
                        CFStringGetCString(style_name, style_buffer, sizeof(style_buffer), kCFStringEncodingUTF8) &&
                        CFURLGetFileSystemRepresentation(url, true, (UInt8*)path_buffer, sizeof(path_buffer))) {

                        SystemFont system_font;
                        system_font.family_name = family_buffer;
                        system_font.style_name = style_buffer;
                        system_font.file_path = path_buffer;
                        system_font.is_bold = (strstr(style_buffer, "Bold") != nullptr);
                        system_font.is_italic = (strstr(style_buffer, "Italic") != nullptr || strstr(style_buffer, "Oblique") != nullptr);

                        s_system_fonts.push_back(system_font);
                    }
                }

                if (family_name) CFRelease(family_name);
                if (style_name) CFRelease(style_name);
                if (url) CFRelease(url);
            }
            CFRelease(fonts);
        }
        CFRelease(collection);
    }

#else
    // Fallback: scan common font directories
    std::vector<std::string> font_dirs = {
        "/usr/share/fonts/",
        "/usr/local/share/fonts/",
        "/System/Library/Fonts/",
        "/Library/Fonts/"
    };

    for (const auto& font_dir : font_dirs) {
        if (std::filesystem::exists(font_dir)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(font_dir)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    if (ext == ".ttf" || ext == ".otf" || ext == ".ttc") {
                        SystemFont font = ExtractFontMetadata(entry.path().string());
                        if (!font.family_name.empty()) {
                            s_system_fonts.push_back(font);
                        }
                    }
                }
            }
        }
    }
#endif

    // Sort fonts by family name for easier browsing
    std::sort(s_system_fonts.begin(), s_system_fonts.end(),
              [](const SystemFont& a, const SystemFont& b) {
                  if (a.family_name != b.family_name) {
                      return a.family_name < b.family_name;
                  }
                  return a.style_name < b.style_name;
              });

    std::cout << "Enumerated " << s_system_fonts.size() << " system fonts" << std::endl;
}

SystemFont ResourceManager::ExtractFontMetadata(const std::string& file_path) {
    SystemFont font;
    font.file_path = file_path;

    // Try to load the font temporarily to extract metadata
    TTF_Font* ttf_font = TTF_OpenFont(file_path.c_str(), 16); // Use small size for metadata extraction
    if (ttf_font) {
        // Get font family name
        const char* family = TTF_FontFaceFamilyName(ttf_font);
        if (family) {
            font.family_name = family;
        }

        // Get font style name
        const char* style = TTF_FontFaceStyleName(ttf_font);
        if (style) {
            font.style_name = style;
        } else {
            font.style_name = "Regular";
        }

        // Determine if bold/italic from style name
        std::string style_lower = font.style_name;
        std::transform(style_lower.begin(), style_lower.end(), style_lower.begin(), ::tolower);
        font.is_bold = (style_lower.find("bold") != std::string::npos);
        font.is_italic = (style_lower.find("italic") != std::string::npos ||
                         style_lower.find("oblique") != std::string::npos);

        TTF_CloseFont(ttf_font);
    } else {
        // Fallback: extract name from filename
        std::filesystem::path path(file_path);
        std::string filename = path.stem().string();

        // Remove common suffixes
        std::vector<std::string> suffixes = {"-Regular", "-Bold", "-Italic", "-BoldItalic", "_Regular", "_Bold", "_Italic", "_BoldItalic"};
        for (const auto& suffix : suffixes) {
            size_t pos = filename.find(suffix);
            if (pos != std::string::npos) {
                font.family_name = filename.substr(0, pos);
                font.style_name = suffix.substr(1); // Remove the dash/underscore
                break;
            }
        }

        if (font.family_name.empty()) {
            font.family_name = filename;
            font.style_name = "Regular";
        }

        // Determine bold/italic from filename
        std::string filename_lower = filename;
        std::transform(filename_lower.begin(), filename_lower.end(), filename_lower.begin(), ::tolower);
        font.is_bold = (filename_lower.find("bold") != std::string::npos);
        font.is_italic = (filename_lower.find("italic") != std::string::npos ||
                         filename_lower.find("oblique") != std::string::npos);
    }

    return font;
}

// Implementation of inline methods that were moved from header
bool Texture::IsValid() const {
    return graphics_texture != nullptr && graphics_texture->IsValid();
}

unsigned int Texture::GetID() const {
    return graphics_texture ? graphics_texture->GetNativeHandle() : 0;
}

unsigned int Glyph::GetTextureID() const {
    return graphics_texture ? graphics_texture->GetNativeHandle() : 0;
}

} // namespace Lupine
