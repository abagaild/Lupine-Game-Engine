#pragma once

#include "GraphicsVertexArray.h"
#include <glad/glad.h>
#include <vector>
#include <memory>

namespace Lupine {

// Forward declarations
class OpenGLBuffer;

/**
 * @brief OpenGL implementation of GraphicsVertexArray
 * 
 * Provides OpenGL-specific vertex array functionality while implementing
 * the abstract GraphicsVertexArray interface for cross-platform compatibility.
 */
class OpenGLVertexArray : public GraphicsVertexArray {
public:
    /**
     * @brief Constructor
     */
    OpenGLVertexArray();

    /**
     * @brief Destructor - cleans up OpenGL vertex array
     */
    virtual ~OpenGLVertexArray();

    // === GraphicsVertexArray Interface ===

    void Bind() override;
    void Unbind() override;

    void SetVertexBuffer(
        std::shared_ptr<GraphicsBuffer> buffer,
        const VertexLayout& layout
    ) override;

    void SetIndexBuffer(std::shared_ptr<GraphicsBuffer> buffer) override;

    void AddVertexBuffer(
        std::shared_ptr<GraphicsBuffer> buffer,
        const VertexLayout& layout,
        int binding_index = 0
    ) override;

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
    bool IsValid() const override { return m_vao_id != 0; }

    // Utility methods
    int GetVertexCount() const override;
    int GetIndexCount() const override;
    size_t GetMemoryUsage() const override;
    bool Validate() const override;
    std::string GetDebugInfo() const override;

    // === OpenGL-specific methods ===

    /**
     * @brief Get the OpenGL VAO ID
     * @return OpenGL VAO ID
     */
    GLuint GetVAOID() const { return m_vao_id; }

    /**
     * @brief Convert vertex attribute type to OpenGL type
     * @param type Vertex attribute type
     * @return OpenGL type
     */
    static GLenum GetGLType(uint32_t type);

private:
    GLuint m_vao_id;
    std::vector<std::shared_ptr<GraphicsBuffer>> m_vertex_buffers;
    std::shared_ptr<GraphicsBuffer> m_index_buffer;
    std::vector<VertexLayout> m_vertex_layouts;
    int m_vertex_count;
    int m_index_count;

    /**
     * @brief Initialize the OpenGL vertex array
     * @return True if successful
     */
    bool Initialize();

    /**
     * @brief Setup vertex attributes for a buffer
     * @param buffer_index Index of the vertex buffer
     * @param layout Vertex layout for the buffer
     */
    void SetupVertexAttributes(int buffer_index, const VertexLayout& layout);
};

} // namespace Lupine
