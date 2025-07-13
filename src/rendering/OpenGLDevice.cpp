#include "lupine/rendering/OpenGLDevice.h"
#include "lupine/rendering/OpenGLBuffer.h"
#include "lupine/rendering/OpenGLTexture.h"
#include "lupine/rendering/OpenGLShader.h"
#include "lupine/rendering/OpenGLVertexArray.h"
#include "lupine/rendering/RenderState.h"
#include <glad/glad.h>
#include <iostream>
#include <cassert>
#include <cstdint>

// Fallback definitions for GLAD extension variables if not linked from static library
#ifndef GLAD_GL_ARB_debug_output
extern "C" int GLAD_GL_ARB_debug_output = 0;
#endif

#ifndef GLAD_GL_EXT_texture_filter_anisotropic
extern "C" int GLAD_GL_EXT_texture_filter_anisotropic = 0;
#endif

// Define OpenGL extension constants if not available
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#ifndef GLAD_GL_KHR_debug
extern "C" int GLAD_GL_KHR_debug = 0;
#endif

namespace Lupine {

// Forward declarations for OpenGL-specific implementations
class OpenGLBuffer;
class OpenGLTexture;
class OpenGLShader;
class OpenGLVertexArray;

OpenGLDevice::OpenGLDevice()
    : m_initialized(false)
    , m_debug_enabled(false) {
    // Initialize capabilities with default values
    m_capabilities.backend = GraphicsBackend::OpenGL;
    m_capabilities.backend_name = "OpenGL";
}

OpenGLDevice::~OpenGLDevice() {
    if (m_initialized) {
        Shutdown();
    }
}

bool OpenGLDevice::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing OpenGL Device..." << std::endl;

    // Initialize OpenGL function pointers
    if (!InitializeOpenGL()) {
        std::cerr << "Failed to initialize OpenGL" << std::endl;
        return false;
    }

    // Query device capabilities
    QueryCapabilities();

    // Setup debug callback if available
    if (m_capabilities.supports_debug_output) {
        SetupDebugCallback();
    }

    // Set default OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    // Initialize state tracking
    m_gl_state = GLState();

    m_initialized = true;
    std::cout << "OpenGL Device initialized successfully" << std::endl;
    std::cout << "Renderer: " << m_capabilities.renderer_name << std::endl;
    std::cout << "Vendor: " << m_capabilities.vendor_name << std::endl;
    std::cout << "Version: " << m_capabilities.version_string << std::endl;

    return true;
}

void OpenGLDevice::Shutdown() {
    if (!m_initialized) {
        return;
    }

    std::cout << "Shutting down OpenGL Device..." << std::endl;
    
    // Reset state
    m_gl_state = GLState();
    m_initialized = false;
}

bool OpenGLDevice::InitializeOpenGL() {
    // GLAD should already be initialized by the main application
    // We just verify that OpenGL is available
    if (!gladLoadGL()) {
        std::cerr << "Failed to load OpenGL functions" << std::endl;
        return false;
    }

    // Check minimum OpenGL version (3.3)
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    
    if (major < 3 || (major == 3 && minor < 3)) {
        std::cerr << "OpenGL 3.3 or higher required, got " << major << "." << minor << std::endl;
        return false;
    }

    return true;
}

