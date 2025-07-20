#include "lupine/rendering/OpenGLVertexArray.h"
#include "lupine/rendering/OpenGLBuffer.h"
#include <iostream>
#include <sstream>

namespace Lupine {

OpenGLVertexArray::OpenGLVertexArray()
    : m_vao_id(0)
    , m_vertex_count(0)
    , m_index_count(0) {
    
    if (!Initialize()) {
        std::cerr << "Failed to initialize OpenGL vertex array" << std::endl;
    }
}

OpenGLVertexArray::~OpenGLVertexArray() {
    if (m_vao_id != 0) {
        glDeleteVertexArrays(1, &m_vao_id);
        m_vao_id = 0;
    }
}

void OpenGLVertexArray::Bind() {
    if (IsValid()) {
        glBindVertexArray(m_vao_id);
    }
}

void OpenGLVertexArray::Unbind() {
    glBindVertexArray(0);
}

void OpenGLVertexArray::SetVertexBuffer(
    std::shared_ptr<GraphicsBuffer> buffer,
    const VertexLayout& layout) {
    
    if (!buffer || !buffer->IsValid()) {
        std::cerr << "Invalid vertex buffer provided" << std::endl;
        return;
    }
    
    // Clear existing buffers and set this as the primary one
    m_vertex_buffers.clear();
    m_vertex_layouts.clear();
    
    m_vertex_buffers.push_back(buffer);
    m_vertex_layouts.push_back(layout);
    
    // Calculate vertex count
    if (layout.GetStride() > 0) {
        m_vertex_count = static_cast<int>(buffer->GetSize() / layout.GetStride());
    }
    
    Bind();
    SetupVertexAttributes(0, layout);
    Unbind();
}

void OpenGLVertexArray::SetIndexBuffer(std::shared_ptr<GraphicsBuffer> buffer) {
    if (!buffer || !buffer->IsValid()) {
        std::cerr << "Invalid index buffer provided" << std::endl;
        return;
    }
    
    if (buffer->GetType() != BufferType::Index) {
        std::cerr << "Buffer is not an index buffer" << std::endl;
        return;
    }
    
    m_index_buffer = buffer;
    
    // Calculate index count (assuming 32-bit indices)
    m_index_count = static_cast<int>(buffer->GetSize() / sizeof(uint32_t));
    
    Bind();
    buffer->Bind();
    Unbind();
}

void OpenGLVertexArray::AddVertexBuffer(
    std::shared_ptr<GraphicsBuffer> buffer,
    const VertexLayout& layout,
    int binding_index) {
    
    if (!buffer || !buffer->IsValid()) {
        std::cerr << "Invalid vertex buffer provided" << std::endl;
        return;
    }
    
    int buffer_index = static_cast<int>(m_vertex_buffers.size());
    m_vertex_buffers.push_back(buffer);
    m_vertex_layouts.push_back(layout);
    
    Bind();
    SetupVertexAttributes(buffer_index, layout);
    Unbind();
}

void OpenGLVertexArray::SetAttributePointer(
    int location,
    int components,
    uint32_t type,
    bool normalized,
    int stride,
    int offset) {
    
    Bind();
    
    glVertexAttribPointer(
        location,
        components,
        GetGLType(type),
        normalized ? GL_TRUE : GL_FALSE,
        stride,
        reinterpret_cast<void*>(offset)
    );
    
    glEnableVertexAttribArray(location);
    
    Unbind();
}

void OpenGLVertexArray::EnableAttribute(int location) {
    Bind();
    glEnableVertexAttribArray(location);
    Unbind();
}

void OpenGLVertexArray::DisableAttribute(int location) {
    Bind();
    glDisableVertexAttribArray(location);
    Unbind();
}

void OpenGLVertexArray::SetAttributeDivisor(int location, int divisor) {
    Bind();
    glVertexAttribDivisor(location, divisor);
    Unbind();
}

int OpenGLVertexArray::GetVertexCount() const {
    return m_vertex_count;
}

int OpenGLVertexArray::GetIndexCount() const {
    return m_index_count;
}

size_t OpenGLVertexArray::GetMemoryUsage() const {
    size_t total = 0;
    
    for (const auto& buffer : m_vertex_buffers) {
        if (buffer) {
            total += buffer->GetSize();
        }
    }
    
    if (m_index_buffer) {
        total += m_index_buffer->GetSize();
    }
    
    return total;
}

bool OpenGLVertexArray::Validate() const {
    if (!IsValid()) {
        return false;
    }
    
    if (m_vertex_buffers.empty()) {
        return false;
    }
    
    // Check that all vertex buffers are valid
    for (const auto& buffer : m_vertex_buffers) {
        if (!buffer || !buffer->IsValid()) {
            return false;
        }
    }
    
    // Check index buffer if present
    if (m_index_buffer && !m_index_buffer->IsValid()) {
        return false;
    }
    
    return true;
}

std::string OpenGLVertexArray::GetDebugInfo() const {
    std::ostringstream oss;
    oss << "OpenGLVertexArray[ID=" << m_vao_id << "]:\n";
    oss << "  Vertex Buffers: " << m_vertex_buffers.size() << "\n";
    oss << "  Vertex Count: " << m_vertex_count << "\n";
    oss << "  Index Count: " << m_index_count << "\n";
    oss << "  Memory Usage: " << GetMemoryUsage() << " bytes\n";
    oss << "  Valid: " << (IsValid() ? "Yes" : "No") << "\n";
    
    for (size_t i = 0; i < m_vertex_buffers.size(); ++i) {
        const auto& buffer = m_vertex_buffers[i];
        if (buffer) {
            oss << "  Buffer " << i << ": " << buffer->GetSize() << " bytes\n";
        }
    }
    
    if (m_index_buffer) {
        oss << "  Index Buffer: " << m_index_buffer->GetSize() << " bytes\n";
    }
    
    return oss.str();
}

bool OpenGLVertexArray::Initialize() {
    glGenVertexArrays(1, &m_vao_id);
    return m_vao_id != 0;
}

void OpenGLVertexArray::SetupVertexAttributes(int buffer_index, const VertexLayout& layout) {
    if (buffer_index >= static_cast<int>(m_vertex_buffers.size())) {
        return;
    }
    
    auto buffer = m_vertex_buffers[buffer_index];
    if (!buffer) {
        return;
    }
    
    buffer->Bind();
    
    const auto& attributes = layout.GetAttributes();
    uint32_t offset = 0;
    
    for (const auto& attr : attributes) {
        glVertexAttribPointer(
            attr.location,
            attr.components,
            GetGLType(attr.type),
            attr.normalized ? GL_TRUE : GL_FALSE,
            layout.GetStride(),
            reinterpret_cast<void*>(offset)
        );
        
        glEnableVertexAttribArray(attr.location);
        
        // Calculate size of this attribute
        uint32_t type_size = 4; // Default to 4 bytes (float)
        switch (attr.type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
                type_size = 1;
                break;
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
                type_size = 2;
                break;
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_FLOAT:
                type_size = 4;
                break;
            case GL_DOUBLE:
                type_size = 8;
                break;
        }
        
        offset += attr.components * type_size;
    }
}

GLenum OpenGLVertexArray::GetGLType(uint32_t type) {
    // The type parameter should already be an OpenGL type
    // This is a pass-through for now, but could include validation
    return static_cast<GLenum>(type);
}

std::shared_ptr<GraphicsBuffer> OpenGLVertexArray::GetVertexBuffer(int binding_index) const {
    if (binding_index < 0 || binding_index >= static_cast<int>(m_vertex_buffers.size())) {
        return nullptr;
    }
    return m_vertex_buffers[binding_index];
}

std::shared_ptr<GraphicsBuffer> OpenGLVertexArray::GetIndexBuffer() const {
    return m_index_buffer;
}

int OpenGLVertexArray::GetVertexBufferCount() const {
    return static_cast<int>(m_vertex_buffers.size());
}

bool OpenGLVertexArray::HasIndexBuffer() const {
    return m_index_buffer != nullptr;
}

VertexLayout OpenGLVertexArray::GetVertexLayout(int binding_index) const {
    if (binding_index < 0 || binding_index >= static_cast<int>(m_vertex_layouts.size())) {
        return VertexLayout(); // Return empty layout
    }
    return m_vertex_layouts[binding_index];
}

} // namespace Lupine
