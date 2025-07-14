#pragma once

#include "GraphicsDevice.h"
#include "GraphicsShader.h"
#include "GraphicsBackend.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Lupine {

/**
 * @brief Shader variant configuration
 * 
 * Defines different shader variants that can be generated
 * based on feature flags and backend requirements.
 */
struct ShaderVariant {
    std::string name;
    std::unordered_map<std::string, std::string> defines;
    GraphicsBackend target_backend = GraphicsBackend::None;
    
    ShaderVariant() = default;
    ShaderVariant(const std::string& variant_name) : name(variant_name) {}
};

/**
 * @brief Shader source container
 * 
 * Contains shader source code for different stages and variants.
 */
struct ShaderSource {
    std::string vertex_source;
    std::string fragment_source;
    std::string geometry_source;
    std::string compute_source;
    
    // Metadata
    std::string name;
    std::string file_path;
    GraphicsBackend target_backend = GraphicsBackend::None;
    std::vector<ShaderVariant> variants;
    
    ShaderSource() = default;
    ShaderSource(const std::string& shader_name) : name(shader_name) {}
};

/**
 * @brief Compiled shader cache entry
 */
struct ShaderCacheEntry {
    std::shared_ptr<GraphicsShader> shader;
    std::string source_hash;
    GraphicsBackend backend;
    std::string variant_name;
    uint64_t compile_time_ms = 0;
    bool is_valid = false;
};

/**
 * @brief Shader compilation statistics
 */
struct ShaderStats {
    uint32_t total_shaders = 0;
    uint32_t cached_shaders = 0;
    uint32_t compiled_shaders = 0;
    uint32_t failed_compilations = 0;
    uint64_t total_compile_time_ms = 0;
    uint64_t cache_hits = 0;
    uint64_t cache_misses = 0;
};

/**
 * @brief Shader manager for backend-agnostic shader compilation and caching
 * 
 * The ShaderManager provides a unified interface for shader compilation,
 * caching, and management across different graphics backends. It handles
 * automatic shader variant generation, backend-specific optimizations,
 * and efficient caching mechanisms.
 */
class ShaderManager {
public:
    /**
     * @brief Initialize the shader manager
     * @param device Graphics device to use for shader compilation
     * @return True if initialization succeeded
     */
    static bool Initialize(std::shared_ptr<GraphicsDevice> device);

    /**
     * @brief Shutdown the shader manager
     */
    static void Shutdown();

    /**
     * @brief Load shader from source code
     * @param name Shader name for caching
     * @param vertex_source Vertex shader source
     * @param fragment_source Fragment shader source
     * @param geometry_source Geometry shader source (optional)
     * @return Shared pointer to compiled shader
     */
    static std::shared_ptr<GraphicsShader> LoadShader(
        const std::string& name,
        const std::string& vertex_source,
        const std::string& fragment_source,
        const std::string& geometry_source = ""
    );

    /**
     * @brief Load shader from file
     * @param name Shader name for caching
     * @param vertex_file Path to vertex shader file
     * @param fragment_file Path to fragment shader file
     * @param geometry_file Path to geometry shader file (optional)
     * @return Shared pointer to compiled shader
     */
    static std::shared_ptr<GraphicsShader> LoadShaderFromFile(
        const std::string& name,
        const std::string& vertex_file,
        const std::string& fragment_file,
        const std::string& geometry_file = ""
    );

    /**
     * @brief Load shader with variant
     * @param name Shader name
     * @param variant Shader variant configuration
     * @param vertex_source Vertex shader source
     * @param fragment_source Fragment shader source
     * @param geometry_source Geometry shader source (optional)
     * @return Shared pointer to compiled shader
     */
    static std::shared_ptr<GraphicsShader> LoadShaderVariant(
        const std::string& name,
        const ShaderVariant& variant,
        const std::string& vertex_source,
        const std::string& fragment_source,
        const std::string& geometry_source = ""
    );

    /**
     * @brief Get cached shader
     * @param name Shader name
     * @param variant_name Variant name (empty for default)
     * @return Shared pointer to cached shader, nullptr if not found
     */
    static std::shared_ptr<GraphicsShader> GetShader(
        const std::string& name,
        const std::string& variant_name = ""
    );

    /**
     * @brief Check if shader is cached
     * @param name Shader name
     * @param variant_name Variant name (empty for default)
     * @return True if shader is cached
     */
    static bool IsShaderCached(
        const std::string& name,
        const std::string& variant_name = ""
    );

    /**
     * @brief Remove shader from cache
     * @param name Shader name
     * @param variant_name Variant name (empty for all variants)
     */
    static void RemoveShader(
        const std::string& name,
        const std::string& variant_name = ""
    );

    /**
     * @brief Clear all cached shaders
     */
    static void ClearCache();

    /**
     * @brief Reload all shaders (useful for hot-reloading)
     * @return Number of shaders successfully reloaded
     */
    static int ReloadAllShaders();

    /**
     * @brief Get shader compilation statistics
     * @return Shader statistics structure
     */
    static const ShaderStats& GetStats() { return s_stats; }

    /**
     * @brief Reset shader statistics
     */
    static void ResetStats();

    /**
     * @brief Set shader search paths
     * @param paths Vector of directory paths to search for shader files
     */
    static void SetShaderPaths(const std::vector<std::string>& paths);

    /**
     * @brief Add shader search path
     * @param path Directory path to add to shader search
     */
    static void AddShaderPath(const std::string& path);

    /**
     * @brief Convert shader source for target backend
     * @param source Original shader source
     * @param backend Target graphics backend
     * @param stage Shader stage
     * @return Backend-compatible shader source
     */
    static std::string ConvertShaderForBackend(
        const std::string& source,
        GraphicsBackend backend,
        ShaderType stage
    );

    /**
     * @brief Generate shader variants
     * @param base_source Base shader source
     * @param variants Vector of variant configurations
     * @return Map of variant name to processed source
     */
    static std::unordered_map<std::string, std::string> GenerateShaderVariants(
        const std::string& base_source,
        const std::vector<ShaderVariant>& variants
    );

private:
    // Internal methods
    static std::string LoadShaderFile(const std::string& file_path);
    static std::string ProcessShaderSource(
        const std::string& source,
        const ShaderVariant& variant,
        GraphicsBackend backend,
        ShaderType stage
    );
    static std::string GenerateShaderKey(
        const std::string& name,
        const std::string& variant_name,
        GraphicsBackend backend
    );
    static std::string CalculateSourceHash(const std::string& source);
    static bool ValidateShaderSource(const std::string& source, ShaderType stage);

    // Static members
    static std::shared_ptr<GraphicsDevice> s_device;
    static std::unordered_map<std::string, ShaderCacheEntry> s_shader_cache;
    static std::unordered_map<std::string, ShaderSource> s_shader_sources;
    static std::vector<std::string> s_shader_paths;
    static ShaderStats s_stats;
    static bool s_initialized;
};

} // namespace Lupine
