#pragma once

#include "GraphicsVertexArray.h"
#include <vector>
#include <memory>

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

class GraphicsBuffer;

/**
 * @brief WebGL implementation of GraphicsVertexArray
 * 
 * Provides WebGL-specific vertex array functionality while implementing
 * the abstract GraphicsVertexArray interface for cross-platform compatibility.
 * Uses VAO when available in WebGL 2.0, otherwise falls back to manual attribute management.
 */
class WebGLVertexArray : public GraphicsVertexArray {
public:
    /**
     * @brief Constructor
     */
    WebGLVertexArray();

    /**
     * @brief Destructor - cleans up WebGL vertex array
     */
    virtual ~WebGLVertexArray();

    // === GraphicsVertexArray Interface ===

    void Bind() override;
    void Unbind() override;

    void SetVertexBuffer(
        std::shared_ptr<GraphicsBuffer> buffer,
        const VertexLayout& layout
    ) override;

    void AddVertexBuffer(
        std::shared_ptr<GraphicsBuffer> buffer,
        const VertexLayout& layout,
        int binding_index = 0
    ) override;

    void SetIndexBuffer(std::shared_ptr<GraphicsBuffer> buffer) override;

    void SetAttributePointer(
        int location,
        int components,
        uint32_t type,
        bool normalized,
        int stride,
        int offset
    ) override;

    void EnableAttribute(int location) override;
    void DisableAttribute(int location) override;

    void SetAttributeDivisor(int location, int divisor) override;

    // Buffer access methods
    std::shared_ptr<GraphicsBuffer> GetVertexBuffer(int binding_index = 0) const override;
    std::shared_ptr<GraphicsBuffer> GetIndexBuffer() const override;
    int GetVertexBufferCount() const override;
    bool HasIndexBuffer() const override;
    VertexLayout GetVertexLayout(int binding_index = 0) const override;

    uint32_t GetNativeHandle() const override { return m_vao_id; }
    bool IsValid() const override;

    // Utility methods
    int GetVertexCount() const override;
    int GetIndexCount() const override;
    size_t GetMemoryUsage() const override;
    bool Validate() const override;
    std::string GetDebugInfo() const override;

    // === WebGL-specific methods ===

    /**
     * @brief Get the WebGL VAO ID (if VAO is supported)
     * @return WebGL VAO ID, 0 if VAO not supported
     */
    GLuint GetVAOID() const { return m_vao_id; }

    /**
     * @brief Check if VAO is supported in current WebGL context
     * @return True if VAO is supported
     */
    bool IsVAOSupported() const { return m_vao_supported; }

private:
    // Initialization
    bool Initialize();
    void SetupVertexAttributes(int buffer_index, const VertexLayout& layout);

    // WebGL state
    GLuint m_vao_id;
    bool m_vao_supported;
    int m_vertex_count;
    int m_index_count;

    // Buffer storage
    std::vector<std::shared_ptr<GraphicsBuffer>> m_vertex_buffers;
    std::vector<VertexLayout> m_vertex_layouts;
    std::shared_ptr<GraphicsBuffer> m_index_buffer;

    // Fallback attribute state (when VAO is not available)
    struct AttributeState {
        bool enabled = false;
        int components = 0;
        GLenum type = 0;
        bool normalized = false;
        int stride = 0;
        const void* offset = nullptr;
        int divisor = 0;
    };
    std::vector<AttributeState> m_attribute_states;

    // Utility methods
    static GLenum GetGLType(uint32_t type);
    void ApplyVertexAttributes();
    void RestoreVertexAttributes();
};

} // namespace Lupine
