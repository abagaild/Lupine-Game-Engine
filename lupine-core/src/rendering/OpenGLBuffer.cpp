#include "lupine/rendering/OpenGLBuffer.h"
#include <iostream>
#include <cassert>

namespace Lupine {

OpenGLBuffer::OpenGLBuffer(BufferType type, BufferUsage usage, size_t size, const void* data)
    : m_buffer_id(0)
    , m_target(GetGLTarget(type))
    , m_type(type)
    , m_usage(usage)
    , m_size(size)
    , m_is_mapped(false) {
    
    if (!Initialize(data)) {
        std::cerr << "Failed to initialize OpenGL buffer" << std::endl;
    }
}

OpenGLBuffer::~OpenGLBuffer() {
    if (m_buffer_id != 0) {
        if (m_is_mapped) {
            Unmap();
        }
        glDeleteBuffers(1, &m_buffer_id);
        m_buffer_id = 0;
    }
}

void OpenGLBuffer::Bind() {
    if (IsValid()) {
        glBindBuffer(m_target, m_buffer_id);
    }
}

void OpenGLBuffer::Unbind() {
    glBindBuffer(m_target, 0);
}

void OpenGLBuffer::UpdateData(size_t offset, size_t size, const void* data) {
    if (!IsValid() || m_is_mapped) return;
    
    glBindBuffer(m_target, m_buffer_id);
    glBufferSubData(m_target, offset, size, data);
    glBindBuffer(m_target, 0);
}

void OpenGLBuffer::UpdateData(const void* data, size_t size) {
    UpdateData(0, size, data);
}

void* OpenGLBuffer::Map(MapAccess access) {
    if (!IsValid() || m_is_mapped) return nullptr;
    
    glBindBuffer(m_target, m_buffer_id);
    void* ptr = glMapBuffer(m_target, GetGLAccess(access));
    
    if (ptr) {
        m_is_mapped = true;
    }
    
    return ptr;
}

void OpenGLBuffer::Unmap() {
    if (!IsValid() || !m_is_mapped) return;
    
    glBindBuffer(m_target, m_buffer_id);
    glUnmapBuffer(m_target);
    glBindBuffer(m_target, 0);
    
    m_is_mapped = false;
}

bool OpenGLBuffer::Initialize(const void* data) {
    glGenBuffers(1, &m_buffer_id);
    if (m_buffer_id == 0) {
        return false;
    }
    
    glBindBuffer(m_target, m_buffer_id);
    glBufferData(m_target, m_size, data, GetGLUsage(m_usage));
    glBindBuffer(m_target, 0);
    
    return true;
}

// Static conversion methods
GLenum OpenGLBuffer::GetGLTarget(BufferType type) {
    switch (type) {
        case BufferType::Vertex:  return GL_ARRAY_BUFFER;
        case BufferType::Index:   return GL_ELEMENT_ARRAY_BUFFER;
        case BufferType::Uniform: return GL_UNIFORM_BUFFER;
        default: return GL_ARRAY_BUFFER;
    }
}

GLenum OpenGLBuffer::GetGLUsage(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Static:  return GL_STATIC_DRAW;
        case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
        case BufferUsage::Stream:  return GL_STREAM_DRAW;
        default: return GL_STATIC_DRAW;
    }
}

GLenum OpenGLBuffer::GetGLAccess(MapAccess access) {
    switch (access) {
        case MapAccess::ReadOnly:  return GL_READ_ONLY;
        case MapAccess::WriteOnly: return GL_WRITE_ONLY;
        case MapAccess::ReadWrite: return GL_READ_WRITE;
        default: return GL_READ_WRITE;
    }
}

} // namespace Lupine
