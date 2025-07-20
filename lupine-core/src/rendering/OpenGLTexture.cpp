#include "lupine/rendering/OpenGLTexture.h"
#include <iostream>
#include <cassert>

namespace Lupine {

OpenGLTexture::OpenGLTexture(int width, int height, TextureFormat format, const void* data)
    : m_texture_id(0)
    , m_target(GL_TEXTURE_2D)
    , m_type(TextureType::Texture2D)
    , m_format(format)
    , m_width(width)
    , m_height(height)
    , m_is_cubemap(false) {
    
    if (!Initialize(data)) {
        std::cerr << "Failed to initialize OpenGL 2D texture" << std::endl;
    }
}

OpenGLTexture::OpenGLTexture(int size, TextureFormat format, const void* data[6])
    : m_texture_id(0)
    , m_target(GL_TEXTURE_CUBE_MAP)
    , m_type(TextureType::TextureCubemap)
    , m_format(format)
    , m_width(size)
    , m_height(size)
    , m_is_cubemap(true) {
    
    if (!InitializeCubemap(data)) {
        std::cerr << "Failed to initialize OpenGL cubemap texture" << std::endl;
    }
}

OpenGLTexture::~OpenGLTexture() {
    if (m_texture_id != 0) {
        glDeleteTextures(1, &m_texture_id);
        m_texture_id = 0;
    }
}

void OpenGLTexture::Bind(int unit) {
    assert(unit >= 0 && unit < 32); // Reasonable texture unit limit
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(m_target, m_texture_id);
}

void OpenGLTexture::Unbind() {
    glBindTexture(m_target, 0);
}

void OpenGLTexture::UpdateData(int level, int x, int y, int width, int height, const void* data) {
    if (!IsValid()) return;
    
    glBindTexture(m_target, m_texture_id);
    
    GLenum format = GetGLFormat(m_format);
    GLenum type = GetGLType(m_format);
    
    if (m_target == GL_TEXTURE_2D) {
        glTexSubImage2D(GL_TEXTURE_2D, level, x, y, width, height, format, type, data);
    }
    
    glBindTexture(m_target, 0);
}

void OpenGLTexture::UpdateData(int level, const void* data) {
    UpdateData(level, 0, 0, m_width, m_height, data);
}

void OpenGLTexture::UpdateCubemapFace(CubemapFace face, int level, const void* data) {
    if (!IsValid() || !m_is_cubemap) return;
    
    glBindTexture(m_target, m_texture_id);
    
    GLenum gl_face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<int>(face);
    GLenum format = GetGLFormat(m_format);
    GLenum type = GetGLType(m_format);
    
    glTexSubImage2D(gl_face, level, 0, 0, m_width, m_height, format, type, data);
    
    glBindTexture(m_target, 0);
}

void OpenGLTexture::SetParameters(const TextureParameters& params) {
    if (!IsValid()) return;
    
    glBindTexture(m_target, m_texture_id);
    
    // Set wrap modes
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GetGLWrapMode(params.wrap_s));
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GetGLWrapMode(params.wrap_t));
    if (m_is_cubemap) {
        glTexParameteri(m_target, GL_TEXTURE_WRAP_R, GetGLWrapMode(params.wrap_r));
    }
    
    // Set filter modes (use provided values or defaults)
    GLint min_filter = params.min_filter != 0 ? params.min_filter : GL_LINEAR;
    GLint mag_filter = params.mag_filter != 0 ? params.mag_filter : GL_LINEAR;
    
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, mag_filter);
    
    // Set anisotropic filtering if supported
    if (params.anisotropy > 1.0f) {
        // Check if anisotropic filtering is supported
        // Note: Extension checking should be done through the graphics device
        // For now, we'll skip anisotropic filtering
        // TODO: Add proper extension checking through GraphicsDevice
    }
    
    glBindTexture(m_target, 0);
}

void OpenGLTexture::GenerateMipmaps() {
    if (!IsValid()) return;
    
    glBindTexture(m_target, m_texture_id);
    glGenerateMipmap(m_target);
    glBindTexture(m_target, 0);
}