void OpenGLDevice::QueryCapabilities() {
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

    // Check for multisampling support
    GLint max_samples;
    glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
    m_capabilities.max_samples = max_samples;
    m_capabilities.supports_multisampling = max_samples > 1;

    // Check for anisotropic filtering
    if (GLAD_GL_EXT_texture_filter_anisotropic) {
        m_capabilities.supports_anisotropic_filtering = true;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &m_capabilities.max_anisotropy);
    }

    // Feature support based on OpenGL version and extensions
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    
    m_capabilities.supports_geometry_shaders = (major > 3) || (major == 3 && minor >= 2);
    m_capabilities.supports_tessellation = (major > 4) || (major == 4 && minor >= 0);
    m_capabilities.supports_compute_shaders = (major > 4) || (major == 4 && minor >= 3);
    m_capabilities.supports_instancing = true; // Available since OpenGL 3.3
    m_capabilities.supports_texture_compression = true;
    m_capabilities.supports_depth_texture = true;
    m_capabilities.supports_shadow_mapping = true;
    m_capabilities.supports_framebuffer_objects = true;
    m_capabilities.supports_vertex_array_objects = true;
    m_capabilities.supports_uniform_buffer_objects = (major > 3) || (major == 3 && minor >= 1);
    m_capabilities.supports_shader_storage_buffer_objects = (major > 4) || (major == 4 && minor >= 3);

    // Check for debug output support
    m_capabilities.supports_debug_output = GLAD_GL_KHR_debug || GLAD_GL_ARB_debug_output;

    // Performance hints for desktop OpenGL
    m_capabilities.prefer_immediate_mode = false;
    m_capabilities.prefer_retained_mode = true;
    m_capabilities.prefer_instancing = m_capabilities.supports_instancing;
    m_capabilities.prefer_uniform_buffers = m_capabilities.supports_uniform_buffer_objects;
}

void OpenGLDevice::SetupDebugCallback() {
    if (!m_capabilities.supports_debug_output) {
        return;
    }

    // Enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    // Set debug callback
    glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
                             GLsizei length, const GLchar* message, const void* userParam) {
        // Filter out non-significant error/warning codes
        if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

        std::string severity_str;
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:         severity_str = "HIGH"; break;
            case GL_DEBUG_SEVERITY_MEDIUM:       severity_str = "MEDIUM"; break;
            case GL_DEBUG_SEVERITY_LOW:          severity_str = "LOW"; break;
            case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "NOTIFICATION"; break;
            default:                             severity_str = "UNKNOWN"; break;
        }

        std::string type_str;
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:               type_str = "ERROR"; break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "DEPRECATED"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_str = "UNDEFINED"; break;
            case GL_DEBUG_TYPE_PORTABILITY:         type_str = "PORTABILITY"; break;
            case GL_DEBUG_TYPE_PERFORMANCE:         type_str = "PERFORMANCE"; break;
            case GL_DEBUG_TYPE_MARKER:              type_str = "MARKER"; break;
            case GL_DEBUG_TYPE_PUSH_GROUP:          type_str = "PUSH_GROUP"; break;
            case GL_DEBUG_TYPE_POP_GROUP:           type_str = "POP_GROUP"; break;
            case GL_DEBUG_TYPE_OTHER:               type_str = "OTHER"; break;
            default:                                type_str = "UNKNOWN"; break;
        }

        std::cout << "[OpenGL " << severity_str << "] " << type_str << ": " << message << std::endl;
    }, nullptr);

    // Filter debug messages
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    
    m_debug_enabled = true;
}

void OpenGLDevice::Clear(const glm::vec4& color, bool clear_color, bool clear_depth, bool clear_stencil) {
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
}

void OpenGLDevice::SetViewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
}

void OpenGLDevice::Draw(PrimitiveType primitive_type, int vertex_count, int first_vertex) {
    GLenum gl_primitive = ConvertPrimitiveType(primitive_type);
    glDrawArrays(gl_primitive, first_vertex, vertex_count);

    TrackDrawCall(vertex_count);
}

void OpenGLDevice::DrawIndexed(PrimitiveType primitive_type, int index_count, int first_index) {
    GLenum gl_primitive = ConvertPrimitiveType(primitive_type);
    glDrawElements(gl_primitive, index_count, GL_UNSIGNED_INT,
                   reinterpret_cast<void*>(first_index * sizeof(uint32_t)));

    TrackDrawCall(index_count);
}

void OpenGLDevice::SetDepthTest(bool enable) {
    if (m_gl_state.depth_test != enable) {
        if (enable) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        m_gl_state.depth_test = enable;
    }
}

void OpenGLDevice::SetDepthWrite(bool enable) {
    if (m_gl_state.depth_write != enable) {
        glDepthMask(enable ? GL_TRUE : GL_FALSE);
        m_gl_state.depth_write = enable;
    }
}

void OpenGLDevice::SetDepthFunc(DepthFunc func) {
    GLenum gl_func = ConvertDepthFunc(func);
    if (m_gl_state.depth_func != gl_func) {
        glDepthFunc(gl_func);
        m_gl_state.depth_func = gl_func;
    }
}

