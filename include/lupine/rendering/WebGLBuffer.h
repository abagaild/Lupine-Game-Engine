#pragma once

#include "GraphicsBuffer.h"

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

/**
 * @brief WebGL implementation of GraphicsBuffer
 * 
 * Provides WebGL-specific buffer functionality while implementing
 * the abstract GraphicsBuffer interface for cross-platform compatibility.
 * Uses GLES3/WebGL 2.0 buffer operations with appropriate fallbacks.
 */
class WebGLBuffer : public GraphicsBuffer {
public:
    /**
     * @brief Constructor
     * @param type Buffer type
     * @param usage Buffer usage pattern
     * @param size Buffer size in bytes
     * @param data Initial data (can be nullptr)
     */
    WebGLBuffer(BufferType type, BufferUsage usage, size_t size, const void* data = nullptr);

    /**
     * @brief Destructor - cleans up WebGL buffer
     */
    virtual ~WebGLBuffer();

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

    // === WebGL-specific methods ===

    /**
     * @brief Get the WebGL buffer ID
     * @return WebGL buffer ID
     */
    GLuint GetBufferID() const { return m_buffer_id; }

    /**
     * @brief Get the WebGL buffer target
     * @return WebGL buffer target (GL_ARRAY_BUFFER, etc.)
     */
    GLenum GetTarget() const { return m_target; }

    /**
     * @brief Check if buffer mapping is supported
     * @return True if mapping is supported in current WebGL context
     */
    bool IsMappingSupported() const;

private:
    // Initialization
    bool Initialize(const void* data);

    // WebGL state
    GLuint m_buffer_id;
    GLenum m_target;
    BufferType m_type;
    BufferUsage m_usage;
    size_t m_size;
    bool m_is_mapped;

    // Mapping fallback for WebGL (since mapping is limited)
    void* m_mapped_data;
    MapAccess m_map_access;

    // Static conversion methods
    static GLenum GetGLTarget(BufferType type);
    static GLenum GetGLUsage(BufferUsage usage);
};

} // namespace Lupine
