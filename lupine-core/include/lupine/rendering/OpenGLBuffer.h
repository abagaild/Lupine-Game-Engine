#pragma once

#include "GraphicsBuffer.h"
#include <glad/glad.h>

namespace Lupine {

/**
 * @brief OpenGL implementation of GraphicsBuffer
 * 
 * Provides OpenGL-specific buffer functionality while implementing
 * the abstract GraphicsBuffer interface for cross-platform compatibility.
 */
class OpenGLBuffer : public GraphicsBuffer {
public:
    /**
     * @brief Constructor
     * @param type Buffer type
     * @param usage Buffer usage pattern
     * @param size Buffer size in bytes
     * @param data Initial data (can be nullptr)
     */
    OpenGLBuffer(BufferType type, BufferUsage usage, size_t size, const void* data = nullptr);

    /**
     * @brief Destructor - cleans up OpenGL buffer
     */
    virtual ~OpenGLBuffer();

    // === GraphicsBuffer Interface ===

    BufferType GetType() const override { return m_type; }
    BufferUsage GetUsage() const override { return m_usage; }
    size_t GetSize() const override { return m_size; }

    void Bind() override;
    void Unbind() override;

    void UpdateData(size_t offset, size_t size, const void* data) override;
    void UpdateData(const void* data, size_t size) override;

    void* Map(MapAccess access) override;
    void Unmap() override;

    uint32_t GetNativeHandle() const override { return m_buffer_id; }
    bool IsValid() const override { return m_buffer_id != 0; }

    // === OpenGL-specific methods ===

    /**
     * @brief Get the OpenGL buffer ID
     * @return OpenGL buffer ID
     */
    GLuint GetBufferID() const { return m_buffer_id; }

    /**
     * @brief Get the OpenGL buffer target
     * @return OpenGL buffer target (GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, etc.)
     */
    GLenum GetTarget() const { return m_target; }

    /**
     * @brief Convert BufferType to OpenGL target
     * @param type Buffer type
     * @return OpenGL buffer target
     */
    static GLenum GetGLTarget(BufferType type);

    /**
     * @brief Convert BufferUsage to OpenGL usage
     * @param usage Buffer usage
     * @return OpenGL usage hint
     */
    static GLenum GetGLUsage(BufferUsage usage);

    /**
     * @brief Convert MapAccess to OpenGL access
     * @param access Map access mode
     * @return OpenGL access mode
     */
    static GLenum GetGLAccess(MapAccess access);

private:
    GLuint m_buffer_id;
    GLenum m_target;
    BufferType m_type;
    BufferUsage m_usage;
    size_t m_size;
    bool m_is_mapped;

    /**
     * @brief Initialize the OpenGL buffer
     * @param data Initial buffer data
     * @return True if successful
     */
    bool Initialize(const void* data);
};

} // namespace Lupine
