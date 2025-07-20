#include "lupine/components/Tilemap3D.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/resources/ResourceManager.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <algorithm>

namespace Lupine {

using json = nlohmann::json;

Tilemap3D::Tilemap3D()
    : Component("Tilemap3D")
    , m_tileset_path("")
    , m_tileset_resource(nullptr)
    , m_tileset_loaded(false)
    , m_tilemap_data(glm::ivec3(10, 1, 10))
    , m_tilemap_data_serialized("")
    , m_tile_size(1.0f, 1.0f, 1.0f)
    , m_modulate(1.0f, 1.0f, 1.0f, 1.0f)
    , m_cast_shadows(true)
    , m_receive_shadows(true)
    , m_frustum_culling_enabled(true)
    , m_culling_distance(100.0f)
    , m_lod_enabled(false)
    , m_lod_distance(50.0f)
    , m_collision_enabled(false)
    , m_visible_tile_count(0)
    , m_culled_tile_count(0)
    , m_visible_tile_min(0)
    , m_visible_tile_max(0)
    , m_bounds_dirty(true)
{
    InitializeExportVariables();
}

void Tilemap3D::SetTilesetPath(const std::string& path) {
    m_tileset_path = path;
    SetExportVariable("tileset_path", path);
    m_tileset_loaded = false;
    LoadTileset();
}

void Tilemap3D::SetTilemapData(const Tilemap3DData& data) {
    m_tilemap_data = data;
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_bounds_dirty = true;
}

void Tilemap3D::SetMapSize(const glm::ivec3& size) {
    m_tilemap_data.Resize(size);
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("map_size", glm::vec3(size.x, size.y, size.z));
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_bounds_dirty = true;
}

void Tilemap3D::SetTileSize(const glm::vec3& size) {
    m_tile_size = size;
    SetExportVariable("tile_size", size);
    m_bounds_dirty = true;
}

void Tilemap3D::SetTile(int x, int y, int z, int tile_id) {
    m_tilemap_data.SetTile(x, y, z, tile_id);
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_bounds_dirty = true;
}

void Tilemap3D::SetModulate(const glm::vec4& modulate) {
    m_modulate = modulate;
    SetExportVariable("modulate", modulate);
}

void Tilemap3D::SetCastShadows(bool cast_shadows) {
    m_cast_shadows = cast_shadows;
    SetExportVariable("cast_shadows", cast_shadows);
}

void Tilemap3D::SetReceiveShadows(bool receive_shadows) {
    m_receive_shadows = receive_shadows;
    SetExportVariable("receive_shadows", receive_shadows);
}

void Tilemap3D::SetFrustumCullingEnabled(bool enabled) {
    m_frustum_culling_enabled = enabled;
    SetExportVariable("frustum_culling_enabled", enabled);
}

void Tilemap3D::SetCullingDistance(float distance) {
    m_culling_distance = distance;
    SetExportVariable("culling_distance", distance);
}

void Tilemap3D::SetLODEnabled(bool enabled) {
    m_lod_enabled = enabled;
    SetExportVariable("lod_enabled", enabled);
}

void Tilemap3D::SetLODDistance(float distance) {
    m_lod_distance = distance;
    SetExportVariable("lod_distance", distance);
}

void Tilemap3D::SetCollisionEnabled(bool enabled) {
    m_collision_enabled = enabled;
    SetExportVariable("collision_enabled", enabled);
}

glm::vec3 Tilemap3D::MapToLocal(const glm::ivec3& map_pos) const {
    return glm::vec3(
        map_pos.x * m_tile_size.x,
        map_pos.y * m_tile_size.y,
        map_pos.z * m_tile_size.z
    );
}

glm::ivec3 Tilemap3D::LocalToMap(const glm::vec3& local_pos) const {
    return glm::ivec3(
        static_cast<int>(local_pos.x / m_tile_size.x),
        static_cast<int>(local_pos.y / m_tile_size.y),
        static_cast<int>(local_pos.z / m_tile_size.z)
    );
}

glm::vec3 Tilemap3D::GetTileWorldPosition(int x, int y, int z) const {
    Node3D* node3d = dynamic_cast<Node3D*>(GetOwner());
    if (!node3d) return glm::vec3(0.0f);
    
    glm::vec3 local_pos = MapToLocal(glm::ivec3(x, y, z));
    glm::vec4 world_pos = node3d->GetGlobalTransform() * glm::vec4(local_pos, 1.0f);
    return glm::vec3(world_pos);
}

void Tilemap3D::ClearTilemap() {
    m_tilemap_data.Clear();
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_bounds_dirty = true;
}

void Tilemap3D::FillTilemap(int tile_id) {
    for (int z = 0; z < m_tilemap_data.size.z; ++z) {
        for (int y = 0; y < m_tilemap_data.size.y; ++y) {
            for (int x = 0; x < m_tilemap_data.size.x; ++x) {
                m_tilemap_data.SetTile(x, y, z, tile_id);
            }
        }
    }
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_bounds_dirty = true;
}

void Tilemap3D::FillLayer(int layer, int tile_id) {
    if (layer < 0 || layer >= m_tilemap_data.size.y) return;
    
    for (int z = 0; z < m_tilemap_data.size.z; ++z) {
        for (int x = 0; x < m_tilemap_data.size.x; ++x) {
            m_tilemap_data.SetTile(x, layer, z, tile_id);
        }
    }
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_bounds_dirty = true;
}

void Tilemap3D::FloodFill3D(int x, int y, int z, int tile_id) {
    if (x < 0 || x >= m_tilemap_data.size.x || 
        y < 0 || y >= m_tilemap_data.size.y || 
        z < 0 || z >= m_tilemap_data.size.z) {
        return;
    }
    
    int original_tile = m_tilemap_data.GetTile(x, y, z);
    if (original_tile == tile_id) return;
    
    // 3D flood fill implementation
    std::vector<glm::ivec3> stack;
    stack.push_back(glm::ivec3(x, y, z));
    
    while (!stack.empty()) {
        glm::ivec3 pos = stack.back();
        stack.pop_back();
        
        if (pos.x < 0 || pos.x >= m_tilemap_data.size.x || 
            pos.y < 0 || pos.y >= m_tilemap_data.size.y || 
            pos.z < 0 || pos.z >= m_tilemap_data.size.z) {
            continue;
        }
        
        if (m_tilemap_data.GetTile(pos.x, pos.y, pos.z) != original_tile) {
            continue;
        }
        
        m_tilemap_data.SetTile(pos.x, pos.y, pos.z, tile_id);
        
        // Add 6 neighbors (3D)
        stack.push_back(glm::ivec3(pos.x + 1, pos.y, pos.z));
        stack.push_back(glm::ivec3(pos.x - 1, pos.y, pos.z));
        stack.push_back(glm::ivec3(pos.x, pos.y + 1, pos.z));
        stack.push_back(glm::ivec3(pos.x, pos.y - 1, pos.z));
        stack.push_back(glm::ivec3(pos.x, pos.y, pos.z + 1));
        stack.push_back(glm::ivec3(pos.x, pos.y, pos.z - 1));
    }
    
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_bounds_dirty = true;
}

std::string Tilemap3D::SerializeTilemapData() const {
    json j;
    j["size"] = {m_tilemap_data.size.x, m_tilemap_data.size.y, m_tilemap_data.size.z};
    j["tiles"] = m_tilemap_data.tiles;
    return j.dump();
}

bool Tilemap3D::DeserializeTilemapData(const std::string& data) {
    try {
        json j = json::parse(data);
        
        glm::ivec3 size(j["size"][0], j["size"][1], j["size"][2]);
        std::vector<int> tiles = j["tiles"].get<std::vector<int>>();
        
        m_tilemap_data.size = size;
        m_tilemap_data.tiles = tiles;
        
        // Ensure tiles vector has correct size
        if (m_tilemap_data.tiles.size() != static_cast<size_t>(size.x * size.y * size.z)) {
            m_tilemap_data.tiles.resize(size.x * size.y * size.z, -1);
        }
        
        m_bounds_dirty = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing tilemap3D data: " << e.what() << std::endl;
        return false;
    }
}

void Tilemap3D::OnReady() {
    LoadTileset();
}

void Tilemap3D::OnUpdate(float delta_time) {
    (void)delta_time;
    
    // Check if export variables have changed
    UpdateFromExportVariables();
    
    // Check rendering context - Tilemap3D should only render in 3D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor2D) {
        return; // Don't render 3D tilemaps in 2D editor view
    }
    
