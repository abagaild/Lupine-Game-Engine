#pragma once

#include "GraphicsDevice.h"
#include "RenderState.h"
#include <memory>
#include <unordered_map>
#include <string>

// Forward declare OpenGL types to avoid including OpenGL headers in header
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef float GLfloat;

namespace Lupine {

// Forward declarations
class OpenGLBuffer;
class OpenGLTexture;
class OpenGLShader;
class OpenGLVertexArray;

// Using FrameStats from GraphicsDevice

/**
 * @brief OpenGL graphics device implementation
 * 
 * Implements the GraphicsDevice interface using OpenGL 3.3+ Core Profile.
 * Provides high-performance rendering with modern OpenGL features while
 * maintaining compatibility with the abstract graphics interface.
 */
class OpenGLDevice : public GraphicsDevice {
public:
    OpenGLDevice();
    virtual ~OpenGLDevice();

    // === GraphicsDevice Interface ===
    
    bool Initialize() override;
    void Shutdown() override;
    GraphicsBackend GetBackend() const override { return GraphicsBackend::OpenGL; }
    const GraphicsCapabilities& GetCapabilities() const override { return m_capabilities; }

    // Buffer Management
    std::shared_ptr<GraphicsBuffer> CreateBuffer(
        BufferType type, 
        BufferUsage usage, 
        size_t size, 
        const void* data = nullptr
    ) override;

    // Texture Management
    std::shared_ptr<GraphicsTexture> CreateTexture2D(
        int width, 
        int height, 
        TextureFormat format, 
        const void* data = nullptr
    ) override;

    std::shared_ptr<GraphicsTexture> CreateTextureCubemap(
        int size, 
        TextureFormat format, 
        const void* data[6] = nullptr
    ) override;

    // Shader Management
    std::shared_ptr<GraphicsShader> CreateShader(
        const std::string& vertex_source,
        const std::string& fragment_source,
        const std::string& geometry_source = ""
    ) override;

    // Vertex Array Management
    std::shared_ptr<GraphicsVertexArray> CreateVertexArray() override;

    // Rendering Commands
    void Clear(
        const glm::vec4& color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        bool clear_color = true,
        bool clear_depth = true,
        bool clear_stencil = false
    ) override;

    void SetViewport(int x, int y, int width, int height) override;

    void Draw(
        PrimitiveType primitive_type,
        int vertex_count,
        int first_vertex = 0
    ) override;

    void DrawIndexed(
        PrimitiveType primitive_type,
        int index_count,
        int first_index = 0
    ) override;

    // State Management
    void SetDepthTest(bool enable) override;
    void SetDepthWrite(bool enable) override;
    void SetDepthFunc(DepthFunc func) override;
    void SetBlending(bool enable) override;
    void SetBlendFunc(int src_factor, int dst_factor) override;
    void SetCulling(bool enable) override;
    void SetCullFace(bool front_face) override;

    // Debug and Utility
    std::string CheckError() override;
    const FrameStats& GetFrameStats() const override { return m_frame_stats; }
    void ResetFrameStats() override;

    // === OpenGL-Specific Methods ===

    /**
     * @brief Apply a complete render state
     * @param state Render state to apply
     */
    void ApplyRenderState(const RenderState& state);

    /**
     * @brief Get the current OpenGL context information
     * @return Context information string
     */
    std::string GetContextInfo() const;

    /**
     * @brief Check if an OpenGL extension is supported
     * @param extension Extension name
     * @return True if extension is supported
     */
    bool IsExtensionSupported(const std::string& extension) const;

    /**
     * @brief Get all supported OpenGL extensions
     * @return Vector of extension names
     */
    std::vector<std::string> GetSupportedExtensions() const;

    /**
     * @brief Enable or disable OpenGL debug output
     * @param enable Whether to enable debug output
     */
    void SetDebugOutput(bool enable);

    /**
     * @brief Convert TextureFormat to OpenGL internal format
     * @param format Texture format
     * @return OpenGL internal format
     */
    static GLenum GetGLInternalFormat(TextureFormat format);

    /**
     * @brief Convert TextureFormat to OpenGL format
     * @param format Texture format
     * @return OpenGL format
     */
    static GLenum GetGLFormat(TextureFormat format);

    /**
     * @brief Convert TextureFormat to OpenGL type
     * @param format Texture format
     * @return OpenGL type
     */
    static GLenum GetGLType(TextureFormat format);

    /**
     * @brief Convert BufferType to OpenGL target
     * @param type Buffer type
     * @return OpenGL buffer target
     */
    static GLenum GetGLBufferTarget(BufferType type);

    /**
     * @brief Convert BufferUsage to OpenGL usage
     * @param usage Buffer usage
     * @return OpenGL usage hint
     */
    static GLenum GetGLBufferUsage(BufferUsage usage);

    /**
     * @brief Convert PrimitiveType to OpenGL primitive
     * @param type Primitive type
     * @return OpenGL primitive type
     */
    static GLenum GetGLPrimitiveType(PrimitiveType type);

    /**
     * @brief Convert BlendFactor to OpenGL blend factor
     * @param factor Blend factor
     * @return OpenGL blend factor
     */
    static GLenum GetGLBlendFactor(BlendFactor factor);

    /**
     * @brief Convert BlendEquation to OpenGL blend equation
     * @param equation Blend equation
     * @return OpenGL blend equation
     */
    static GLenum GetGLBlendEquation(BlendEquation equation);

    /**
     * @brief Convert DepthFunc to OpenGL depth function
     * @param func Depth function
     * @return OpenGL depth function
     */
    static GLenum GetGLDepthFunc(DepthFunc func);

private:
    // Initialization helpers
    bool InitializeOpenGL();
    void QueryCapabilities();
    void SetupDebugCallback();

    // Utility methods
    GLenum ConvertPrimitiveType(PrimitiveType type) const;
    GLenum ConvertBlendFactor(BlendFactor factor) const;
    GLenum ConvertDepthFunc(DepthFunc func) const;
    void UpdateFrameStats();
    void TrackDrawCall(int vertices);
    
    // OpenGL state caching
    struct GLState {
        bool depth_test = false;
        bool depth_write = true;
        GLenum depth_func = 0x0201; // GL_LESS
        bool blending = false;
        bool culling = false;
        GLenum cull_face = 0;
        GLenum blend_src = 0;
        GLenum blend_dst = 0;
        GLuint bound_vao = 0;
        GLuint bound_program = 0;
        std::unordered_map<GLenum, GLuint> bound_buffers;
        std::unordered_map<int, GLuint> bound_textures;
    };

    GraphicsCapabilities m_capabilities;
    FrameStats m_frame_stats;
    GLState m_gl_state;
    bool m_initialized = false;
    bool m_debug_enabled = false;
    
    // Extension support
    std::vector<std::string> m_extensions;
    std::unordered_map<std::string, bool> m_extension_cache;
};

} // namespace Lupine
