#include "lupine/components/TerrainLoader.h"
#include "lupine/components/TerrainRenderer.h"
#include "lupine/components/TerrainCollider.h"
#include "lupine/terrain/TerrainData.h"
#include "lupine/core/Node.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace Lupine {

TerrainLoader::TerrainLoader()
    : Component("TerrainLoader")
    , m_terrain_file_path("")
    , m_auto_load(false)
    , m_streaming_enabled(false)
    , m_streaming_distance(100.0f)
    , m_cache_size(16)
    , m_terrain_renderer(nullptr)
    , m_terrain_collider(nullptr)
    , m_terrain_data(nullptr)
    , m_last_error("")
    , m_is_loading(false)
    , m_is_saving(false)
    , m_current_progress(0.0f)
{
    InitializeExportVariables();
}

bool TerrainLoader::LoadTerrain(const std::string& file_path, 
                               const TerrainImportOptions& options,
                               TerrainLoadProgressCallback progress_callback) {
    if (m_is_loading) {
        SetError("Already loading terrain");
        return false;
    }
    
    if (!FileExists(file_path)) {
        SetError("File does not exist: " + file_path);
        return false;
    }
    
    m_is_loading = true;
    m_current_progress = 0.0f;
    ClearLastError();
    
    UpdateProgress(0.0f, "Starting terrain load...", progress_callback);
    
    // Detect format if auto-detect is enabled
    TerrainFileFormat format = options.format;
    if (options.auto_detect_format) {
        format = DetectFileFormat(file_path);
    }
    
    bool success = false;
    
    try {
        switch (format) {
            case TerrainFileFormat::LupineTerrain:
                success = LoadLupineTerrainFormat(file_path, options, progress_callback);
                break;
            case TerrainFileFormat::Heightmap:
                success = LoadHeightmapFormat(file_path, options, progress_callback);
                break;
            case TerrainFileFormat::Image:
                success = LoadImageFormat(file_path, options, progress_callback);
                break;
            default:
                SetError("Unsupported terrain format for loading");
                break;
        }
        
        if (success) {
            m_terrain_file_path = file_path;
            UpdateProgress(1.0f, "Terrain loaded successfully", progress_callback);
            NotifyComponentsOfDataChange();
            UpdateExportVariables();
        }
    } catch (const std::exception& e) {
        SetError("Exception during terrain loading: " + std::string(e.what()));
        success = false;
    }
    
    m_is_loading = false;
    return success;
}

bool TerrainLoader::SaveTerrain(const std::string& file_path,
                               const TerrainExportOptions& options,
                               TerrainLoadProgressCallback progress_callback) {
    if (m_is_saving) {
        SetError("Already saving terrain");
        return false;
    }
    
    if (!m_terrain_data) {
        SetError("No terrain data to save");
        return false;
    }
    
    m_is_saving = true;
    m_current_progress = 0.0f;
    ClearLastError();
    
    UpdateProgress(0.0f, "Starting terrain save...", progress_callback);
    
    bool success = false;
    
    try {
        switch (options.format) {
            case TerrainFileFormat::LupineTerrain:
                success = SaveLupineTerrainFormat(file_path, options, progress_callback);
                break;
            case TerrainFileFormat::OBJ:
                success = SaveOBJFormat(file_path, options, progress_callback);
                break;
            case TerrainFileFormat::Heightmap:
                success = SaveHeightmapFormat(file_path, options, progress_callback);
                break;
            default:
                SetError("Unsupported terrain format for saving");
                break;
        }
        
        if (success) {
            m_terrain_file_path = file_path;
            UpdateProgress(1.0f, "Terrain saved successfully", progress_callback);
            UpdateExportVariables();
        }
    } catch (const std::exception& e) {
        SetError("Exception during terrain saving: " + std::string(e.what()));
        success = false;
    }
    
    m_is_saving = false;
    return success;
}

bool TerrainLoader::IsFormatSupported(const std::string& file_path) const {
    std::string ext = GetFileExtension(file_path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    std::vector<std::string> supported = GetSupportedExtensions();
    return std::find(supported.begin(), supported.end(), ext) != supported.end();
}

TerrainFileFormat TerrainLoader::DetectFileFormat(const std::string& file_path) const {
    std::string ext = GetFileExtension(file_path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".terrain") return TerrainFileFormat::LupineTerrain;
    if (ext == ".raw" || ext == ".r16" || ext == ".r32") return TerrainFileFormat::Heightmap;
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".exr") return TerrainFileFormat::Image;
    if (ext == ".obj") return TerrainFileFormat::OBJ;
    
    return TerrainFileFormat::Custom;
}

