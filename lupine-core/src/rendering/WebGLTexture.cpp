#include "lupine/rendering/WebGLTexture.h"
#include <iostream>
#include <cassert>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#endif

namespace Lupine {

WebGLTexture::WebGLTexture(int width, int height, TextureFormat format, const void* data)
    : m_texture_id(0)
#ifdef __EMSCRIPTEN__
    , m_target(GL_TEXTURE_2D)
#else
    , m_target(0x0DE1) // GL_TEXTURE_2D
#endif
    , m_type(TextureType::Texture2D)
    , m_format(format)
    , m_width(width)
    , m_height(height)
    , m_is_cubemap(false)
    , m_mip_levels(1) {
    
    if (!Initialize(data)) {
        std::cerr << "Failed to initialize WebGL 2D texture" << std::endl;
    }
}

WebGLTexture::WebGLTexture(int size, TextureFormat format, const void* data[6])
    : m_texture_id(0)
#ifdef __EMSCRIPTEN__
    , m_target(GL_TEXTURE_CUBE_MAP)
#else
    , m_target(0x8513) // GL_TEXTURE_CUBE_MAP
#endif
    , m_type(TextureType::TextureCubemap)
    , m_format(format)
    , m_width(size)
    , m_height(size)
    , m_is_cubemap(true)
    , m_mip_levels(1) {
    
    if (!InitializeCubemap(data)) {
        std::cerr << "Failed to initialize WebGL cubemap texture" << std::endl;
    }
}

WebGLTexture::~WebGLTexture() {
#ifdef __EMSCRIPTEN__
    if (m_texture_id != 0) {
        glDeleteTextures(1, &m_texture_id);
        m_texture_id = 0;
    }
#endif
}

int WebGLTexture::GetMipLevels() const {
    return m_mip_levels;
}

void WebGLTexture::Bind(int unit) {
#ifdef __EMSCRIPTEN__
    assert(unit >= 0 && unit < 32); // Reasonable texture unit limit
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(m_target, m_texture_id);
#endif
}

void WebGLTexture::Unbind() {
#ifdef __EMSCRIPTEN__
    glBindTexture(m_target, 0);
#endif
}

void WebGLTexture::UpdateData(
    int level,
    int x, int y,
    int width, int height,
    const void* data) {
#ifdef __EMSCRIPTEN__
    if (!IsValid() || m_is_cubemap) {
        return;
    }
    
    Bind();
    GLenum format = GetGLFormat(m_format);
    GLenum type = GetGLType(m_format);
    glTexSubImage2D(m_target, level, x, y, width, height, format, type, data);
    Unbind();
#endif
}

void WebGLTexture::UpdateData(int level, const void* data) {
#ifdef __EMSCRIPTEN__
    if (!IsValid() || m_is_cubemap) {
        return;
    }
    
    Bind();
    GLenum internal_format = GetGLInternalFormat(m_format);
    GLenum format = GetGLFormat(m_format);
    GLenum type = GetGLType(m_format);
    
    // Calculate mip level dimensions
    int mip_width = std::max(1, m_width >> level);
    int mip_height = std::max(1, m_height >> level);
    
    glTexImage2D(m_target, level, internal_format, mip_width, mip_height, 0, format, type, data);
    Unbind();
#endif
}

void WebGLTexture::UpdateCubemapFace(
    CubemapFace face,
    int level,
    const void* data) {
#ifdef __EMSCRIPTEN__
    if (!IsValid() || !m_is_cubemap) {
        return;
    }
    
    Bind();
    GLenum internal_format = GetGLInternalFormat(m_format);
    GLenum format = GetGLFormat(m_format);
    GLenum type = GetGLType(m_format);
    GLenum gl_face = GetGLCubemapFace(face);
    
    // Calculate mip level dimensions
    int mip_size = std::max(1, m_width >> level);
    
    glTexImage2D(gl_face, level, internal_format, mip_size, mip_size, 0, format, type, data);
    Unbind();
#endif
}

void WebGLTexture::SetParameters(const TextureParameters& params) {
#ifdef __EMSCRIPTEN__
    if (!IsValid()) {
        return;
    }
    
    Bind();
    
    // Set filtering
    GLenum min_filter = GetGLFilterMode(params.min_filter);
    GLenum mag_filter = GetGLFilterMode(params.mag_filter);
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, mag_filter);
    
    // Set wrapping
    GLenum wrap_s = GetGLWrapMode(params.wrap_s);
    GLenum wrap_t = GetGLWrapMode(params.wrap_t);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, wrap_t);
    
    if (m_is_cubemap) {
        GLenum wrap_r = GetGLWrapMode(params.wrap_r);
        glTexParameteri(m_target, GL_TEXTURE_WRAP_R, wrap_r);
    }
    
    Unbind();
