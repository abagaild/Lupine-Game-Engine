#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include "GraphicsBackend.h"
#include "RenderState.h"

namespace Lupine {

// Forward declarations
class GraphicsBuffer;
class GraphicsTexture;
class GraphicsShader;
class GraphicsVertexArray;
struct RenderState;

// GraphicsBackend is now defined in GraphicsBackend.h

/**
 * @brief Buffer usage patterns
 */
enum class BufferUsage {
    Static,     // Data rarely changes
    Dynamic,    // Data changes frequently
    Stream      // Data changes every frame
};

/**
 * @brief Buffer types
 */
enum class BufferType {
    Vertex,     // Vertex buffer
    Index,      // Index buffer
    Uniform     // Uniform buffer
};

/**
 * @brief Texture formats
 */
enum class TextureFormat {
    R8,         // 8-bit red
    RG8,        // 8-bit red-green
    RGB8,       // 8-bit RGB
    RGBA8,      // 8-bit RGBA
    R16F,       // 16-bit float red
    RG16F,      // 16-bit float red-green
    RGB16F,     // 16-bit float RGB
    RGBA16F,    // 16-bit float RGBA
    R32F,       // 32-bit float red
    RG32F,      // 32-bit float red-green
    RGB32F,     // 32-bit float RGB
    RGBA32F,    // 32-bit float RGBA
    Depth16,    // 16-bit depth
    Depth24,    // 24-bit depth
    Depth32F    // 32-bit float depth
};

/**
 * @brief Texture wrap modes
 */
enum class TextureWrap {
    Repeat,         // Repeat texture
    ClampToEdge,    // Clamp to edge
    ClampToBorder,  // Clamp to border
    MirroredRepeat  // Mirrored repeat
};

/**
 * @brief Shader types
 */
enum class ShaderType {
    Vertex,     // Vertex shader
    Fragment,   // Fragment shader
    Geometry,   // Geometry shader (if supported)
    Compute     // Compute shader (if supported)
};

/**
 * @brief Primitive types for rendering
 */
enum class PrimitiveType {
    Triangles,      // Triangle list
    TriangleStrip,  // Triangle strip
    Lines,          // Line list
    LineStrip,      // Line strip
    Points          // Point list
};

// GraphicsCapabilities is now defined in GraphicsBackend.h

/**
 * @brief Abstract graphics device interface
 * 
 * This class provides a unified interface for different graphics backends
 * (OpenGL, WebGL, etc.) while maintaining high performance through
 * virtual function optimization and backend-specific implementations.
 */
class GraphicsDevice {
public:
    virtual ~GraphicsDevice() = default;

    /**
     * @brief Initialize the graphics device
     * @return True if initialization succeeded
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Shutdown the graphics device
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Get the graphics backend type
     * @return Graphics backend enumeration
     */
    virtual GraphicsBackend GetBackend() const = 0;

    /**
     * @brief Get device capabilities
     * @return Device capabilities structure
     */
    virtual const GraphicsCapabilities& GetCapabilities() const = 0;

    // === Buffer Management ===
    
    /**
     * @brief Create a graphics buffer
     * @param type Buffer type
     * @param usage Buffer usage pattern
     * @param size Buffer size in bytes
     * @param data Initial data (can be nullptr)
     * @return Shared pointer to graphics buffer
     */
    virtual std::shared_ptr<GraphicsBuffer> CreateBuffer(
        BufferType type, 
        BufferUsage usage, 
        size_t size, 
        const void* data = nullptr
    ) = 0;

    // === Texture Management ===
    
    /**
     * @brief Create a 2D texture
     * @param width Texture width
     * @param height Texture height
     * @param format Texture format
     * @param data Initial texture data (can be nullptr)
     * @return Shared pointer to graphics texture
     */
    virtual std::shared_ptr<GraphicsTexture> CreateTexture2D(
        int width, 
        int height, 
        TextureFormat format, 
        const void* data = nullptr
    ) = 0;

