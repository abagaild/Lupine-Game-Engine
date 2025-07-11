#pragma once

#include "GraphicsDevice.h"
#include "RenderState.h"
#include <memory>
#include <unordered_map>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#else
// Forward declare WebGL types for non-Emscripten builds
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef float GLfloat;
#endif

namespace Lupine {

// Forward declarations
class WebGLBuffer;
class WebGLTexture;
class WebGLShader;
class WebGLVertexArray;

// Using FrameStats from GraphicsDevice

/**
 * @brief WebGL graphics device implementation
 * 
 * Implements the GraphicsDevice interface using WebGL 2.0 for web platforms.
 * Provides web-optimized rendering with WebGL-specific features and limitations
 * while maintaining compatibility with the abstract graphics interface.
 */
class WebGLDevice : public GraphicsDevice {
public:
    WebGLDevice();
    virtual ~WebGLDevice();

    // === GraphicsDevice Interface ===
    
    bool Initialize() override;
    void Shutdown() override;
    GraphicsBackend GetBackend() const override { return GraphicsBackend::WebGL; }
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

    // === WebGL-Specific Methods ===

    /**
     * @brief Get the WebGL context handle
     * @return WebGL context handle
     */
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE GetWebGLContext() const { return m_webgl_context; }
#else
    int GetWebGLContext() const { return m_webgl_context; }
#endif

    /**
     * @brief Check if a WebGL extension is supported
     * @param extension Extension name
     * @return True if extension is supported
     */
    bool IsExtensionSupported(const std::string& extension) const;

    /**
     * @brief Get all supported WebGL extensions
     * @return Vector of extension names
     */
    std::vector<std::string> GetSupportedExtensions() const;

    /**
     * @brief Convert shader source from GLSL to GLSL ES
     * @param source Original GLSL source
     * @param type Shader type
     * @return GLSL ES compatible source
     */
    std::string ConvertShaderToGLSLES(const std::string& source, GLenum type) const;

    /**
     * @brief Get WebGL-specific performance information
     * @return Performance info string
     */
    std::string GetPerformanceInfo() const;

private:
    // Initialization helpers
    bool InitializeWebGL();
    void QueryCapabilities();
    void DetectExtensions();
    
    // State tracking
    void UpdateFrameStats();
    void TrackDrawCall(int vertices);

    // Utility methods
    GLenum ConvertDepthFunc(DepthFunc func) const;
    
    // WebGL state caching
    struct WebGLState {
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

    // Utility methods
    GLenum ConvertPrimitiveType(PrimitiveType type) const;
    GLenum ConvertBlendFactor(BlendFactor factor) const;
    std::string GetWebGLErrorString(GLenum error) const;

    GraphicsCapabilities m_capabilities;
    FrameStats m_frame_stats;
    WebGLState m_webgl_state;
    bool m_initialized = false;
    
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE m_webgl_context = 0;
#else
    int m_webgl_context = 0; // Placeholder for non-Emscripten builds
#endif
    
    // Extension support
    std::vector<std::string> m_extensions;
    std::unordered_map<std::string, bool> m_extension_cache;
    
    // WebGL-specific capabilities
    bool m_supports_webgl2 = false;
    bool m_supports_instancing = false;
    bool m_supports_vao = false;
    bool m_supports_depth_texture = false;
    bool m_supports_float_textures = false;
    bool m_supports_anisotropic_filtering = false;
};

} // namespace Lupine
