#include "lupine/rendering/WebGLDevice.h"
#include "lupine/rendering/WebGLBuffer.h"
#include "lupine/rendering/WebGLTexture.h"
#include "lupine/rendering/WebGLShader.h"
#include "lupine/rendering/WebGLVertexArray.h"
#include "lupine/rendering/RenderState.h"
#include <iostream>
#include <cassert>
#include <cstdint>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#else
// Stub implementations for non-Emscripten builds
#define GL_NO_ERROR 0
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_CULL_FACE 0x0B44
#define GL_TRIANGLES 0x0004
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#endif

namespace Lupine {

// Forward declarations for WebGL-specific implementations
class WebGLBuffer;
class WebGLTexture;
class WebGLShader;
class WebGLVertexArray;

WebGLDevice::WebGLDevice() 
    : m_initialized(false) {
    // Initialize capabilities with default values
    m_capabilities.backend = GraphicsBackend::WebGL;
    m_capabilities.backend_name = "WebGL";
}

WebGLDevice::~WebGLDevice() {
    if (m_initialized) {
        Shutdown();
    }
}

bool WebGLDevice::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing WebGL Device..." << std::endl;

#ifdef __EMSCRIPTEN__
    // Initialize WebGL context
    if (!InitializeWebGL()) {
        std::cerr << "Failed to initialize WebGL" << std::endl;
        return false;
    }

    // Query device capabilities
    QueryCapabilities();

    // Detect available extensions
    DetectExtensions();

    // Set default WebGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize state tracking
    m_webgl_state = {};

    m_initialized = true;
    std::cout << "WebGL Device initialized successfully" << std::endl;
    std::cout << "Renderer: " << m_capabilities.renderer_name << std::endl;
    std::cout << "Vendor: " << m_capabilities.vendor_name << std::endl;
    std::cout << "Version: " << m_capabilities.version_string << std::endl;
#else
    std::cout << "WebGL Device initialization skipped (not running in Emscripten)" << std::endl;
    m_initialized = true;
#endif

    return true;
}

void WebGLDevice::Shutdown() {
    if (!m_initialized) {
        return;
    }

    std::cout << "Shutting down WebGL Device..." << std::endl;
    
#ifdef __EMSCRIPTEN__
    // Destroy WebGL context
    if (m_webgl_context != 0) {
        emscripten_webgl_destroy_context(m_webgl_context);
        m_webgl_context = 0;
    }
#endif
    
    // Reset state
    m_webgl_state = {};
    m_initialized = false;
}

bool WebGLDevice::InitializeWebGL() {
#ifdef __EMSCRIPTEN__
    // Get the canvas element
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    
    // Configure WebGL context attributes
    attrs.alpha = EM_TRUE;
    attrs.depth = EM_TRUE;
    attrs.stencil = EM_FALSE;
    attrs.antialias = EM_TRUE;
    attrs.premultipliedAlpha = EM_FALSE;
    attrs.preserveDrawingBuffer = EM_FALSE;
    attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    attrs.failIfMajorPerformanceCaveat = EM_FALSE;
    attrs.majorVersion = 2; // Request WebGL 2.0
    attrs.minorVersion = 0;
    attrs.enableExtensionsByDefault = EM_TRUE;
    attrs.explicitSwapControl = EM_FALSE;
    attrs.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_FALLBACK;
    attrs.renderViaOffscreenBackBuffer = EM_FALSE;

    // Create WebGL context
    m_webgl_context = emscripten_webgl_create_context("#canvas", &attrs);
    if (m_webgl_context <= 0) {
        std::cerr << "Failed to create WebGL context" << std::endl;
        return false;
    }

    // Make context current
    EMSCRIPTEN_RESULT result = emscripten_webgl_make_context_current(m_webgl_context);
    if (result != EMSCRIPTEN_RESULT_SUCCESS) {
        std::cerr << "Failed to make WebGL context current" << std::endl;
        return false;
    }

    // Check if we got WebGL 2.0
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (version && strstr(version, "WebGL 2.0")) {
        m_supports_webgl2 = true;
        std::cout << "WebGL 2.0 context created successfully" << std::endl;
    } else {
        std::cout << "WebGL 1.0 context created (WebGL 2.0 not available)" << std::endl;
        m_supports_webgl2 = false;
    }

    return true;
#else
    return false;
#endif
}