    // Update visible tile bounds if needed
    if (m_bounds_dirty) {
        UpdateVisibleTiles();
        m_bounds_dirty = false;
    }
    
    // Render the tilemap
    RenderTilemap();
}

void Tilemap3D::InitializeExportVariables() {
    AddExportVariable("tileset_path", m_tileset_path, "Path to tileset3D resource file", ExportVariableType::FilePath);
    AddExportVariable("map_size", glm::vec3(m_tilemap_data.size.x, m_tilemap_data.size.y, m_tilemap_data.size.z), "Map size in tiles (width, height, depth)", ExportVariableType::Vec3);
    AddExportVariable("tile_size", m_tile_size, "Tile size in world units", ExportVariableType::Vec3);
    AddExportVariable("tilemap_data", m_tilemap_data_serialized, "Serialized tilemap data", ExportVariableType::String);
    AddExportVariable("modulate", m_modulate, "Color modulation (RGBA)", ExportVariableType::Color);
    AddExportVariable("cast_shadows", m_cast_shadows, "Cast shadows", ExportVariableType::Bool);
    AddExportVariable("receive_shadows", m_receive_shadows, "Receive shadows", ExportVariableType::Bool);
    AddExportVariable("frustum_culling_enabled", m_frustum_culling_enabled, "Enable frustum culling", ExportVariableType::Bool);
    AddExportVariable("culling_distance", m_culling_distance, "Maximum culling distance", ExportVariableType::Float);
    AddExportVariable("lod_enabled", m_lod_enabled, "Enable Level of Detail", ExportVariableType::Bool);
    AddExportVariable("lod_distance", m_lod_distance, "LOD transition distance", ExportVariableType::Float);
    AddExportVariable("collision_enabled", m_collision_enabled, "Enable collision detection", ExportVariableType::Bool);
}