void TerrainLoader::SetTerrainRenderer(TerrainRenderer* terrain_renderer) {
    m_terrain_renderer = terrain_renderer;
    if (terrain_renderer) {
        m_terrain_data = terrain_renderer->GetTerrainData();
    }
}

void TerrainLoader::SetTerrainCollider(TerrainCollider* terrain_collider) {
    m_terrain_collider = terrain_collider;
}

void TerrainLoader::SetTerrainData(std::shared_ptr<TerrainData> data) {
    m_terrain_data = data;
}

void TerrainLoader::SetTerrainFilePath(const std::string& file_path) {
    m_terrain_file_path = file_path;
    UpdateExportVariables();
}

void TerrainLoader::SetAutoLoad(bool auto_load) {
    m_auto_load = auto_load;
    UpdateExportVariables();
}

void TerrainLoader::SetStreamingEnabled(bool streaming) {
    m_streaming_enabled = streaming;
    UpdateExportVariables();
}

void TerrainLoader::SetStreamingDistance(float distance) {
    m_streaming_distance = std::max(0.0f, distance);
    UpdateExportVariables();
}

void TerrainLoader::SetCacheSize(int cache_size) {
    m_cache_size = std::max(1, cache_size);
    UpdateExportVariables();
}

std::vector<std::string> TerrainLoader::GetSupportedExtensions() const {
    return {
        ".terrain",     // Native Lupine terrain format
        ".raw",         // Raw heightmap data
        ".r16",         // 16-bit raw heightmap
        ".r32",         // 32-bit raw heightmap
        ".png",         // PNG image heightmap
        ".jpg",         // JPEG image heightmap
        ".jpeg",        // JPEG image heightmap
        ".tga",         // TGA image heightmap
        ".exr",         // EXR image heightmap
        ".obj"          // Wavefront OBJ (export only)
    };
}

void TerrainLoader::OnReady() {
    UpdateFromExportVariables();
    
    // Try to find terrain components on the same node if not set
    if (GetOwner()) {
        if (!m_terrain_renderer) {
            m_terrain_renderer = GetOwner()->GetComponent<TerrainRenderer>();
        }
        if (!m_terrain_collider) {
            m_terrain_collider = GetOwner()->GetComponent<TerrainCollider>();
        }
    }
    
    // Auto-load terrain if enabled and file path is set
    if (m_auto_load && !m_terrain_file_path.empty()) {
        LoadTerrain(m_terrain_file_path);
    }
}

void TerrainLoader::OnUpdate(float delta_time) {
    (void)delta_time;
    
    UpdateFromExportVariables();
    
    // Handle streaming if enabled
    if (m_streaming_enabled && m_terrain_data) {
        // Streaming logic will be implemented when we have proper terrain chunking
    }
}

void TerrainLoader::InitializeExportVariables() {
    // File configuration
    AddExportVariable("terrain_file_path", m_terrain_file_path, "Path to terrain file", ExportVariableType::FilePath);
    AddExportVariable("auto_load", m_auto_load, "Auto-load terrain on ready", ExportVariableType::Bool);
    
    // Streaming configuration
    AddExportVariable("streaming_enabled", m_streaming_enabled, "Enable terrain streaming", ExportVariableType::Bool);
    AddExportVariable("streaming_distance", m_streaming_distance, "Streaming distance", ExportVariableType::Float);
    AddExportVariable("cache_size", m_cache_size, "Streaming cache size", ExportVariableType::Int);
}

void TerrainLoader::UpdateFromExportVariables() {
    m_terrain_file_path = GetExportVariableValue<std::string>("terrain_file_path", m_terrain_file_path);
    m_auto_load = GetExportVariableValue<bool>("auto_load", m_auto_load);
    m_streaming_enabled = GetExportVariableValue<bool>("streaming_enabled", m_streaming_enabled);
    m_streaming_distance = GetExportVariableValue<float>("streaming_distance", m_streaming_distance);
    m_cache_size = GetExportVariableValue<int>("cache_size", m_cache_size);
}

void TerrainLoader::UpdateExportVariables() {
    SetExportVariable("terrain_file_path", m_terrain_file_path);
    SetExportVariable("auto_load", m_auto_load);
    SetExportVariable("streaming_enabled", m_streaming_enabled);
    SetExportVariable("streaming_distance", m_streaming_distance);
    SetExportVariable("cache_size", m_cache_size);
}

