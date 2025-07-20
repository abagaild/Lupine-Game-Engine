#pragma once

#include "GraphicsDevice.h"
#include <memory>
#include <string>

namespace Lupine {

/**
 * @brief Graphics device factory for creating backend-specific devices
 * 
 * This factory class handles the creation of graphics devices based on
 * the target platform and available graphics APIs. It provides automatic
 * backend detection and fallback mechanisms.
 */
class GraphicsDeviceFactory {
public:
    /**
     * @brief Backend preference for device creation
     */
    enum class BackendPreference {
        Auto,       // Automatically select best available backend
        OpenGL,     // Prefer OpenGL (desktop)
        WebGL,      // Prefer WebGL (web)
        Any         // Accept any available backend
    };

    /**
     * @brief Device creation parameters
     */
    struct DeviceParams {
        BackendPreference preference;
        bool debug_mode;
        bool vsync;
        int major_version;      // Minimum major version
        int minor_version;      // Minimum minor version
        bool core_profile;      // Use core profile (OpenGL)

        DeviceParams()
            : preference(BackendPreference::Auto)
            , debug_mode(false)
            , vsync(true)
            , major_version(3)
            , minor_version(3)
            , core_profile(true)
        {}
    };

    /**
     * @brief Create a graphics device
     * @param params Device creation parameters
     * @return Shared pointer to graphics device, nullptr if creation failed
     */
    static std::shared_ptr<GraphicsDevice> CreateDevice(const DeviceParams& params = DeviceParams());

    /**
     * @brief Detect available graphics backends
     * @return Vector of available backends
     */
    static std::vector<GraphicsBackend> GetAvailableBackends();

    /**
     * @brief Check if a specific backend is available
     * @param backend Backend to check
     * @return True if backend is available
     */
    static bool IsBackendAvailable(GraphicsBackend backend);

    /**
     * @brief Get the best available backend for the current platform
     * @return Best available backend
     */
    static GraphicsBackend GetBestBackend();

    /**
     * @brief Get backend name as string
     * @param backend Graphics backend
     * @return Backend name string
     */
    static std::string GetBackendName(GraphicsBackend backend);

    /**
     * @brief Get detailed backend information
     * @param backend Graphics backend
     * @return Detailed backend information string
     */
    static std::string GetBackendInfo(GraphicsBackend backend);

    /**
     * @brief Get default backend for current platform
     * @return Default graphics backend
     */
    static GraphicsBackend GetDefaultBackend();

    /**
     * @brief Check if backend is supported on current platform
     * @param backend Graphics backend to check
     * @return True if backend is supported
     */
    static bool IsBackendSupported(GraphicsBackend backend);

    /**
     * @brief Validate device parameters for a specific backend
     * @param backend Target backend
     * @param params Device parameters to validate
     * @return True if parameters are valid for the backend
     */
    static bool ValidateParams(GraphicsBackend backend, const DeviceParams& params);

    /**
     * @brief Get recommended parameters for a backend
     * @param backend Target backend
     * @return Recommended device parameters
     */
    static DeviceParams GetRecommendedParams(GraphicsBackend backend);

    /**
     * @brief Create device with fallback options
     * @param preferred_backends List of preferred backends in order
     * @param params Device parameters
     * @return Created device or nullptr if all backends failed
     */
    static std::shared_ptr<GraphicsDevice> CreateDeviceWithFallback(
        const std::vector<GraphicsBackend>& preferred_backends,
        const DeviceParams& params = DeviceParams()
    );

    /**
     * @brief Print available backends to console
     */
    static void PrintAvailableBackends();

    /**
     * @brief Validate device parameters
     * @param params Device parameters to validate
     * @return True if parameters are valid
     */
    static bool ValidateDeviceParams(const DeviceParams& params);

    /**
     * @brief Get the current graphics device instance
     * @return Shared pointer to current graphics device, nullptr if none exists
     */
    static std::shared_ptr<GraphicsDevice> GetDevice();

    /**
     * @brief Set the current graphics device instance
     * @param device Graphics device to set as current
     */
    static void SetDevice(std::shared_ptr<GraphicsDevice> device);

private:
    // Private constructor - static class only
    GraphicsDeviceFactory() = delete;

    // Backend detection helpers
    static bool DetectOpenGL();
    static bool DetectWebGL();
    
    // Device creation helpers
    static std::shared_ptr<GraphicsDevice> CreateOpenGLDevice(const DeviceParams& params);
    static std::shared_ptr<GraphicsDevice> CreateWebGLDevice(const DeviceParams& params);

    // Backend determination
    static GraphicsBackend DetermineBackend(BackendPreference preference);

    // Device creation for specific backend
    static std::shared_ptr<GraphicsDevice> CreateDeviceForBackend(
        GraphicsBackend backend,
        const DeviceParams& params
    );
};

/**
 * @brief Graphics backend detector utility class
 * 
 * Provides detailed information about available graphics backends
 * and their capabilities on the current platform.
 */
class GraphicsBackendDetector {
public:
    /**
     * @brief Backend detection result
     */
    struct BackendInfo {
        GraphicsBackend backend;
        bool available;
        std::string version;
        std::string renderer;
        std::string vendor;
        std::vector<std::string> extensions;
        std::string error_message;

        BackendInfo()
            : backend(GraphicsBackend::None)
            , available(false)
        {}

        BackendInfo(GraphicsBackend b)
            : backend(b)
            , available(false)
        {}
    };

    /**
     * @brief Detect all available backends with detailed information
     * @return Vector of backend information
     */
    static std::vector<BackendInfo> DetectAllBackends();

    /**
     * @brief Get detailed information about a specific backend
     * @param backend Backend to query
     * @return Backend information
     */
    static BackendInfo GetBackendInfo(GraphicsBackend backend);

    /**
     * @brief Check OpenGL availability and version
     * @return OpenGL backend information
     */
    static BackendInfo DetectOpenGL();

    /**
     * @brief Check WebGL availability and version
     * @return WebGL backend information
     */
    static BackendInfo DetectWebGL();

    /**
     * @brief Get platform-specific graphics information
     * @return Platform graphics information string
     */
    static std::string GetPlatformInfo();

    /**
     * @brief Check if running in a web environment
     * @return True if running in web browser
     */
    static bool IsWebEnvironment();

    /**
     * @brief Check if running on mobile platform
     * @return True if running on mobile device
     */
    static bool IsMobilePlatform();

    /**
     * @brief Get recommended backend for current platform
     * @return Recommended backend with reasoning
     */
    struct RecommendationResult {
        GraphicsBackend backend;
        std::string reason;
        float confidence; // 0.0 to 1.0

        RecommendationResult()
            : backend(GraphicsBackend::None)
            , confidence(0.0f)
        {}
    };
    
    static RecommendationResult GetRecommendedBackend();

private:
    // Private constructor - static class only
    GraphicsBackendDetector() = delete;

    // Platform detection helpers
    static bool IsEmscripten();
    static bool IsAndroid();
    static bool IsIOS();
    static std::string GetPlatformName();
};

} // namespace Lupine