    /**
     * @brief Create a cubemap texture
     * @param size Cubemap face size
     * @param format Texture format
     * @param data Array of 6 face data pointers (can be nullptr)
     * @return Shared pointer to graphics texture
     */
    virtual std::shared_ptr<GraphicsTexture> CreateTextureCubemap(
        int size, 
        TextureFormat format, 
        const void* data[6] = nullptr
    ) = 0;

    // === Shader Management ===
    
    /**
     * @brief Create a shader program
     * @param vertex_source Vertex shader source code
     * @param fragment_source Fragment shader source code
     * @param geometry_source Geometry shader source (optional)
     * @return Shared pointer to graphics shader
     */
    virtual std::shared_ptr<GraphicsShader> CreateShader(
        const std::string& vertex_source,
        const std::string& fragment_source,
        const std::string& geometry_source = ""
    ) = 0;

    // === Vertex Array Management ===

    /**
     * @brief Create a vertex array object
     * @return Shared pointer to graphics vertex array
     */
    virtual std::shared_ptr<GraphicsVertexArray> CreateVertexArray() = 0;

    // === Rendering Commands ===

    /**
     * @brief Clear the framebuffer
     * @param color Clear color (RGBA)
     * @param clear_color Whether to clear color buffer
     * @param clear_depth Whether to clear depth buffer
     * @param clear_stencil Whether to clear stencil buffer
     */
    virtual void Clear(
        const glm::vec4& color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        bool clear_color = true,
        bool clear_depth = true,
        bool clear_stencil = false
    ) = 0;

    /**
     * @brief Set the viewport
     * @param x Viewport X coordinate
     * @param y Viewport Y coordinate
     * @param width Viewport width
     * @param height Viewport height
     */
    virtual void SetViewport(int x, int y, int width, int height) = 0;

    /**
     * @brief Draw primitives
     * @param primitive_type Type of primitives to draw
     * @param vertex_count Number of vertices to draw
     * @param first_vertex First vertex index
     */
    virtual void Draw(
        PrimitiveType primitive_type,
        int vertex_count,
        int first_vertex = 0
    ) = 0;

    /**
     * @brief Draw indexed primitives
     * @param primitive_type Type of primitives to draw
     * @param index_count Number of indices to draw
     * @param first_index First index
     */
    virtual void DrawIndexed(
        PrimitiveType primitive_type,
        int index_count,
        int first_index = 0
    ) = 0;

    // === State Management ===

    /**
     * @brief Enable or disable depth testing
     * @param enable Whether to enable depth testing
     */
    virtual void SetDepthTest(bool enable) = 0;

    /**
     * @brief Enable or disable depth writing
     * @param enable Whether to enable depth writing
     */
    virtual void SetDepthWrite(bool enable) = 0;

    /**
     * @brief Set depth comparison function
     * @param func Depth comparison function
     */
    virtual void SetDepthFunc(DepthFunc func) = 0;

    /**
     * @brief Enable or disable blending
     * @param enable Whether to enable blending
     */
    virtual void SetBlending(bool enable) = 0;

    /**
     * @brief Set blend function
     * @param src_factor Source blend factor
     * @param dst_factor Destination blend factor
     */
    virtual void SetBlendFunc(int src_factor, int dst_factor) = 0;

    /**
     * @brief Enable or disable face culling
     * @param enable Whether to enable face culling
     */
    virtual void SetCulling(bool enable) = 0;

    /**
     * @brief Set which faces to cull
     * @param front_face Whether to cull front faces
     */
    virtual void SetCullFace(bool front_face) = 0;

    // === Debug and Utility ===

    /**
     * @brief Check for graphics API errors
     * @return Error message if any, empty string if no error
     */
    virtual std::string CheckError() = 0;

    /**
     * @brief Get the current frame statistics
     * @return Frame statistics structure
     */
    struct FrameStats {
        int draw_calls = 0;
        int vertices_rendered = 0;
        int triangles_rendered = 0;
        float frame_time_ms = 0.0f;
    };

    virtual const FrameStats& GetFrameStats() const = 0;

    /**
     * @brief Reset frame statistics
     */
    virtual void ResetFrameStats() = 0;
};

} // namespace Lupine
