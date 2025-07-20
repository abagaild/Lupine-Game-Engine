#include "lupine/rendering/WebGLBuffer.h"
#include <iostream>
#include <cassert>
#include <cstring>

namespace Lupine {

WebGLBuffer::WebGLBuffer(BufferType type, BufferUsage usage, size_t size, const void* data)
    : m_buffer_id(0)
    , m_target(GetGLTarget(type))
    , m_type(type)
    , m_usage(usage)
    , m_size(size)
    , m_is_mapped(false)
    , m_mapped_data(nullptr)
    , m_map_access(MapAccess::ReadOnly) {
    
    if (!Initialize(data)) {
        std::cerr << "Failed to initialize WebGL buffer" << std::endl;
    }
}

WebGLBuffer::~WebGLBuffer() {
#ifdef __EMSCRIPTEN__
    if (m_buffer_id != 0) {
        if (m_is_mapped) {
            Unmap();
        }
        glDeleteBuffers(1, &m_buffer_id);
        m_buffer_id = 0;
    }
#endif
    
    // Clean up mapped data fallback
    if (m_mapped_data) {
        delete[] static_cast<char*>(m_mapped_data);
        m_mapped_data = nullptr;
    }
}

void WebGLBuffer::Bind() {
#ifdef __EMSCRIPTEN__
    if (IsValid()) {
        glBindBuffer(m_target, m_buffer_id);
    }
#endif
}

void WebGLBuffer::Unbind() {
#ifdef __EMSCRIPTEN__
    glBindBuffer(m_target, 0);
#endif
}

void WebGLBuffer::UpdateData(size_t offset, size_t size, const void* data) {
#ifdef __EMSCRIPTEN__
    if (!IsValid() || !data) {
        return;
    }
    
    if (offset + size > m_size) {
        std::cerr << "WebGL buffer update out of bounds" << std::endl;
        return;
    }
    
    Bind();
    glBufferSubData(m_target, offset, size, data);
    Unbind();
#endif
}

void WebGLBuffer::UpdateData(const void* data, size_t size) {
#ifdef __EMSCRIPTEN__
    if (!IsValid()) {
        return;
    }
    
    if (size != m_size) {
        std::cerr << "WebGL buffer size mismatch in UpdateData" << std::endl;
        return;
    }
    
    Bind();
    if (data) {
        glBufferSubData(m_target, 0, size, data);
    }
    Unbind();
#endif
}

void* WebGLBuffer::Map(MapAccess access) {
#ifdef __EMSCRIPTEN__
    if (!IsValid() || m_is_mapped) {
        return nullptr;
    }
    
    // WebGL has limited mapping support, so we use a fallback approach
    // Allocate local memory and read buffer data if needed
    if (!m_mapped_data) {
        m_mapped_data = new char[m_size];
    }
    
    m_map_access = access;
    m_is_mapped = true;
    
    // For read access, we would need to read back from GPU
    // This is not efficiently supported in WebGL, so we just return the buffer
    // The application should avoid mapping in WebGL when possible
    
    return m_mapped_data;
#else
    return nullptr;
#endif
}

void WebGLBuffer::Unmap() {
#ifdef __EMSCRIPTEN__
    if (!m_is_mapped || !m_mapped_data) {
        return;
    }
    
    // If we were writing, upload the data back to GPU
    if (m_map_access == MapAccess::WriteOnly || m_map_access == MapAccess::ReadWrite) {
        Bind();
        glBufferSubData(m_target, 0, m_size, m_mapped_data);
        Unbind();
    }
    
    m_is_mapped = false;
#endif
}

bool WebGLBuffer::IsMappingSupported() const {
    // WebGL has very limited mapping support compared to desktop OpenGL
    // We provide a fallback but it's not efficient
    return true; // Fallback is always available
}

bool WebGLBuffer::Initialize(const void* data) {
#ifdef __EMSCRIPTEN__
    glGenBuffers(1, &m_buffer_id);
    if (m_buffer_id == 0) {
        return false;
    }
    
    glBindBuffer(m_target, m_buffer_id);
    glBufferData(m_target, m_size, data, GetGLUsage(m_usage));
    glBindBuffer(m_target, 0);
    
    return true;
#else
    // For non-Emscripten builds, just mark as valid
    m_buffer_id = 1; // Fake ID for non-WebGL builds
    return true;
#endif
}

// Static conversion methods
GLenum WebGLBuffer::GetGLTarget(BufferType type) {
#ifdef __EMSCRIPTEN__
    switch (type) {
        case BufferType::Vertex:  return GL_ARRAY_BUFFER;
        case BufferType::Index:   return GL_ELEMENT_ARRAY_BUFFER;
        case BufferType::Uniform: return GL_UNIFORM_BUFFER;
        default: return GL_ARRAY_BUFFER;
    }
#else
    return 0;
#endif
}

GLenum WebGLBuffer::GetGLUsage(BufferUsage usage) {
#ifdef __EMSCRIPTEN__
    switch (usage) {
        case BufferUsage::Static:  return GL_STATIC_DRAW;
        case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
        case BufferUsage::Stream:  return GL_STREAM_DRAW;
        default: return GL_STATIC_DRAW;
    }
#else
    return 0;
#endif
}

} // namespace Lupine
