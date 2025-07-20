#pragma once

#include "GraphicsTexture.h"
#include <glad/glad.h>

namespace Lupine {

/**
 * @brief OpenGL implementation of GraphicsTexture
 * 
 * Provides OpenGL-specific texture functionality while implementing
 * the abstract GraphicsTexture interface for cross-platform compatibility.
 */
class OpenGLTexture : public GraphicsTexture {
public:
    /**
     * @brief Constructor for 2D texture
     * @param width Texture width
     * @param height Texture height
     * @param format Texture format
     * @param data Initial texture data (can be nullptr)
     */
    OpenGLTexture(int width, int height, TextureFormat format, const void* data = nullptr);

    /**
     * @brief Constructor for cubemap texture
     * @param size Cubemap face size
     * @param format Texture format
     * @param data Array of 6 face data pointers (can be nullptr)
     */
    OpenGLTexture(int size, TextureFormat format, const void* data[6] = nullptr);

    /**
     * @brief Destructor - cleans up OpenGL texture
     */
    virtual ~OpenGLTexture();

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

    // === OpenGL-specific methods ===

    /**
     * @brief Get the OpenGL texture ID
     * @return OpenGL texture ID
     */
    GLuint GetTextureID() const { return m_texture_id; }

    /**
     * @brief Get the OpenGL texture target
     * @return OpenGL texture target (GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, etc.)
     */
    GLenum GetTarget() const { return m_target; }

    /**
     * @brief Convert TextureFormat to OpenGL internal format
     * @param format Texture format
     * @return OpenGL internal format
     */
    static GLenum GetGLInternalFormat(TextureFormat format);

    /**
     * @brief Convert TextureFormat to OpenGL format
     * @param format Texture format
     * @return OpenGL format
     */
    static GLenum GetGLFormat(TextureFormat format);

    /**
     * @brief Convert TextureFormat to OpenGL type
     * @param format Texture format
     * @return OpenGL type
     */
    static GLenum GetGLType(TextureFormat format);

    /**
     * @brief Convert TextureWrap to OpenGL wrap mode
     * @param wrap Texture wrap mode
     * @return OpenGL wrap mode
     */
    static GLenum GetGLWrapMode(TextureWrap wrap);

private:
    GLuint m_texture_id;
    GLenum m_target;
    TextureType m_type;
    TextureFormat m_format;
    int m_width;
    int m_height;
    bool m_is_cubemap;

    /**
     * @brief Initialize the OpenGL texture
     * @param data Initial texture data
     * @return True if successful
     */
    bool Initialize(const void* data);

    /**
     * @brief Initialize cubemap texture
     * @param data Array of 6 face data pointers
     * @return True if successful
     */
    bool InitializeCubemap(const void* data[6]);

    /**
     * @brief Set default texture parameters
     */
    void SetDefaultParameters();
};

} // namespace Lupine