void WebGLDevice::QueryCapabilities() {
#ifdef __EMSCRIPTEN__
    // Get basic information
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

    m_capabilities.renderer_name = renderer ? renderer : "Unknown";
    m_capabilities.vendor_name = vendor ? vendor : "Unknown";
    m_capabilities.version_string = version ? version : "Unknown";

    // Query limits
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_capabilities.max_texture_size);
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &m_capabilities.max_cubemap_size);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_capabilities.max_texture_units);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &m_capabilities.max_vertex_attributes);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &m_capabilities.max_renderbuffer_size);
    
    GLint viewport[4];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, viewport);
    m_capabilities.max_viewport_width = viewport[0];
    m_capabilities.max_viewport_height = viewport[1];

    // WebGL-specific feature support
    m_capabilities.supports_geometry_shaders = false; // Not available in WebGL
    m_capabilities.supports_tessellation = false;     // Not available in WebGL
    m_capabilities.supports_compute_shaders = false;  // Not available in WebGL 2.0
    m_capabilities.supports_instancing = m_supports_webgl2;
    m_capabilities.supports_texture_compression = true;
    m_capabilities.supports_depth_texture = m_supports_depth_texture;
    m_capabilities.supports_shadow_mapping = m_supports_depth_texture;
    m_capabilities.supports_framebuffer_objects = true;
    m_capabilities.supports_vertex_array_objects = m_supports_vao;
    m_capabilities.supports_uniform_buffer_objects = m_supports_webgl2;
    m_capabilities.supports_shader_storage_buffer_objects = false; // Not in WebGL 2.0

    // Performance hints for WebGL
    m_capabilities.prefer_immediate_mode = true;  // WebGL prefers immediate mode
    m_capabilities.prefer_retained_mode = false;
    m_capabilities.prefer_instancing = m_supports_instancing;
    m_capabilities.prefer_uniform_buffers = false; // Uniforms are often faster than UBOs in WebGL
#endif
}

void WebGLDevice::DetectExtensions() {
#ifdef __EMSCRIPTEN__
    // Get number of extensions
    GLint num_extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);

    // Query all extensions
    m_extensions.clear();
    for (int i = 0; i < num_extensions; ++i) {
        const char* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (extension) {
            std::string ext_name(extension);
            m_extensions.push_back(ext_name);
            m_extension_cache[ext_name] = true;
        }
    }

    // Check for specific extensions we care about
    m_supports_vao = IsExtensionSupported("OES_vertex_array_object") || m_supports_webgl2;
    m_supports_instancing = IsExtensionSupported("ANGLE_instanced_arrays") || m_supports_webgl2;
    m_supports_depth_texture = IsExtensionSupported("WEBGL_depth_texture") || m_supports_webgl2;
    m_supports_float_textures = IsExtensionSupported("OES_texture_float") || m_supports_webgl2;
    m_supports_anisotropic_filtering = IsExtensionSupported("EXT_texture_filter_anisotropic");

    // Update capabilities based on extensions
    m_capabilities.supports_anisotropic_filtering = m_supports_anisotropic_filtering;
    if (m_supports_anisotropic_filtering) {
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &m_capabilities.max_anisotropy);
    }

    std::cout << "Detected " << m_extensions.size() << " WebGL extensions" << std::endl;
#endif
}

void WebGLDevice::Clear(const glm::vec4& color, bool clear_color, bool clear_depth, bool clear_stencil) {
#ifdef __EMSCRIPTEN__
    GLbitfield mask = 0;
    
    if (clear_color) {
        glClearColor(color.r, color.g, color.b, color.a);
        mask |= GL_COLOR_BUFFER_BIT;
    }
    
    if (clear_depth) {
        mask |= GL_DEPTH_BUFFER_BIT;
    }
    
    if (clear_stencil) {
        mask |= GL_STENCIL_BUFFER_BIT;
    }
    
    if (mask != 0) {
        glClear(mask);
    }
#endif
}

void WebGLDevice::SetViewport(int x, int y, int width, int height) {
#ifdef __EMSCRIPTEN__
    glViewport(x, y, width, height);
#endif
}

void WebGLDevice::Draw(PrimitiveType primitive_type, int vertex_count, int first_vertex) {
#ifdef __EMSCRIPTEN__
    GLenum gl_primitive = ConvertPrimitiveType(primitive_type);
    glDrawArrays(gl_primitive, first_vertex, vertex_count);
    TrackDrawCall(vertex_count);
#endif
}

void WebGLDevice::DrawIndexed(PrimitiveType primitive_type, int index_count, int first_index) {
#ifdef __EMSCRIPTEN__
    GLenum gl_primitive = ConvertPrimitiveType(primitive_type);
    glDrawElements(gl_primitive, index_count, GL_UNSIGNED_SHORT,
                   reinterpret_cast<void*>(first_index * sizeof(uint16_t)));
    TrackDrawCall(index_count);
#endif
}