void Tilemap3D::LoadTileset() {
    if (m_tileset_path.empty()) {
        m_tileset_loaded = false;
        m_tileset_resource.reset();
        return;
    }

    try {
        m_tileset_resource = std::make_unique<Tileset3DResource>();
        if (m_tileset_resource->LoadFromFile(m_tileset_path)) {
            m_tileset_loaded = true;
            std::cout << "Loaded tileset3D for Tilemap3D: " << m_tileset_path << std::endl;
        } else {
            std::cerr << "Failed to load tileset3D for Tilemap3D: " << m_tileset_path << std::endl;
            m_tileset_loaded = false;
            m_tileset_resource.reset();
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception loading tileset3D " << m_tileset_path << ": " << e.what() << std::endl;
        m_tileset_loaded = false;
        m_tileset_resource.reset();
    }
}

void Tilemap3D::UpdateFromExportVariables() {
    // Check for changes in export variables
    std::string new_tileset_path = GetExportVariableValue<std::string>("tileset_path", "");
    if (new_tileset_path != m_tileset_path) {
        SetTilesetPath(new_tileset_path);
    }

    glm::vec3 new_map_size = GetExportVariableValue<glm::vec3>("map_size", glm::vec3(10, 1, 10));
    glm::ivec3 new_map_size_int(static_cast<int>(new_map_size.x), static_cast<int>(new_map_size.y), static_cast<int>(new_map_size.z));
    if (new_map_size_int != m_tilemap_data.size) {
        SetMapSize(new_map_size_int);
    }

    glm::vec3 new_tile_size = GetExportVariableValue<glm::vec3>("tile_size", glm::vec3(1.0f));
    if (new_tile_size != m_tile_size) {
        SetTileSize(new_tile_size);
    }

    std::string new_tilemap_data = GetExportVariableValue<std::string>("tilemap_data", "");
    if (new_tilemap_data != m_tilemap_data_serialized && !new_tilemap_data.empty()) {
        if (DeserializeTilemapData(new_tilemap_data)) {
            m_tilemap_data_serialized = new_tilemap_data;
        }
    }

    m_modulate = GetExportVariableValue<glm::vec4>("modulate", glm::vec4(1.0f));
    m_cast_shadows = GetExportVariableValue<bool>("cast_shadows", true);
    m_receive_shadows = GetExportVariableValue<bool>("receive_shadows", true);
    m_frustum_culling_enabled = GetExportVariableValue<bool>("frustum_culling_enabled", true);
    m_culling_distance = GetExportVariableValue<float>("culling_distance", 100.0f);
    m_lod_enabled = GetExportVariableValue<bool>("lod_enabled", false);
    m_lod_distance = GetExportVariableValue<float>("lod_distance", 50.0f);
    m_collision_enabled = GetExportVariableValue<bool>("collision_enabled", false);
}

void Tilemap3D::UpdateExportVariables() {
    SetExportVariable("tileset_path", m_tileset_path);
    SetExportVariable("map_size", glm::vec3(m_tilemap_data.size.x, m_tilemap_data.size.y, m_tilemap_data.size.z));
    SetExportVariable("tile_size", m_tile_size);
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    SetExportVariable("modulate", m_modulate);
    SetExportVariable("cast_shadows", m_cast_shadows);
    SetExportVariable("receive_shadows", m_receive_shadows);
    SetExportVariable("frustum_culling_enabled", m_frustum_culling_enabled);
    SetExportVariable("culling_distance", m_culling_distance);
    SetExportVariable("lod_enabled", m_lod_enabled);
    SetExportVariable("lod_distance", m_lod_distance);
    SetExportVariable("collision_enabled", m_collision_enabled);
}

void Tilemap3D::RenderTilemap() {
    if (!m_tileset_loaded || !m_tileset_resource) {
        return;
    }

    Node3D* node3d = dynamic_cast<Node3D*>(GetOwner());
    if (!node3d) {
        return;
    }

    // Reset performance counters
    m_visible_tile_count = 0;
    m_culled_tile_count = 0;

    // Render visible tiles with frustum culling
    for (int z = m_visible_tile_min.z; z <= m_visible_tile_max.z; ++z) {
        for (int y = m_visible_tile_min.y; y <= m_visible_tile_max.y; ++y) {
            for (int x = m_visible_tile_min.x; x <= m_visible_tile_max.x; ++x) {
                int tile_id = m_tilemap_data.GetTile(x, y, z);
                if (tile_id >= 0) {
                    if (IsTileVisible(x, y, z, glm::vec3(0.0f))) { // TODO: Get actual camera position
                        RenderTile(x, y, z, tile_id, node3d);
                        m_visible_tile_count++;
                    } else {
                        m_culled_tile_count++;
                    }
                }
            }
        }
    }
}

void Tilemap3D::RenderTile(int x, int y, int z, int tile_id, Node3D* node3d) {
    if (!m_tileset_loaded || !m_tileset_resource) {
        return;
    }

    const Tile3DData* tile = m_tileset_resource->GetTile(tile_id);
    if (!tile || !tile->model_loaded || !tile->model) {
        return;
    }

    // Calculate tile transform
    glm::mat4 tile_transform = CalculateTileTransform(x, y, z, node3d);

    // Apply tile's default transform
    glm::mat4 tile_local_transform = glm::mat4(1.0f);
    tile_local_transform = glm::translate(tile_local_transform, tile->default_transform.position);
    tile_local_transform = tile_local_transform * glm::mat4_cast(tile->default_transform.rotation);
    tile_local_transform = glm::scale(tile_local_transform, tile->default_transform.scale);

    glm::mat4 final_transform = tile_transform * tile_local_transform;

    // Render the 3D model
    if (tile->model && tile->model->IsLoaded()) {
        for (const auto& mesh : tile->model->GetMeshes()) {
            Renderer::RenderMesh(mesh, final_transform, m_modulate, 0, true);
        }
    }
}

glm::mat4 Tilemap3D::CalculateTileTransform(int x, int y, int z, Node3D* node3d) const {
    // Get base transform from node
    glm::mat4 base_transform = node3d->GetGlobalTransform();

    // Calculate tile position in local space
    glm::vec3 tile_pos = MapToLocal(glm::ivec3(x, y, z));

    // Create tile transform
    glm::mat4 tile_transform = glm::translate(base_transform, tile_pos);
    tile_transform = glm::scale(tile_transform, m_tile_size);

    return tile_transform;
}

void Tilemap3D::UpdateVisibleTiles() {
    // For now, render all tiles
    // TODO: Implement proper frustum culling based on camera view
    m_visible_tile_min = glm::ivec3(0);
    m_visible_tile_max = glm::ivec3(
        m_tilemap_data.size.x - 1,
        m_tilemap_data.size.y - 1,
        m_tilemap_data.size.z - 1
    );
}

bool Tilemap3D::IsTileVisible(int x, int y, int z, const glm::vec3& camera_pos) const {
    if (!m_frustum_culling_enabled) {
        return true;
    }

    glm::vec3 tile_world_pos = GetTileWorldPosition(x, y, z);

    // Distance culling
    float distance = GetDistanceToCamera(tile_world_pos);
    if (distance > m_culling_distance) {
        return false;
    }

    // Frustum culling
    if (!IsTileInFrustum(tile_world_pos)) {
        return false;
    }

    return true;
}

bool Tilemap3D::IsTileInFrustum(const glm::vec3& tile_world_pos) const {
    // TODO: Implement proper frustum culling
    // This would check if the tile is within the camera's view frustum
    (void)tile_world_pos;
    return true;
}

float Tilemap3D::GetDistanceToCamera(const glm::vec3& tile_world_pos) const {
    // TODO: Get actual camera position from renderer
    // For now, assume camera is at origin
    glm::vec3 camera_pos(0.0f);
    return glm::distance(tile_world_pos, camera_pos);
}

} // namespace Lupine