#endif
}

void WebGLTexture::GenerateMipmaps() {
#ifdef __EMSCRIPTEN__
    if (!IsValid()) {
        return;
    }
    
    Bind();
    glGenerateMipmap(m_target);
    m_mip_levels = CalculateMipLevels();
    Unbind();
#endif
}

size_t WebGLTexture::GetMemoryUsage() const {
    return CalculateMemoryUsage();
}

bool WebGLTexture::IsFormatSupported(TextureFormat format) {
    // WebGL supports a subset of OpenGL formats
    switch (format) {
        case TextureFormat::R8:
        case TextureFormat::RG8:
        case TextureFormat::RGB8:
        case TextureFormat::RGBA8:
        case TextureFormat::R16F:
        case TextureFormat::RG16F:
        case TextureFormat::RGB16F:
        case TextureFormat::RGBA16F:
        case TextureFormat::R32F:
        case TextureFormat::RG32F:
        case TextureFormat::RGB32F:
        case TextureFormat::RGBA32F:
        case TextureFormat::Depth16:
        case TextureFormat::Depth24:
        case TextureFormat::Depth32F:
            return true;
        default:
            return false;
    }
}

bool WebGLTexture::Initialize(const void* data) {
#ifdef __EMSCRIPTEN__
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
#else
    // For non-Emscripten builds, just mark as valid
    m_texture_id = 1; // Fake ID for non-WebGL builds
    return true;
#endif
}

bool WebGLTexture::InitializeCubemap(const void* data[6]) {
#ifdef __EMSCRIPTEN__
    glGenTextures(1, &m_texture_id);
    if (m_texture_id == 0) {
        return false;
    }
    
    glBindTexture(m_target, m_texture_id);
    
    GLenum internal_format = GetGLInternalFormat(m_format);
    GLenum format = GetGLFormat(m_format);
    GLenum type = GetGLType(m_format);
    
    // Upload all 6 faces
    GLenum faces[6] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };
    
    for (int i = 0; i < 6; ++i) {
        const void* face_data = data ? data[i] : nullptr;
        glTexImage2D(faces[i], 0, internal_format, m_width, m_height, 0, format, type, face_data);
    }
    
    SetDefaultParameters();
    
    glBindTexture(m_target, 0);
    
    return true;
#else
    // For non-Emscripten builds, just mark as valid
    m_texture_id = 1; // Fake ID for non-WebGL builds
    return true;
#endif
}