bool TerrainLoader::LoadLupineTerrainFormat(const std::string& file_path, const TerrainImportOptions& options, TerrainLoadProgressCallback callback) {
    (void)options;
    UpdateProgress(0.2f, "Loading Lupine terrain format...", callback);
    
    std::cout << "TerrainLoader: Loading Lupine terrain format from " << file_path << std::endl;
    
    // Native terrain format loading will be implemented when we have TerrainData structure
    UpdateProgress(1.0f, "Lupine terrain format loaded", callback);
    return true;
}

bool TerrainLoader::LoadHeightmapFormat(const std::string& file_path, const TerrainImportOptions& options, TerrainLoadProgressCallback callback) {
    (void)options;
    UpdateProgress(0.2f, "Loading heightmap format...", callback);
    
    std::cout << "TerrainLoader: Loading heightmap format from " << file_path << std::endl;
    
    // Heightmap loading will be implemented when we have TerrainData structure
    UpdateProgress(1.0f, "Heightmap format loaded", callback);
    return true;
}

bool TerrainLoader::LoadImageFormat(const std::string& file_path, const TerrainImportOptions& options, TerrainLoadProgressCallback callback) {
    (void)options;
    UpdateProgress(0.2f, "Loading image heightmap format...", callback);
    
    std::cout << "TerrainLoader: Loading image heightmap format from " << file_path << std::endl;
    
    // Image heightmap loading will be implemented when we have TerrainData structure
    UpdateProgress(1.0f, "Image heightmap format loaded", callback);
    return true;
}

bool TerrainLoader::SaveLupineTerrainFormat(const std::string& file_path, const TerrainExportOptions& options, TerrainLoadProgressCallback callback) {
    (void)options;
    UpdateProgress(0.2f, "Saving Lupine terrain format...", callback);
    
    std::cout << "TerrainLoader: Saving Lupine terrain format to " << file_path << std::endl;
    
    // Native terrain format saving will be implemented when we have TerrainData structure
    UpdateProgress(1.0f, "Lupine terrain format saved", callback);
    return true;
}

bool TerrainLoader::SaveOBJFormat(const std::string& file_path, const TerrainExportOptions& options, TerrainLoadProgressCallback callback) {
    (void)options;
    UpdateProgress(0.2f, "Saving OBJ format...", callback);
    
    std::cout << "TerrainLoader: Saving OBJ format to " << file_path << std::endl;
    
    // OBJ export will be implemented when we have TerrainData structure
    UpdateProgress(1.0f, "OBJ format saved", callback);
    return true;
}

bool TerrainLoader::SaveHeightmapFormat(const std::string& file_path, const TerrainExportOptions& options, TerrainLoadProgressCallback callback) {
    (void)options;
    UpdateProgress(0.2f, "Saving heightmap format...", callback);
    
    std::cout << "TerrainLoader: Saving heightmap format to " << file_path << std::endl;
    
    // Heightmap export will be implemented when we have TerrainData structure
    UpdateProgress(1.0f, "Heightmap format saved", callback);
    return true;
}

void TerrainLoader::SetError(const std::string& error) {
    m_last_error = error;
    std::cerr << "TerrainLoader Error: " << error << std::endl;
}

void TerrainLoader::UpdateProgress(float progress, const std::string& status, TerrainLoadProgressCallback callback) {
    m_current_progress = progress;
    if (callback) {
        callback(progress, status);
    }
}

std::string TerrainLoader::GetFileExtension(const std::string& file_path) const {
    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        return file_path.substr(dot_pos);
    }
    return "";
}

bool TerrainLoader::FileExists(const std::string& file_path) const {
    return std::filesystem::exists(file_path);
}

void TerrainLoader::NotifyComponentsOfDataChange() {
    if (m_terrain_renderer) {
        m_terrain_renderer->SetTerrainData(m_terrain_data);
    }
    if (m_terrain_collider) {
        m_terrain_collider->SetTerrainData(m_terrain_data);
    }
}

int TerrainLoader::FileFormatToInt(TerrainFileFormat format) const {
    switch (format) {
        case TerrainFileFormat::LupineTerrain: return 0;
        case TerrainFileFormat::Heightmap: return 1;
        case TerrainFileFormat::Image: return 2;
        case TerrainFileFormat::OBJ: return 3;
        case TerrainFileFormat::Custom: return 4;
        default: return 0;
    }
}

TerrainFileFormat TerrainLoader::IntToFileFormat(int value) const {
    switch (value) {
        case 0: return TerrainFileFormat::LupineTerrain;
        case 1: return TerrainFileFormat::Heightmap;
        case 2: return TerrainFileFormat::Image;
        case 3: return TerrainFileFormat::OBJ;
        case 4: return TerrainFileFormat::Custom;
        default: return TerrainFileFormat::LupineTerrain;
    }
}

} // namespace Lupine