void WebGLDevice::SetDepthTest(bool enable) {
#ifdef __EMSCRIPTEN__
    if (m_webgl_state.depth_test != enable) {
        if (enable) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        m_webgl_state.depth_test = enable;
    }
#endif
}

void WebGLDevice::SetDepthWrite(bool enable) {
#ifdef __EMSCRIPTEN__
    if (m_webgl_state.depth_write != enable) {
        glDepthMask(enable ? GL_TRUE : GL_FALSE);
        m_webgl_state.depth_write = enable;
    }
#endif
}

void WebGLDevice::SetDepthFunc(DepthFunc func) {
#ifdef __EMSCRIPTEN__
    GLenum gl_func = ConvertDepthFunc(func);
    if (m_webgl_state.depth_func != gl_func) {
        glDepthFunc(gl_func);
        m_webgl_state.depth_func = gl_func;
    }
#endif
}

void WebGLDevice::SetBlending(bool enable) {
#ifdef __EMSCRIPTEN__
    if (m_webgl_state.blending != enable) {
        if (enable) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
        m_webgl_state.blending = enable;
    }
#endif
}

void WebGLDevice::SetCulling(bool enable) {
#ifdef __EMSCRIPTEN__
    if (m_webgl_state.culling != enable) {
        if (enable) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }
        m_webgl_state.culling = enable;
    }
#endif
}

void WebGLDevice::SetBlendFunc(int src_factor, int dst_factor) {
#ifdef __EMSCRIPTEN__
    GLenum gl_src = ConvertBlendFactor(static_cast<BlendFactor>(src_factor));
    GLenum gl_dst = ConvertBlendFactor(static_cast<BlendFactor>(dst_factor));

    if (m_webgl_state.blend_src != gl_src || m_webgl_state.blend_dst != gl_dst) {
        glBlendFunc(gl_src, gl_dst);
        m_webgl_state.blend_src = gl_src;
        m_webgl_state.blend_dst = gl_dst;
    }
#endif
}

void WebGLDevice::SetCullFace(bool front_face) {
#ifdef __EMSCRIPTEN__
    GLenum cull_face = front_face ? GL_FRONT : GL_BACK;
    if (m_webgl_state.cull_face != cull_face) {
        glCullFace(cull_face);
        m_webgl_state.cull_face = cull_face;
    }
#endif
}

std::string WebGLDevice::CheckError() {
#ifdef __EMSCRIPTEN__
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
        return "";
    }
    return GetWebGLErrorString(error);
#else
    return "";
#endif
}

void WebGLDevice::ResetFrameStats() {
    UpdateFrameStats();
}

// Resource creation methods
std::shared_ptr<GraphicsBuffer> WebGLDevice::CreateBuffer(
    BufferType type,
    BufferUsage usage,
    size_t size,
    const void* data) {
    return std::make_shared<WebGLBuffer>(type, usage, size, data);
}

std::shared_ptr<GraphicsTexture> WebGLDevice::CreateTexture2D(
    int width,
    int height,
    TextureFormat format,
    const void* data) {
    return std::make_shared<WebGLTexture>(width, height, format, data);
}

std::shared_ptr<GraphicsTexture> WebGLDevice::CreateTextureCubemap(
    int size,
    TextureFormat format,
    const void* data[6]) {
    return std::make_shared<WebGLTexture>(size, format, data);
}

std::shared_ptr<GraphicsShader> WebGLDevice::CreateShader(
    const std::string& vertex_source,
    const std::string& fragment_source,
    const std::string& geometry_source) {
    return std::make_shared<WebGLShader>(vertex_source, fragment_source, geometry_source);
}

std::shared_ptr<GraphicsVertexArray> WebGLDevice::CreateVertexArray() {
    return std::make_shared<WebGLVertexArray>();
}

bool WebGLDevice::IsExtensionSupported(const std::string& extension) const {
    auto it = m_extension_cache.find(extension);
    return it != m_extension_cache.end() && it->second;
}

std::vector<std::string> WebGLDevice::GetSupportedExtensions() const {
    return m_extensions;
}

std::string WebGLDevice::ConvertShaderToGLSLES(const std::string& source, GLenum type) const {
    // Convert GLSL to GLSL ES
    std::string converted = source;

    // Replace version directive
    size_t version_pos = converted.find("#version");
    if (version_pos != std::string::npos) {
        size_t line_end = converted.find('\n', version_pos);
        if (m_supports_webgl2) {
            converted.replace(version_pos, line_end - version_pos, "#version 300 es");
        } else {
            converted.replace(version_pos, line_end - version_pos, "#version 100");
        }
    } else {
        // Add version directive if missing
        if (m_supports_webgl2) {
            converted = "#version 300 es\n" + converted;
        } else {
            converted = "#version 100\n" + converted;
        }
    }

#ifdef __EMSCRIPTEN__
    // Add precision qualifiers for fragment shaders
    if (type == GL_FRAGMENT_SHADER) {
        size_t insert_pos = converted.find('\n') + 1;
        converted.insert(insert_pos, "precision mediump float;\n");
    }
#endif

    return converted;
}