size_t OpenGLTexture::GetMemoryUsage() const {
    if (!IsValid()) return 0;
    
    int bytes_per_pixel = GraphicsTexture::GetBytesPerPixel(m_format);
    size_t base_size = m_width * m_height * bytes_per_pixel;
    
    if (m_is_cubemap) {
        base_size *= 6; // 6 faces
    }
    
    // Estimate mipmap memory usage (approximately 1.33x base size)
    return static_cast<size_t>(base_size * 1.33f);
}

bool OpenGLTexture::Initialize(const void* data) {
    glGenTextures(1, &m_texture_id);
    if (m_texture_id == 0) {
        return false;
    }
    
    glBindTexture(m_target, m_texture_id);
    
    GLenum internal_format = GetGLInternalFormat(m_format);
    GLenum format = GetGLFormat(m_format);
    GLenum type = GetGLType(m_format);
    
    glTexImage2D(m_target, 0, internal_format, m_width, m_height, 0, format, type, data);
    
    SetDefaultParameters();
    
    glBindTexture(m_target, 0);
    
    return true;
}

bool OpenGLTexture::InitializeCubemap(const void* data[6]) {
    glGenTextures(1, &m_texture_id);
    if (m_texture_id == 0) {
        return false;
    }
    
    glBindTexture(m_target, m_texture_id);
    
    GLenum internal_format = GetGLInternalFormat(m_format);
    GLenum format = GetGLFormat(m_format);
    GLenum type = GetGLType(m_format);
    
    // Upload all 6 faces
    for (int i = 0; i < 6; ++i) {
        GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        const void* face_data = data ? data[i] : nullptr;
        glTexImage2D(face, 0, internal_format, m_width, m_height, 0, format, type, face_data);
    }
    
    SetDefaultParameters();
    
    glBindTexture(m_target, 0);
    
    return true;
}

void OpenGLTexture::SetDefaultParameters() {
    // Set default texture parameters
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (m_is_cubemap) {
        glTexParameteri(m_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

// Static conversion methods
GLenum OpenGLTexture::GetGLInternalFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGB8:     return GL_RGB8;
        case TextureFormat::RGBA8:    return GL_RGBA8;
        case TextureFormat::RGB16F:   return GL_RGB16F;
        case TextureFormat::RGBA16F:  return GL_RGBA16F;
        case TextureFormat::RGB32F:   return GL_RGB32F;
        case TextureFormat::RGBA32F:  return GL_RGBA32F;
        case TextureFormat::Depth24:  return GL_DEPTH_COMPONENT24;
        case TextureFormat::Depth32F: return GL_DEPTH_COMPONENT32F;
        default: return GL_RGBA8;
    }
}

GLenum OpenGLTexture::GetGLFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGB8:
        case TextureFormat::RGB16F:
        case TextureFormat::RGB32F:   return GL_RGB;
        case TextureFormat::RGBA8:
        case TextureFormat::RGBA16F:
        case TextureFormat::RGBA32F:  return GL_RGBA;
        case TextureFormat::Depth24:
        case TextureFormat::Depth32F: return GL_DEPTH_COMPONENT;
        default: return GL_RGBA;
    }
}

GLenum OpenGLTexture::GetGLType(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGB8:
        case TextureFormat::RGBA8:    return GL_UNSIGNED_BYTE;
        case TextureFormat::RGB16F:
        case TextureFormat::RGBA16F:  return GL_HALF_FLOAT;
        case TextureFormat::RGB32F:
        case TextureFormat::RGBA32F:
        case TextureFormat::Depth32F: return GL_FLOAT;
        case TextureFormat::Depth24:  return GL_UNSIGNED_INT;
        default: return GL_UNSIGNED_BYTE;
    }
}

GLenum OpenGLTexture::GetGLWrapMode(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::Repeat:         return GL_REPEAT;
        case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        case TextureWrap::ClampToBorder:  return GL_CLAMP_TO_BORDER;
        case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
        default: return GL_REPEAT;
    }
}

int OpenGLTexture::GetMipLevels() const {
    if (!IsValid()) {
        return 0;
    }

    // Calculate maximum possible mip levels based on texture dimensions
    int max_dimension = std::max(m_width, m_height);
    if (max_dimension <= 0) {
        return 1;
    }

    // Calculate log2(max_dimension) + 1
    int mip_levels = 1;
    while (max_dimension > 1) {
        max_dimension >>= 1;
        mip_levels++;
    }

    return mip_levels;
}

} // namespace Lupine