void WebGLTexture::SetDefaultParameters() {
#ifdef __EMSCRIPTEN__
    // Set default filtering and wrapping
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    if (m_is_cubemap) {
        glTexParameteri(m_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
#endif
}

int WebGLTexture::CalculateMipLevels() const {
    int max_dim = std::max(m_width, m_height);
    int levels = 1;
    while (max_dim > 1) {
        max_dim >>= 1;
        levels++;
    }
    return levels;
}

size_t WebGLTexture::CalculateMemoryUsage() const {
    // Estimate memory usage based on format and dimensions
    size_t bytes_per_pixel = 4; // Default to RGBA8
    
    switch (m_format) {
        case TextureFormat::R8: bytes_per_pixel = 1; break;
        case TextureFormat::RG8: bytes_per_pixel = 2; break;
        case TextureFormat::RGB8: bytes_per_pixel = 3; break;
        case TextureFormat::RGBA8: bytes_per_pixel = 4; break;
        case TextureFormat::R16F: bytes_per_pixel = 2; break;
        case TextureFormat::RG16F: bytes_per_pixel = 4; break;
        case TextureFormat::RGB16F: bytes_per_pixel = 6; break;
        case TextureFormat::RGBA16F: bytes_per_pixel = 8; break;
        case TextureFormat::R32F: bytes_per_pixel = 4; break;
        case TextureFormat::RG32F: bytes_per_pixel = 8; break;
        case TextureFormat::RGB32F: bytes_per_pixel = 12; break;
        case TextureFormat::RGBA32F: bytes_per_pixel = 16; break;
        default: bytes_per_pixel = 4; break;
    }
    
    size_t base_size = m_width * m_height * bytes_per_pixel;
    if (m_is_cubemap) {
        base_size *= 6; // 6 faces
    }
    
    // Add mipmap memory if mipmaps are present
    if (m_mip_levels > 1) {
        base_size = base_size * 4 / 3; // Approximate mipmap overhead
    }
    
    return base_size;
}

// Static format conversion methods
GLenum WebGLTexture::GetGLInternalFormat(TextureFormat format) {
#ifdef __EMSCRIPTEN__
    switch (format) {
        case TextureFormat::R8:       return GL_R8;
        case TextureFormat::RG8:      return GL_RG8;
        case TextureFormat::RGB8:     return GL_RGB8;
        case TextureFormat::RGBA8:    return GL_RGBA8;
        case TextureFormat::R16F:     return GL_R16F;
        case TextureFormat::RG16F:    return GL_RG16F;
        case TextureFormat::RGB16F:   return GL_RGB16F;
        case TextureFormat::RGBA16F:  return GL_RGBA16F;
        case TextureFormat::R32F:     return GL_R32F;
        case TextureFormat::RG32F:    return GL_RG32F;
        case TextureFormat::RGB32F:   return GL_RGB32F;
        case TextureFormat::RGBA32F:  return GL_RGBA32F;
        case TextureFormat::Depth16:  return GL_DEPTH_COMPONENT16;
        case TextureFormat::Depth24:  return GL_DEPTH_COMPONENT24;
        case TextureFormat::Depth32F: return GL_DEPTH_COMPONENT32F;
        default: return GL_RGBA8;
    }
#else
    return 0;
#endif
}

GLenum WebGLTexture::GetGLFormat(TextureFormat format) {
#ifdef __EMSCRIPTEN__
    switch (format) {
        case TextureFormat::R8:
        case TextureFormat::R16F:
        case TextureFormat::R32F:     return GL_RED;
        case TextureFormat::RG8:
        case TextureFormat::RG16F:
        case TextureFormat::RG32F:    return GL_RG;
        case TextureFormat::RGB8:
        case TextureFormat::RGB16F:
        case TextureFormat::RGB32F:   return GL_RGB;
        case TextureFormat::RGBA8:
        case TextureFormat::RGBA16F:
        case TextureFormat::RGBA32F:  return GL_RGBA;
        case TextureFormat::Depth16:
        case TextureFormat::Depth24:
        case TextureFormat::Depth32F: return GL_DEPTH_COMPONENT;
        default: return GL_RGBA;
    }
#else
    return 0;
#endif
}

GLenum WebGLTexture::GetGLType(TextureFormat format) {
#ifdef __EMSCRIPTEN__
    switch (format) {
        case TextureFormat::R8:
        case TextureFormat::RG8:
        case TextureFormat::RGB8:
        case TextureFormat::RGBA8:    return GL_UNSIGNED_BYTE;
        case TextureFormat::R16F:
        case TextureFormat::RG16F:
        case TextureFormat::RGB16F:
        case TextureFormat::RGBA16F:  return GL_HALF_FLOAT;
        case TextureFormat::R32F:
        case TextureFormat::RG32F:
        case TextureFormat::RGB32F:
        case TextureFormat::RGBA32F:
        case TextureFormat::Depth32F: return GL_FLOAT;
        case TextureFormat::Depth16:  return GL_UNSIGNED_SHORT;
        case TextureFormat::Depth24:  return GL_UNSIGNED_INT;
        default: return GL_UNSIGNED_BYTE;
    }
#else
    return 0;
#endif
}

GLenum WebGLTexture::GetGLCubemapFace(CubemapFace face) {
#ifdef __EMSCRIPTEN__
    switch (face) {
        case CubemapFace::PositiveX: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        case CubemapFace::NegativeX: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        case CubemapFace::PositiveY: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        case CubemapFace::NegativeY: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        case CubemapFace::PositiveZ: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
        case CubemapFace::NegativeZ: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
        default: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    }
#else
    return 0;
#endif
}

GLenum WebGLTexture::GetGLWrapMode(TextureWrap wrap) {
#ifdef __EMSCRIPTEN__
    switch (wrap) {
        case TextureWrap::Repeat:         return GL_REPEAT;
        case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
        case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        default: return GL_CLAMP_TO_EDGE;
    }
#else
    return 0;
#endif
}

GLenum WebGLTexture::GetGLFilterMode(int filter_mode) {
#ifdef __EMSCRIPTEN__
    // For now, use simple mapping - can be enhanced later
    switch (filter_mode) {
        case 0: return GL_NEAREST;  // Nearest
        case 1: return GL_LINEAR;   // Linear/Bilinear
        case 2: return GL_LINEAR;   // Bicubic (fallback to linear)
        default: return GL_LINEAR;
    }
#else
    return 0;
#endif
}

} // namespace Lupine