std::string WebGLDevice::GetPerformanceInfo() const {
    std::string info = "WebGL Performance Info:\n";
    info += "Draw Calls: " + std::to_string(m_frame_stats.draw_calls) + "\n";
    info += "Vertices Rendered: " + std::to_string(m_frame_stats.vertices_rendered) + "\n";
    info += "Triangles Rendered: " + std::to_string(m_frame_stats.triangles_rendered) + "\n";
    info += "WebGL 2.0: " + std::string(m_supports_webgl2 ? "Yes" : "No") + "\n";
    info += "VAO Support: " + std::string(m_supports_vao ? "Yes" : "No") + "\n";
    info += "Instancing Support: " + std::string(m_supports_instancing ? "Yes" : "No") + "\n";
    return info;
}

void WebGLDevice::UpdateFrameStats() {
    m_frame_stats.draw_calls = 0;
    m_frame_stats.vertices_rendered = 0;
    m_frame_stats.triangles_rendered = 0;
}

void WebGLDevice::TrackDrawCall(int vertices) {
    m_frame_stats.draw_calls++;
    m_frame_stats.vertices_rendered += vertices;
    m_frame_stats.triangles_rendered += vertices / 3; // Assuming triangles
}

GLenum WebGLDevice::ConvertPrimitiveType(PrimitiveType type) const {
#ifdef __EMSCRIPTEN__
    switch (type) {
        case PrimitiveType::Points:        return GL_POINTS;
        case PrimitiveType::Lines:         return GL_LINES;
        case PrimitiveType::LineStrip:     return GL_LINE_STRIP;
        case PrimitiveType::Triangles:     return GL_TRIANGLES;
        case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
        case PrimitiveType::TriangleFan:   return GL_TRIANGLE_FAN;
        default:                           return GL_TRIANGLES;
    }
#else
    // Stub implementation for non-Emscripten builds
    return 0;
#endif
}

GLenum WebGLDevice::ConvertBlendFactor(BlendFactor factor) const {
#ifdef __EMSCRIPTEN__
    switch (factor) {
        case BlendFactor::Zero:                return GL_ZERO;
        case BlendFactor::One:                 return GL_ONE;
        case BlendFactor::SrcColor:            return GL_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:    return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DstColor:            return GL_DST_COLOR;
        case BlendFactor::OneMinusDstColor:    return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::SrcAlpha:            return GL_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:    return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstAlpha:            return GL_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:    return GL_ONE_MINUS_DST_ALPHA;
        case BlendFactor::ConstantColor:       return GL_CONSTANT_COLOR;
        case BlendFactor::OneMinusConstantColor: return GL_ONE_MINUS_CONSTANT_COLOR;
        case BlendFactor::ConstantAlpha:       return GL_CONSTANT_ALPHA;
        case BlendFactor::OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
        default:                               return GL_ONE;
    }
#else
    // Stub implementation for non-Emscripten builds
    return 0;
#endif
}

std::string WebGLDevice::GetWebGLErrorString(GLenum error) const {
#ifdef __EMSCRIPTEN__
    switch (error) {
        case GL_INVALID_ENUM:      return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:     return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY:     return "GL_OUT_OF_MEMORY";
        default:                   return "Unknown WebGL error";
    }
#else
    // Stub implementation for non-Emscripten builds
    return "WebGL not available";
#endif
}

GLenum WebGLDevice::ConvertDepthFunc(DepthFunc func) const {
#ifdef __EMSCRIPTEN__
    switch (func) {
        case DepthFunc::Never:         return GL_NEVER;
        case DepthFunc::Less:          return GL_LESS;
        case DepthFunc::Equal:         return GL_EQUAL;
        case DepthFunc::LessEqual:     return GL_LEQUAL;
        case DepthFunc::Greater:       return GL_GREATER;
        case DepthFunc::NotEqual:      return GL_NOTEQUAL;
        case DepthFunc::GreaterEqual:  return GL_GEQUAL;
        case DepthFunc::Always:        return GL_ALWAYS;
        default:                       return GL_LESS;
    }
#else
    // Stub for non-Emscripten builds
    return 0;
#endif
}

} // namespace Lupine
