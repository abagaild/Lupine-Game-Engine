#pragma once

#include <string>
#include <cstddef>

namespace Lupine {

/**
 * @brief Graphics backend enumeration
 * 
 * Defines the available graphics backends that the engine can use.
 * This allows for runtime and compile-time backend selection.
 */
enum class GraphicsBackend {
    None = 0,       ///< No graphics backend (headless mode)
    OpenGL,         ///< Desktop OpenGL 3.3+ Core Profile
    WebGL,          ///< WebGL 2.0 for web platforms
    Vulkan,         ///< Vulkan API (future implementation)
    DirectX11,      ///< DirectX 11 (future implementation)
    DirectX12,      ///< DirectX 12 (future implementation)
    Metal           ///< Apple Metal (future implementation)
};

/**
 * @brief Graphics capabilities structure
 * 
 * Contains information about the capabilities and limits of the
 * current graphics backend and hardware.
 */
struct GraphicsCapabilities {
    // Backend information
    GraphicsBackend backend = GraphicsBackend::None;
    std::string backend_name;
    std::string renderer_name;
    std::string vendor_name;
    std::string version_string;
    
    // Feature support
    bool supports_geometry_shaders = false;
    bool supports_tessellation = false;
    bool supports_compute_shaders = false;
    bool supports_instancing = false;
    bool supports_multisampling = false;
    bool supports_anisotropic_filtering = false;
    bool supports_texture_compression = false;
    bool supports_depth_texture = false;
    bool supports_shadow_mapping = false;
    bool supports_framebuffer_objects = false;
    bool supports_vertex_array_objects = false;
    bool supports_uniform_buffer_objects = false;
    bool supports_shader_storage_buffer_objects = false;
    bool supports_debug_output = false;
    
    // Limits
    int max_texture_size = 0;
    int max_cubemap_size = 0;
    int max_texture_units = 0;
    int max_vertex_attributes = 0;
    int max_uniform_locations = 0;
    int max_varying_vectors = 0;
    int max_vertex_uniform_vectors = 0;
    int max_fragment_uniform_vectors = 0;
    int max_renderbuffer_size = 0;
    int max_viewport_width = 0;
    int max_viewport_height = 0;
    int max_samples = 0;
    float max_anisotropy = 0.0f;
    
    // Memory information
    size_t total_video_memory = 0;      ///< Total video memory in bytes
    size_t available_video_memory = 0;  ///< Available video memory in bytes
    
    // Performance hints
    bool prefer_immediate_mode = false;  ///< Prefer immediate mode rendering
    bool prefer_retained_mode = true;    ///< Prefer retained mode rendering
    bool prefer_instancing = false;      ///< Prefer instanced rendering
    bool prefer_uniform_buffers = false; ///< Prefer uniform buffer objects
};

/**
 * @brief Get string representation of graphics backend
 * @param backend Graphics backend enumeration
 * @return String name of the backend
 */
inline const char* GetBackendName(GraphicsBackend backend) {
    switch (backend) {
        case GraphicsBackend::None:      return "None";
        case GraphicsBackend::OpenGL:    return "OpenGL";
        case GraphicsBackend::WebGL:     return "WebGL";
        case GraphicsBackend::Vulkan:    return "Vulkan";
        case GraphicsBackend::DirectX11: return "DirectX11";
        case GraphicsBackend::DirectX12: return "DirectX12";
        case GraphicsBackend::Metal:     return "Metal";
        default:                         return "Unknown";
    }
}

/**
 * @brief Check if backend is available at compile time
 * @param backend Graphics backend to check
 * @return True if backend is compiled in
 */
inline bool IsBackendAvailable(GraphicsBackend backend) {
    switch (backend) {
        case GraphicsBackend::OpenGL:
#ifdef LUPINE_OPENGL_BACKEND
            return true;
#else
            return false;
#endif
        
        case GraphicsBackend::WebGL:
#ifdef LUPINE_WEBGL_BACKEND
            return true;
#else
            return false;
#endif
        
        case GraphicsBackend::Vulkan:
#ifdef LUPINE_VULKAN_BACKEND
            return true;
#else
            return false;
#endif
        
        default:
            return false;
    }
}

} // namespace Lupine
