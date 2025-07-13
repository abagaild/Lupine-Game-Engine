#pragma once

#include "GraphicsDevice.h"
#include <cstdint>
#include <memory>

namespace Lupine {

/**
 * @brief Texture types
 */
enum class TextureType {
    Texture2D,      // 2D texture
    TextureCubemap, // Cubemap texture
    Texture3D,      // 3D texture (if supported)
    TextureArray    // Texture array (if supported)
};

/**
 * @brief Cubemap faces
 */
enum class CubemapFace {
    PositiveX = 0,  // Right
    NegativeX = 1,  // Left
    PositiveY = 2,  // Top
    NegativeY = 3,  // Bottom
    PositiveZ = 4,  // Front
    NegativeZ = 5   // Back
};

/**
 * @brief Texture parameters
 */
struct TextureParameters {
    int min_filter = 0; // Will be set to appropriate backend value
    int mag_filter = 0; // Will be set to appropriate backend value
    TextureWrap wrap_s = TextureWrap::Repeat;
    TextureWrap wrap_t = TextureWrap::Repeat;
    TextureWrap wrap_r = TextureWrap::Repeat;
    float anisotropy = 1.0f;
    bool generate_mipmaps = true;
    
    TextureParameters() = default;
};

/**
 * @brief Abstract graphics texture interface
 * 
 * Represents a texture object that can store image data on the GPU.
 * This abstraction allows for different graphics backends while
 * maintaining a consistent interface.
 */
class GraphicsTexture {
public:
    virtual ~GraphicsTexture() = default;

    /**
     * @brief Get the texture type
     * @return Texture type enumeration
     */
    virtual TextureType GetType() const = 0;

    /**
     * @brief Get the texture format
     * @return Texture format enumeration
     */
    virtual TextureFormat GetFormat() const = 0;

    /**
     * @brief Get the texture width
     * @return Texture width in pixels
     */
    virtual int GetWidth() const = 0;

    /**
     * @brief Get the texture height
     * @return Texture height in pixels
     */
    virtual int GetHeight() const = 0;

    /**
     * @brief Get the texture depth (for 3D textures)
     * @return Texture depth in pixels
     */
    virtual int GetDepth() const = 0;

    /**
     * @brief Get the number of mipmap levels
     * @return Number of mipmap levels
     */
    virtual int GetMipLevels() const = 0;

    /**
     * @brief Bind the texture to a texture unit
     * @param unit Texture unit index (0-based)
     */
    virtual void Bind(int unit = 0) = 0;

    /**
     * @brief Unbind the texture from its current unit
     */
    virtual void Unbind() = 0;

    /**
     * @brief Update texture data
     * @param level Mipmap level (0 = base level)
     * @param x X offset
     * @param y Y offset
     * @param width Width of update region
     * @param height Height of update region
     * @param data Pointer to new texture data
     */
    virtual void UpdateData(
        int level,
        int x, int y,
        int width, int height,
        const void* data
    ) = 0;

    /**
     * @brief Update entire texture data
     * @param level Mipmap level (0 = base level)
     * @param data Pointer to new texture data
     */
    virtual void UpdateData(int level, const void* data) = 0;

    /**
     * @brief Update cubemap face data
     * @param face Cubemap face
     * @param level Mipmap level
     * @param data Pointer to face data
     */
    virtual void UpdateCubemapFace(
        CubemapFace face,
        int level,
        const void* data
    ) = 0;

    /**
     * @brief Set texture parameters
     * @param params Texture parameters
     */
    virtual void SetParameters(const TextureParameters& params) = 0;

    /**
     * @brief Generate mipmaps for the texture
     */
    virtual void GenerateMipmaps() = 0;

    /**
     * @brief Get the native texture handle (backend-specific)
     * @return Native texture handle (e.g., OpenGL texture ID)
     */
    virtual uint32_t GetNativeHandle() const = 0;

    /**
     * @brief Check if the texture is valid
     * @return True if texture is valid and can be used
     */
    virtual bool IsValid() const = 0;

    /**
     * @brief Get texture memory usage in bytes
     * @return Memory usage in bytes
     */
    virtual size_t GetMemoryUsage() const = 0;

    // === Utility Methods ===

    /**
     * @brief Check if texture is a power of two
     * @return True if both width and height are powers of two
     */
    bool IsPowerOfTwo() const {
        auto isPowerOfTwo = [](int value) {
            return value > 0 && (value & (value - 1)) == 0;
        };
        return isPowerOfTwo(GetWidth()) && isPowerOfTwo(GetHeight());
    }

    /**
     * @brief Get the aspect ratio
     * @return Width / Height ratio
     */
    float GetAspectRatio() const {
        int height = GetHeight();
        return height > 0 ? static_cast<float>(GetWidth()) / height : 1.0f;
    }

    /**
     * @brief Calculate the number of bytes per pixel for a format
     * @param format Texture format
     * @return Bytes per pixel
     */
    static int GetBytesPerPixel(TextureFormat format) {
        switch (format) {
            case TextureFormat::RGB8:     return 3;
            case TextureFormat::RGBA8:    return 4;
            case TextureFormat::RGB16F:   return 6;
            case TextureFormat::RGBA16F:  return 8;
            case TextureFormat::RGB32F:   return 12;
            case TextureFormat::RGBA32F:  return 16;
            case TextureFormat::Depth24:  return 3;
            case TextureFormat::Depth32F: return 4;
            default: return 4;
        }
    }

    /**
     * @brief Check if format supports floating point data
     * @param format Texture format
     * @return True if format uses floating point
     */
    static bool IsFloatFormat(TextureFormat format) {
        return format == TextureFormat::RGB16F ||
               format == TextureFormat::RGBA16F ||
               format == TextureFormat::RGB32F ||
               format == TextureFormat::RGBA32F ||
               format == TextureFormat::Depth32F;
    }

    /**
     * @brief Check if format is a depth format
     * @param format Texture format
     * @return True if format is depth-only
     */
    static bool IsDepthFormat(TextureFormat format) {
        return format == TextureFormat::Depth24 ||
               format == TextureFormat::Depth32F;
    }

protected:
    // Protected constructor to prevent direct instantiation
    GraphicsTexture() = default;
};

} // namespace Lupine
