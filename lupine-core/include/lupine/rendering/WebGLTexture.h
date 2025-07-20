#pragma once

#include "GraphicsTexture.h"

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
// Forward declare WebGL types for non-Emscripten builds
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef float GLfloat;
#endif

namespace Lupine {

/**
 * @brief WebGL implementation of GraphicsTexture
 * 
 * Provides WebGL-specific texture functionality while implementing
 * the abstract GraphicsTexture interface for cross-platform compatibility.
 * Supports both 2D textures and cubemaps with WebGL format limitations.
 */
class WebGLTexture : public GraphicsTexture {
public:
    /**
     * @brief Constructor for 2D texture
     * @param width Texture width
     * @param height Texture height
     * @param format Texture format
     * @param data Initial texture data (can be nullptr)
     */
    WebGLTexture(int width, int height, TextureFormat format, const void* data = nullptr);

    /**
     * @brief Constructor for cubemap texture
     * @param size Cubemap face size (width and height)
     * @param format Texture format
     * @param data Array of 6 face data pointers (can be nullptr)
     */
    WebGLTexture(int size, TextureFormat format, const void* data[6] = nullptr);

    /**
     * @brief Destructor - cleans up WebGL texture
     */
    virtual ~WebGLTexture();

    // === GraphicsTexture Interface ===

    TextureType GetType() const override { return m_type; }
    TextureFormat GetFormat() const override { return m_format; }
    int GetWidth() const override { return m_width; }
    int GetHeight() const override { return m_height; }
    int GetDepth() const override { return 1; } // 2D textures have depth of 1
    int GetMipLevels() const override;

    void Bind(int unit = 0) override;
    void Unbind() override;

    void UpdateData(
        int level,
        int x, int y,
        int width, int height,
        const void* data
    ) override;

    void UpdateData(int level, const void* data) override;

    void UpdateCubemapFace(
        CubemapFace face,
        int level,
        const void* data
    ) override;

    void SetParameters(const TextureParameters& params) override;
    void GenerateMipmaps() override;

    uint32_t GetNativeHandle() const override { return m_texture_id; }
    bool IsValid() const override { return m_texture_id != 0; }
    size_t GetMemoryUsage() const override;

    // === WebGL-specific methods ===

    /**
     * @brief Get the WebGL texture ID
     * @return WebGL texture ID
     */
    GLuint GetTextureID() const { return m_texture_id; }

    /**
     * @brief Get the WebGL texture target
     * @return WebGL texture target (GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP)
     */
    GLenum GetTarget() const { return m_target; }

    /**
     * @brief Check if texture format is supported in WebGL
     * @param format Texture format to check
     * @return True if format is supported
     */
    static bool IsFormatSupported(TextureFormat format);

private:
    // Initialization
    bool Initialize(const void* data);
    bool InitializeCubemap(const void* data[6]);
    void SetDefaultParameters();

    // WebGL state
    GLuint m_texture_id;
    GLenum m_target;
    TextureType m_type;
    TextureFormat m_format;
    int m_width;
    int m_height;
    bool m_is_cubemap;
    int m_mip_levels;

    // Format conversion
    static GLenum GetGLInternalFormat(TextureFormat format);
    static GLenum GetGLFormat(TextureFormat format);
    static GLenum GetGLType(TextureFormat format);
    static GLenum GetGLCubemapFace(CubemapFace face);
    static GLenum GetGLWrapMode(TextureWrap wrap);
    static GLenum GetGLFilterMode(int filter_mode);

    // Utility
    int CalculateMipLevels() const;
    size_t CalculateMemoryUsage() const;
};

} // namespace Lupine
