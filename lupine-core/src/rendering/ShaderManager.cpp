#include "lupine/rendering/ShaderManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <algorithm>

namespace Lupine {

// Static member definitions
std::shared_ptr<GraphicsDevice> ShaderManager::s_device = nullptr;
std::unordered_map<std::string, ShaderCacheEntry> ShaderManager::s_shader_cache;
std::unordered_map<std::string, ShaderSource> ShaderManager::s_shader_sources;
std::vector<std::string> ShaderManager::s_shader_paths;
ShaderStats ShaderManager::s_stats;
bool ShaderManager::s_initialized = false;

bool ShaderManager::Initialize(std::shared_ptr<GraphicsDevice> device) {
    if (s_initialized) {
        return true;
    }

    if (!device) {
        std::cerr << "ShaderManager: Invalid graphics device" << std::endl;
        return false;
    }

    s_device = device;
    s_shader_cache.clear();
    s_shader_sources.clear();
    s_shader_paths.clear();
    s_stats = {};

    // Add default shader search paths
    s_shader_paths.push_back("assets/shaders");
    s_shader_paths.push_back("shaders");
    s_shader_paths.push_back("resources/shaders");

    s_initialized = true;
    std::cout << "ShaderManager initialized successfully" << std::endl;
    return true;
}

void ShaderManager::Shutdown() {
    if (!s_initialized) {
        return;
    }

    std::cout << "Shutting down ShaderManager..." << std::endl;
    
    // Clear all cached shaders
    ClearCache();
    
    s_device = nullptr;
    s_shader_sources.clear();
    s_shader_paths.clear();
    s_stats = {};
    s_initialized = false;
}

std::shared_ptr<GraphicsShader> ShaderManager::LoadShader(
    const std::string& name,
    const std::string& vertex_source,
    const std::string& fragment_source,
    const std::string& geometry_source) {
    
    if (!s_initialized || !s_device) {
        std::cerr << "ShaderManager not initialized" << std::endl;
        return nullptr;
    }

    // Create default variant
    ShaderVariant default_variant("default");
    
    return LoadShaderVariant(name, default_variant, vertex_source, fragment_source, geometry_source);
}

std::shared_ptr<GraphicsShader> ShaderManager::LoadShaderFromFile(
    const std::string& name,
    const std::string& vertex_file,
    const std::string& fragment_file,
    const std::string& geometry_file) {
    
    if (!s_initialized) {
        std::cerr << "ShaderManager not initialized" << std::endl;
        return nullptr;
    }

    // Load shader source files
    std::string vertex_source = LoadShaderFile(vertex_file);
    if (vertex_source.empty()) {
        std::cerr << "Failed to load vertex shader: " << vertex_file << std::endl;
        return nullptr;
    }

    std::string fragment_source = LoadShaderFile(fragment_file);
    if (fragment_source.empty()) {
        std::cerr << "Failed to load fragment shader: " << fragment_file << std::endl;
        return nullptr;
    }

    std::string geometry_source;
    if (!geometry_file.empty()) {
        geometry_source = LoadShaderFile(geometry_file);
        if (geometry_source.empty()) {
            std::cerr << "Failed to load geometry shader: " << geometry_file << std::endl;
            return nullptr;
        }
    }

    return LoadShader(name, vertex_source, fragment_source, geometry_source);
}

std::shared_ptr<GraphicsShader> ShaderManager::LoadShaderVariant(
    const std::string& name,
    const ShaderVariant& variant,
    const std::string& vertex_source,
    const std::string& fragment_source,
    const std::string& geometry_source) {
    
    if (!s_initialized || !s_device) {
        std::cerr << "ShaderManager not initialized" << std::endl;
        return nullptr;
    }

    GraphicsBackend backend = s_device->GetBackend();
    std::string shader_key = GenerateShaderKey(name, variant.name, backend);

    // Check cache first
    auto cache_it = s_shader_cache.find(shader_key);
    if (cache_it != s_shader_cache.end() && cache_it->second.is_valid) {
        s_stats.cache_hits++;
        return cache_it->second.shader;
    }

    s_stats.cache_misses++;

    // Process shader sources for the target backend
    std::string processed_vertex = ProcessShaderSource(vertex_source, variant, backend, ShaderType::Vertex);
    std::string processed_fragment = ProcessShaderSource(fragment_source, variant, backend, ShaderType::Fragment);
    std::string processed_geometry;
    
    if (!geometry_source.empty()) {
        processed_geometry = ProcessShaderSource(geometry_source, variant, backend, ShaderType::Geometry);
    }

    // Validate shader sources
    if (!ValidateShaderSource(processed_vertex, ShaderType::Vertex) ||
        !ValidateShaderSource(processed_fragment, ShaderType::Fragment)) {
        std::cerr << "Shader source validation failed for: " << name << std::endl;
        s_stats.failed_compilations++;
        return nullptr;
    }

    // Compile shader
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::shared_ptr<GraphicsShader> shader = s_device->CreateShader(
        processed_vertex,
        processed_fragment,
        processed_geometry
    );

    auto end_time = std::chrono::high_resolution_clock::now();
    auto compile_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    if (!shader || !shader->IsValid()) {
        std::cerr << "Failed to compile shader: " << name << std::endl;
        if (shader) {
            std::cerr << "Compilation log: " << shader->GetCompileLog() << std::endl;
        }
        s_stats.failed_compilations++;
        return nullptr;
    }

    // Cache the compiled shader
    ShaderCacheEntry cache_entry;
    cache_entry.shader = shader;
    cache_entry.source_hash = CalculateSourceHash(vertex_source + fragment_source + geometry_source);
    cache_entry.backend = backend;
    cache_entry.variant_name = variant.name;
    cache_entry.compile_time_ms = compile_time;
    cache_entry.is_valid = true;

    s_shader_cache[shader_key] = cache_entry;

    // Update statistics
    s_stats.total_shaders++;
    s_stats.compiled_shaders++;
    s_stats.total_compile_time_ms += compile_time;

    std::cout << "Compiled shader '" << name << "' (variant: " << variant.name 
              << ") in " << compile_time << "ms" << std::endl;

    return shader;
}

std::shared_ptr<GraphicsShader> ShaderManager::GetShader(
    const std::string& name,
    const std::string& variant_name) {
    
    if (!s_initialized || !s_device) {
        return nullptr;
    }

    GraphicsBackend backend = s_device->GetBackend();
    std::string shader_key = GenerateShaderKey(name, variant_name.empty() ? "default" : variant_name, backend);

    auto cache_it = s_shader_cache.find(shader_key);
    if (cache_it != s_shader_cache.end() && cache_it->second.is_valid) {
        return cache_it->second.shader;
    }

    return nullptr;
}

bool ShaderManager::IsShaderCached(const std::string& name, const std::string& variant_name) {
    if (!s_initialized || !s_device) {
        return false;
    }

    GraphicsBackend backend = s_device->GetBackend();
    std::string shader_key = GenerateShaderKey(name, variant_name.empty() ? "default" : variant_name, backend);

    auto cache_it = s_shader_cache.find(shader_key);
    return cache_it != s_shader_cache.end() && cache_it->second.is_valid;
}

void ShaderManager::RemoveShader(const std::string& name, const std::string& variant_name) {
    if (!s_initialized) {
        return;
    }

    if (variant_name.empty()) {
        // Remove all variants of the shader
        auto it = s_shader_cache.begin();
        while (it != s_shader_cache.end()) {
            if (it->first.find(name + "_") == 0) {
                it = s_shader_cache.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        // Remove specific variant
        GraphicsBackend backend = s_device ? s_device->GetBackend() : GraphicsBackend::None;
        std::string shader_key = GenerateShaderKey(name, variant_name, backend);
        s_shader_cache.erase(shader_key);
    }
}

void ShaderManager::ClearCache() {
    s_shader_cache.clear();
    s_stats.cached_shaders = 0;
    std::cout << "Shader cache cleared" << std::endl;
}

int ShaderManager::ReloadAllShaders() {
    if (!s_initialized) {
        return 0;
    }

    int reloaded_count = 0;
    std::vector<std::string> shader_keys;

    // Collect all shader keys
    for (const auto& entry : s_shader_cache) {
        shader_keys.push_back(entry.first);
    }

    // Clear cache and reload
    ClearCache();

    // TODO: Implement shader reloading from stored sources
    // This would require storing the original source code and variant information

    std::cout << "Reloaded " << reloaded_count << " shaders" << std::endl;
    return reloaded_count;
}

void ShaderManager::ResetStats() {
    s_stats = {};
}

void ShaderManager::SetShaderPaths(const std::vector<std::string>& paths) {
    s_shader_paths = paths;
}

void ShaderManager::AddShaderPath(const std::string& path) {
    if (std::find(s_shader_paths.begin(), s_shader_paths.end(), path) == s_shader_paths.end()) {
        s_shader_paths.push_back(path);
    }
}

std::string ShaderManager::ConvertShaderForBackend(
    const std::string& source,
    GraphicsBackend backend,
    ShaderType stage) {

    std::string converted = source;

    switch (backend) {
        case GraphicsBackend::WebGL: {
            // Convert GLSL to GLSL ES
            size_t version_pos = converted.find("#version");
            if (version_pos != std::string::npos) {
                size_t line_end = converted.find('\n', version_pos);
                converted.replace(version_pos, line_end - version_pos, "#version 300 es");
            } else {
                converted = "#version 300 es\n" + converted;
            }

            // Add precision qualifiers for fragment shaders
            if (stage == ShaderType::Fragment) {
                size_t insert_pos = converted.find('\n') + 1;
                converted.insert(insert_pos, "precision mediump float;\n");
            }
            break;
        }

        case GraphicsBackend::OpenGL: {
            // Ensure proper OpenGL version
            size_t version_pos = converted.find("#version");
            if (version_pos == std::string::npos) {
                converted = "#version 330 core\n" + converted;
            }
            break;
        }

        default:
            break;
    }

    return converted;
}

std::unordered_map<std::string, std::string> ShaderManager::GenerateShaderVariants(
    const std::string& base_source,
    const std::vector<ShaderVariant>& variants) {

    std::unordered_map<std::string, std::string> variant_sources;

    for (const auto& variant : variants) {
        std::string processed_source = base_source;

        // Add defines for this variant
        std::string defines_block;
        for (const auto& define : variant.defines) {
            defines_block += "#define " + define.first + " " + define.second + "\n";
        }

        // Insert defines after version directive
        size_t version_end = processed_source.find('\n');
        if (version_end != std::string::npos) {
            processed_source.insert(version_end + 1, defines_block);
        } else {
            processed_source = defines_block + processed_source;
        }

        variant_sources[variant.name] = processed_source;
    }

    return variant_sources;
}

std::string ShaderManager::LoadShaderFile(const std::string& file_path) {
    // Try to find the file in shader search paths
    std::string full_path;

    if (std::filesystem::exists(file_path)) {
        full_path = file_path;
    } else {
        for (const auto& search_path : s_shader_paths) {
            std::string candidate = search_path + "/" + file_path;
            if (std::filesystem::exists(candidate)) {
                full_path = candidate;
                break;
            }
        }
    }

    if (full_path.empty()) {
        std::cerr << "Shader file not found: " << file_path << std::endl;
        return "";
    }

    std::ifstream file(full_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << full_path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string ShaderManager::ProcessShaderSource(
    const std::string& source,
    const ShaderVariant& variant,
    GraphicsBackend backend,
    ShaderType stage) {

    std::string processed = source;

    // Apply variant defines
    std::string defines_block;
    for (const auto& define : variant.defines) {
        defines_block += "#define " + define.first + " " + define.second + "\n";
    }

    // Insert defines after version directive
    size_t version_end = processed.find('\n');
    if (version_end != std::string::npos) {
        processed.insert(version_end + 1, defines_block);
    } else {
        processed = defines_block + processed;
    }

    // Convert for target backend
    processed = ConvertShaderForBackend(processed, backend, stage);

    return processed;
}

std::string ShaderManager::GenerateShaderKey(
    const std::string& name,
    const std::string& variant_name,
    GraphicsBackend backend) {

    return name + "_" + variant_name + "_" + GetBackendName(backend);
}

std::string ShaderManager::CalculateSourceHash(const std::string& source) {
    // Simple hash calculation (in a real implementation, use a proper hash function)
    std::hash<std::string> hasher;
    return std::to_string(hasher(source));
}

bool ShaderManager::ValidateShaderSource(const std::string& source, ShaderType stage) {
    // Basic validation - check for empty source and required elements
    if (source.empty()) {
        return false;
    }

    // Check for main function
    if (source.find("void main()") == std::string::npos) {
        return false;
    }

    return true;
}

} // namespace Lupine
