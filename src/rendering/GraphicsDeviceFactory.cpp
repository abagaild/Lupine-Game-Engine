#include "lupine/rendering/GraphicsDeviceFactory.h"
#include "lupine/rendering/GraphicsBackend.h"
#include <iostream>

// Include backend implementations
#ifdef LUPINE_OPENGL_BACKEND
#include "lupine/rendering/OpenGLDevice.h"
#endif

#ifdef LUPINE_WEBGL_BACKEND
#include "lupine/rendering/WebGLDevice.h"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

namespace Lupine {

// Static member for current device instance
static std::shared_ptr<GraphicsDevice> s_current_device = nullptr;

std::shared_ptr<GraphicsDevice> GraphicsDeviceFactory::CreateDevice(const DeviceParams& params) {
    std::cout << "Creating graphics device..." << std::endl;
    
    // Determine the best backend based on parameters and platform
    GraphicsBackend backend = DetermineBackend(params.preference);
    
    if (backend == GraphicsBackend::None) {
        std::cerr << "No suitable graphics backend found" << std::endl;
        return nullptr;
    }
    
    std::cout << "Selected backend: " << GetBackendName(backend) << std::endl;
    
    // Create device based on backend
    std::shared_ptr<GraphicsDevice> device = CreateDeviceForBackend(backend, params);
    
    if (!device) {
        std::cerr << "Failed to create graphics device for backend: " << GetBackendName(backend) << std::endl;
        return nullptr;
    }
    
    // Initialize the device
    if (!device->Initialize()) {
        std::cerr << "Failed to initialize graphics device" << std::endl;
        return nullptr;
    }
    
    std::cout << "Graphics device created and initialized successfully" << std::endl;
    return device;
}

std::vector<GraphicsBackend> GraphicsDeviceFactory::GetAvailableBackends() {
    std::vector<GraphicsBackend> backends;
    
#ifdef __EMSCRIPTEN__
    // Web platform - only WebGL is available
    if (IsBackendAvailable(GraphicsBackend::WebGL)) {
        backends.push_back(GraphicsBackend::WebGL);
    }
#else
    // Desktop platform - OpenGL is primary
    if (IsBackendAvailable(GraphicsBackend::OpenGL)) {
        backends.push_back(GraphicsBackend::OpenGL);
    }
    
    // Future backends can be added here
    if (IsBackendAvailable(GraphicsBackend::Vulkan)) {
        backends.push_back(GraphicsBackend::Vulkan);
    }
#endif
    
    return backends;
}

GraphicsBackend GraphicsDeviceFactory::GetDefaultBackend() {
#ifdef __EMSCRIPTEN__
    return GraphicsBackend::WebGL;
#else
    return GraphicsBackend::OpenGL;
#endif
}

bool GraphicsDeviceFactory::IsBackendSupported(GraphicsBackend backend) {
    auto available = GetAvailableBackends();
    return std::find(available.begin(), available.end(), backend) != available.end();
}

std::string GraphicsDeviceFactory::GetBackendInfo(GraphicsBackend backend) {
    switch (backend) {
        case GraphicsBackend::OpenGL:
            return "Desktop OpenGL 3.3+ Core Profile - High performance desktop rendering";
        case GraphicsBackend::WebGL:
            return "WebGL 2.0 - Web-based rendering with GLES compatibility";
        case GraphicsBackend::Vulkan:
            return "Vulkan API - Low-level high-performance graphics (future)";
        case GraphicsBackend::DirectX11:
            return "DirectX 11 - Windows native graphics API (future)";
        case GraphicsBackend::DirectX12:
            return "DirectX 12 - Modern Windows graphics API (future)";
        case GraphicsBackend::Metal:
            return "Apple Metal - macOS/iOS native graphics API (future)";
        default:
            return "Unknown graphics backend";
    }
}

GraphicsBackend GraphicsDeviceFactory::DetermineBackend(BackendPreference preference) {
    auto available = GetAvailableBackends();
    
    if (available.empty()) {
        return GraphicsBackend::None;
    }
    
    switch (preference) {
        case BackendPreference::Auto:
            // Return the best available backend for the platform
            return GetDefaultBackend();
            
        case BackendPreference::OpenGL:
            if (std::find(available.begin(), available.end(), GraphicsBackend::OpenGL) != available.end()) {
                return GraphicsBackend::OpenGL;
            }
            break;
            
        case BackendPreference::WebGL:
            if (std::find(available.begin(), available.end(), GraphicsBackend::WebGL) != available.end()) {
                return GraphicsBackend::WebGL;
            }
            break;
            
        case BackendPreference::Any:
            // Return the first available backend
            return available[0];
    }
    
    // Fallback to default if preferred backend is not available
    std::cout << "Preferred backend not available, falling back to default" << std::endl;
    return GetDefaultBackend();
}

std::shared_ptr<GraphicsDevice> GraphicsDeviceFactory::CreateDeviceForBackend(
    GraphicsBackend backend, 
    const DeviceParams& params) {
    
    switch (backend) {
#ifdef LUPINE_OPENGL_BACKEND
        case GraphicsBackend::OpenGL: {
            auto device = std::make_shared<OpenGLDevice>();
            return device;
        }
#endif

#ifdef LUPINE_WEBGL_BACKEND
        case GraphicsBackend::WebGL: {
            auto device = std::make_shared<WebGLDevice>();
            return device;
        }
#endif

        default:
            std::cerr << "Backend not implemented: " << GetBackendName(backend) << std::endl;
            return nullptr;
    }
}

bool GraphicsDeviceFactory::ValidateDeviceParams(const DeviceParams& params) {
    // Validate version requirements
    if (params.major_version < 3 || (params.major_version == 3 && params.minor_version < 3)) {
        std::cerr << "Minimum OpenGL version 3.3 required" << std::endl;
        return false;
    }
    
    // Validate backend preference
    if (params.preference != BackendPreference::Auto &&
        params.preference != BackendPreference::OpenGL &&
        params.preference != BackendPreference::WebGL &&
        params.preference != BackendPreference::Any) {
        std::cerr << "Invalid backend preference" << std::endl;
        return false;
    }
    
    return true;
}

std::shared_ptr<GraphicsDevice> GraphicsDeviceFactory::CreateDeviceWithFallback(
    const std::vector<GraphicsBackend>& preferred_backends,
    const DeviceParams& params) {
    
    for (GraphicsBackend backend : preferred_backends) {
        if (IsBackendSupported(backend)) {
            std::cout << "Trying backend: " << GetBackendName(backend) << std::endl;
            
            auto device = CreateDeviceForBackend(backend, params);
            if (device && device->Initialize()) {
                std::cout << "Successfully created device with backend: " << GetBackendName(backend) << std::endl;
                return device;
            }
            
            std::cout << "Failed to create device with backend: " << GetBackendName(backend) << std::endl;
        }
    }
    
    std::cerr << "Failed to create device with any of the preferred backends" << std::endl;
    return nullptr;
}

void GraphicsDeviceFactory::PrintAvailableBackends() {
    auto backends = GetAvailableBackends();

    std::cout << "Available graphics backends:" << std::endl;
    for (GraphicsBackend backend : backends) {
        std::cout << "  - " << GetBackendName(backend) << ": " << GetBackendInfo(backend) << std::endl;
    }

    if (backends.empty()) {
        std::cout << "  No graphics backends available" << std::endl;
    }
}

std::shared_ptr<GraphicsDevice> GraphicsDeviceFactory::GetDevice() {
    return s_current_device;
}

void GraphicsDeviceFactory::SetDevice(std::shared_ptr<GraphicsDevice> device) {
    s_current_device = device;
}

bool GraphicsDeviceFactory::IsBackendAvailable(GraphicsBackend backend) {
    switch (backend) {
        case GraphicsBackend::OpenGL:
            // OpenGL is available on desktop platforms
#ifndef __EMSCRIPTEN__
            return true;
#else
            return false;
#endif

        case GraphicsBackend::WebGL:
            // WebGL is available on web platforms
#ifdef __EMSCRIPTEN__
            return true;
#else
            return false;
#endif

        case GraphicsBackend::Vulkan:
            // Vulkan support not implemented yet
            return false;

        case GraphicsBackend::None:
        default:
            return false;
    }
}

std::string GraphicsDeviceFactory::GetBackendName(GraphicsBackend backend) {
    switch (backend) {
        case GraphicsBackend::OpenGL:
            return "OpenGL";
        case GraphicsBackend::WebGL:
            return "WebGL";
        case GraphicsBackend::Vulkan:
            return "Vulkan";
        case GraphicsBackend::None:
        default:
            return "None";
    }
}

} // namespace Lupine