void OpenGLDevice::SetBlending(bool enable) {
    if (m_gl_state.blending != enable) {
        if (enable) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
        m_gl_state.blending = enable;
    }
}

void OpenGLDevice::SetCulling(bool enable) {
    if (m_gl_state.culling != enable) {
        if (enable) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }
        m_gl_state.culling = enable;
    }
}

GLenum OpenGLDevice::ConvertPrimitiveType(PrimitiveType type) const {
    switch (type) {
        case PrimitiveType::Points:        return GL_POINTS;
        case PrimitiveType::Lines:         return GL_LINES;
        case PrimitiveType::LineStrip:     return GL_LINE_STRIP;
        case PrimitiveType::Triangles:     return GL_TRIANGLES;
        case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
        default:                           return GL_TRIANGLES;
    }
}

void OpenGLDevice::UpdateFrameStats() {
    m_frame_stats.draw_calls = 0;
    m_frame_stats.vertices_rendered = 0;
    m_frame_stats.triangles_rendered = 0;
}

void OpenGLDevice::TrackDrawCall(int vertices) {
    m_frame_stats.draw_calls++;
    m_frame_stats.vertices_rendered += vertices;
    m_frame_stats.triangles_rendered += vertices / 3; // Assuming triangles
}

// Resource creation methods
std::shared_ptr<GraphicsBuffer> OpenGLDevice::CreateBuffer(
    BufferType type,
    BufferUsage usage,
    size_t size,
    const void* data) {
    return std::make_shared<OpenGLBuffer>(type, usage, size, data);
}

std::shared_ptr<GraphicsTexture> OpenGLDevice::CreateTexture2D(
    int width,
    int height,
    TextureFormat format,
    const void* data) {
    return std::make_shared<OpenGLTexture>(width, height, format, data);
}

std::shared_ptr<GraphicsTexture> OpenGLDevice::CreateTextureCubemap(
    int size,
    TextureFormat format,
    const void* data[6]) {
    return std::make_shared<OpenGLTexture>(size, format, data);
}

std::shared_ptr<GraphicsShader> OpenGLDevice::CreateShader(
    const std::string& vertex_source,
    const std::string& fragment_source,
    const std::string& geometry_source) {
    return std::make_shared<OpenGLShader>(vertex_source, fragment_source, geometry_source);
}

std::shared_ptr<GraphicsVertexArray> OpenGLDevice::CreateVertexArray() {
    return std::make_shared<OpenGLVertexArray>();
}

void OpenGLDevice::SetBlendFunc(int src_factor, int dst_factor) {
    GLenum gl_src = ConvertBlendFactor(static_cast<BlendFactor>(src_factor));
    GLenum gl_dst = ConvertBlendFactor(static_cast<BlendFactor>(dst_factor));

    if (m_gl_state.blend_src != gl_src || m_gl_state.blend_dst != gl_dst) {
        glBlendFunc(gl_src, gl_dst);
        m_gl_state.blend_src = gl_src;
        m_gl_state.blend_dst = gl_dst;
    }
}

void OpenGLDevice::SetCullFace(bool front_face) {
    GLenum cull_face = front_face ? GL_FRONT : GL_BACK;
    if (m_gl_state.cull_face != cull_face) {
        glCullFace(cull_face);
        m_gl_state.cull_face = cull_face;
    }
}

std::string OpenGLDevice::CheckError() {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
        return "";
    }

    std::string error_string;
    switch (error) {
        case GL_INVALID_ENUM:      error_string = "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE:     error_string = "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: error_string = "GL_INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY:     error_string = "GL_OUT_OF_MEMORY"; break;
        default:                   error_string = "Unknown OpenGL error"; break;
    }

    return error_string;
}

void OpenGLDevice::ResetFrameStats() {
    UpdateFrameStats();
}

GLenum OpenGLDevice::ConvertBlendFactor(BlendFactor factor) const {
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
}

GLenum OpenGLDevice::ConvertDepthFunc(DepthFunc func) const {
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
}

} // namespace Lupine
