#include "lupine/rendering/WebGLVertexArray.h"
#include "lupine/rendering/WebGLBuffer.h"
#include <iostream>
#include <sstream>

namespace Lupine {

WebGLVertexArray::WebGLVertexArray()
    : m_vao_id(0)
    , m_vao_supported(false)
    , m_vertex_count(0)
    , m_index_count(0) {
    
    if (!Initialize()) {
        std::cerr << "Failed to initialize WebGL vertex array" << std::endl;
    }
}

WebGLVertexArray::~WebGLVertexArray() {
#ifdef __EMSCRIPTEN__
    if (m_vao_id != 0 && m_vao_supported) {
        glDeleteVertexArrays(1, &m_vao_id);
        m_vao_id = 0;
    }
#endif
}

void WebGLVertexArray::Bind() {
#ifdef __EMSCRIPTEN__
    if (m_vao_supported && m_vao_id != 0) {
        glBindVertexArray(m_vao_id);
    } else {
        // Fallback: manually apply vertex attributes
        ApplyVertexAttributes();
    }
#endif
}

void WebGLVertexArray::Unbind() {
#ifdef __EMSCRIPTEN__
    if (m_vao_supported) {
        glBindVertexArray(0);
    } else {
        // Fallback: restore previous vertex attribute state
        RestoreVertexAttributes();
    }
#endif
}

void WebGLVertexArray::SetVertexBuffer(
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

void WebGLVertexArray::AddVertexBuffer(
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

void WebGLVertexArray::SetIndexBuffer(std::shared_ptr<GraphicsBuffer> buffer) {
    if (!buffer || !buffer->IsValid()) {
        std::cerr << "Invalid index buffer provided" << std::endl;
        return;
    }
    
    m_index_buffer = buffer;
    
    // Calculate index count (assuming 16-bit indices for WebGL compatibility)
    m_index_count = static_cast<int>(buffer->GetSize() / sizeof(uint16_t));
    
#ifdef __EMSCRIPTEN__
    Bind();
    buffer->Bind(); // Bind the index buffer
    Unbind();
#endif
}

void WebGLVertexArray::SetAttributePointer(
    int location,
    int components,
    uint32_t type,
    bool normalized,
    int stride,
    int offset) {
    
#ifdef __EMSCRIPTEN__
    if (location < 0) {
        return;
    }
    
    // Ensure attribute states vector is large enough
    if (static_cast<size_t>(location) >= m_attribute_states.size()) {
        m_attribute_states.resize(location + 1);
    }
    
    // Store attribute state
    AttributeState& state = m_attribute_states[location];
    state.components = components;
    state.type = GetGLType(type);
    state.normalized = normalized;
    state.stride = stride;
    state.offset = reinterpret_cast<const void*>(static_cast<uintptr_t>(offset));
    
    // Apply immediately if bound
    glVertexAttribPointer(location, components, state.type, 
                         normalized ? GL_TRUE : GL_FALSE, 
                         stride, state.offset);
#endif
}

void WebGLVertexArray::EnableAttribute(int location) {
#ifdef __EMSCRIPTEN__
    if (location < 0) {
        return;
    }
    
    // Ensure attribute states vector is large enough
    if (static_cast<size_t>(location) >= m_attribute_states.size()) {
        m_attribute_states.resize(location + 1);
    }
    
    m_attribute_states[location].enabled = true;
    glEnableVertexAttribArray(location);
#endif
}

void WebGLVertexArray::DisableAttribute(int location) {
#ifdef __EMSCRIPTEN__
    if (location < 0) {
        return;
    }
    
    // Ensure attribute states vector is large enough
    if (static_cast<size_t>(location) >= m_attribute_states.size()) {
        m_attribute_states.resize(location + 1);
    }
    
    m_attribute_states[location].enabled = false;
    glDisableVertexAttribArray(location);
#endif
}

void WebGLVertexArray::SetAttributeDivisor(int location, int divisor) {
#ifdef __EMSCRIPTEN__
    if (location < 0) {
        return;
    }
    
    // Ensure attribute states vector is large enough
    if (static_cast<size_t>(location) >= m_attribute_states.size()) {
        m_attribute_states.resize(location + 1);
    }
    
    m_attribute_states[location].divisor = divisor;
    
    // WebGL 2.0 supports instancing
    glVertexAttribDivisor(location, divisor);
#endif
}

std::shared_ptr<GraphicsBuffer> WebGLVertexArray::GetVertexBuffer(int binding_index) const {
    if (binding_index >= 0 && binding_index < static_cast<int>(m_vertex_buffers.size())) {
        return m_vertex_buffers[binding_index];
    }
    return nullptr;
}

std::shared_ptr<GraphicsBuffer> WebGLVertexArray::GetIndexBuffer() const {
    return m_index_buffer;
}

int WebGLVertexArray::GetVertexBufferCount() const {
    return static_cast<int>(m_vertex_buffers.size());
}

bool WebGLVertexArray::HasIndexBuffer() const {
    return m_index_buffer != nullptr;
}

VertexLayout WebGLVertexArray::GetVertexLayout(int binding_index) const {
    if (binding_index >= 0 && binding_index < static_cast<int>(m_vertex_layouts.size())) {
        return m_vertex_layouts[binding_index];
    }
    return VertexLayout();
}

bool WebGLVertexArray::IsValid() const {
    if (m_vao_supported) {
        return m_vao_id != 0;
    } else {
        // For fallback mode, we're valid if we have at least one vertex buffer
        return !m_vertex_buffers.empty();
    }
}

int WebGLVertexArray::GetVertexCount() const {
    return m_vertex_count;
}

int WebGLVertexArray::GetIndexCount() const {
    return m_index_count;
}

size_t WebGLVertexArray::GetMemoryUsage() const {
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

bool WebGLVertexArray::Validate() const {
    // Check if we have at least one vertex buffer
    if (m_vertex_buffers.empty()) {
        return false;
    }
    
    // Check if all vertex buffers are valid
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

std::string WebGLVertexArray::GetDebugInfo() const {
    std::stringstream ss;
    ss << "WebGL Vertex Array:\n";
    ss << "  VAO Supported: " << (m_vao_supported ? "Yes" : "No") << "\n";
    ss << "  VAO ID: " << m_vao_id << "\n";
    ss << "  Vertex Buffers: " << m_vertex_buffers.size() << "\n";
    ss << "  Vertex Count: " << m_vertex_count << "\n";
    ss << "  Index Buffer: " << (m_index_buffer ? "Yes" : "No") << "\n";
    ss << "  Index Count: " << m_index_count << "\n";
    ss << "  Memory Usage: " << GetMemoryUsage() << " bytes\n";
    ss << "  Valid: " << (IsValid() ? "Yes" : "No") << "\n";
    return ss.str();
}

bool WebGLVertexArray::Initialize() {
#ifdef __EMSCRIPTEN__
    // Check if VAO is supported (WebGL 2.0 feature)
    // In WebGL 2.0, VAO is part of the core specification
    m_vao_supported = true; // Assume WebGL 2.0 for now

    if (m_vao_supported) {
        glGenVertexArrays(1, &m_vao_id);
        if (m_vao_id == 0) {
            std::cerr << "Failed to create WebGL vertex array object" << std::endl;
            m_vao_supported = false;
        }
    }

    return true;
#else
    // For non-Emscripten builds, just mark as valid
    m_vao_id = 1; // Fake ID for non-WebGL builds
    m_vao_supported = false; // No VAO support in stub mode
    return true;
#endif
}

void WebGLVertexArray::SetupVertexAttributes(int buffer_index, const VertexLayout& layout) {
#ifdef __EMSCRIPTEN__
    if (buffer_index >= static_cast<int>(m_vertex_buffers.size())) {
        return;
    }

    auto buffer = m_vertex_buffers[buffer_index];
    if (!buffer) {
        return;
    }

    // Bind the vertex buffer
    buffer->Bind();

    // Setup vertex attributes based on layout
    const auto& attributes = layout.GetAttributes();
    uint32_t offset = 0;

    for (const auto& attr : attributes) {
        if (attr.location >= 0) {
            // Enable the attribute
            glEnableVertexAttribArray(attr.location);

            // Set the attribute pointer
            GLenum gl_type = GetGLType(attr.type);
            glVertexAttribPointer(
                attr.location,
                attr.components,
                gl_type,
                attr.normalized ? GL_TRUE : GL_FALSE,
                layout.GetStride(),
                reinterpret_cast<const void*>(offset)
            );

            // Store attribute state for fallback mode
            if (static_cast<size_t>(attr.location) >= m_attribute_states.size()) {
                m_attribute_states.resize(attr.location + 1);
            }

            AttributeState& state = m_attribute_states[attr.location];
            state.enabled = true;
            state.components = attr.components;
            state.type = gl_type;
            state.normalized = attr.normalized;
            state.stride = layout.GetStride();
            state.offset = reinterpret_cast<const void*>(offset);
        }

        // Calculate size of this attribute
        uint32_t attr_size = 0;
        switch (attr.type) {
            case 0x1406: // GL_FLOAT
                attr_size = sizeof(float) * attr.components;
                break;
            case 0x1401: // GL_UNSIGNED_BYTE
                attr_size = sizeof(uint8_t) * attr.components;
                break;
            case 0x1403: // GL_UNSIGNED_SHORT
                attr_size = sizeof(uint16_t) * attr.components;
                break;
            case 0x1405: // GL_UNSIGNED_INT
                attr_size = sizeof(uint32_t) * attr.components;
                break;
            default:
                attr_size = sizeof(float) * attr.components; // Default to float
                break;
        }

        offset += attr_size;
    }

    buffer->Unbind();
#endif
}

GLenum WebGLVertexArray::GetGLType(uint32_t type) {
#ifdef __EMSCRIPTEN__
    switch (type) {
        case 0x1400: return GL_BYTE;           // GL_BYTE
        case 0x1401: return GL_UNSIGNED_BYTE;  // GL_UNSIGNED_BYTE
        case 0x1402: return GL_SHORT;          // GL_SHORT
        case 0x1403: return GL_UNSIGNED_SHORT; // GL_UNSIGNED_SHORT
        case 0x1404: return GL_INT;            // GL_INT
        case 0x1405: return GL_UNSIGNED_INT;   // GL_UNSIGNED_INT
        case 0x1406: return GL_FLOAT;          // GL_FLOAT
        default:     return GL_FLOAT;          // Default to float
    }
#else
    return 0;
#endif
}

void WebGLVertexArray::ApplyVertexAttributes() {
#ifdef __EMSCRIPTEN__
    // Bind vertex buffers and apply attributes manually
    for (size_t i = 0; i < m_vertex_buffers.size(); ++i) {
        if (m_vertex_buffers[i]) {
            m_vertex_buffers[i]->Bind();
            SetupVertexAttributes(static_cast<int>(i), m_vertex_layouts[i]);
        }
    }

    // Bind index buffer if present
    if (m_index_buffer) {
        m_index_buffer->Bind();
    }
#endif
}

void WebGLVertexArray::RestoreVertexAttributes() {
#ifdef __EMSCRIPTEN__
    // Unbind all buffers
    for (const auto& buffer : m_vertex_buffers) {
        if (buffer) {
            buffer->Unbind();
        }
    }

    if (m_index_buffer) {
        m_index_buffer->Unbind();
    }

    // Disable all enabled attributes
    for (size_t i = 0; i < m_attribute_states.size(); ++i) {
        if (m_attribute_states[i].enabled) {
            glDisableVertexAttribArray(static_cast<GLuint>(i));
        }
    }
#endif
}

} // namespace Lupine
