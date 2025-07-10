#include "lupine/components/Tilemap25D.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/resources/ResourceManager.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace Lupine {

using json = nlohmann::json;

Tilemap25D::Tilemap25D()
    : Component("Tilemap2.5D")
    , m_tileset_path("")
    , m_tileset_resource(nullptr)
    , m_tileset_loaded(false)
    , m_tilemap_data(glm::ivec2(10, 10))
    , m_tilemap_data_serialized("")
    , m_tile_size(1.0f, 1.0f)
    , m_modulate(1.0f, 1.0f, 1.0f, 1.0f)
    , m_billboard_mode(BillboardMode::Disabled)
    , m_transparent(false)
    , m_double_sided(false)
    , m_receives_lighting(true)
    , m_show_grid(false)
    , m_grid_color(1.0f, 1.0f, 1.0f, 0.5f)
    , m_collision_enabled(false)
    , m_meshes_dirty(true)
    , m_visible_tile_bounds(0, 0, 0, 0)
    , m_bounds_dirty(true)
{
    InitializeExportVariables();
}

Tilemap25D::~Tilemap25D() {
    CleanupMeshes();
}

void Tilemap25D::SetTilesetPath(const std::string& path) {
    m_tileset_path = path;
    SetExportVariable("tileset_path", path);
    m_tileset_loaded = false;
    LoadTileset();
}

void Tilemap25D::SetTilemapData(const TilemapData& data) {
    m_tilemap_data = data;
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_meshes_dirty = true;
    m_bounds_dirty = true;
}

void Tilemap25D::SetMapSize(const glm::ivec2& size) {
    m_tilemap_data.Resize(size);
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("map_size", glm::vec2(size.x, size.y));
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_meshes_dirty = true;
    m_bounds_dirty = true;
}

void Tilemap25D::SetTileSize(const glm::vec2& size) {
    m_tile_size = size;
    SetExportVariable("tile_size", size);
    m_meshes_dirty = true;
    m_bounds_dirty = true;
}

void Tilemap25D::SetTile(int x, int y, int tile_id) {
    m_tilemap_data.SetTile(x, y, tile_id);
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_meshes_dirty = true;
    m_bounds_dirty = true;
}

void Tilemap25D::SetModulate(const glm::vec4& modulate) {
    m_modulate = modulate;
    SetExportVariable("modulate", modulate);
}

void Tilemap25D::SetBillboardMode(BillboardMode mode) {
    m_billboard_mode = mode;
    SetExportVariable("billboard_mode", static_cast<int>(mode));
}

void Tilemap25D::SetTransparent(bool transparent) {
    m_transparent = transparent;
    SetExportVariable("transparent", transparent);
}

void Tilemap25D::SetDoubleSided(bool double_sided) {
    m_double_sided = double_sided;
    SetExportVariable("double_sided", double_sided);
}

void Tilemap25D::SetReceivesLighting(bool receives_lighting) {
    m_receives_lighting = receives_lighting;
    SetExportVariable("receives_lighting", receives_lighting);
}

void Tilemap25D::SetShowGrid(bool show_grid) {
    m_show_grid = show_grid;
    SetExportVariable("show_grid", show_grid);
}

void Tilemap25D::SetGridColor(const glm::vec4& color) {
    m_grid_color = color;
    SetExportVariable("grid_color", color);
}

void Tilemap25D::SetCollisionEnabled(bool enabled) {
    m_collision_enabled = enabled;
    SetExportVariable("collision_enabled", enabled);
}

glm::vec3 Tilemap25D::MapToLocal(const glm::ivec2& map_pos) const {
    return glm::vec3(map_pos.x * m_tile_size.x, 0.0f, map_pos.y * m_tile_size.y);
}

glm::ivec2 Tilemap25D::LocalToMap(const glm::vec3& local_pos) const {
    return glm::ivec2(
        static_cast<int>(local_pos.x / m_tile_size.x),
        static_cast<int>(local_pos.z / m_tile_size.y)
    );
}

glm::vec3 Tilemap25D::GetTileWorldPosition(int x, int y) const {
    Node3D* node3d = dynamic_cast<Node3D*>(GetOwner());
    if (!node3d) return glm::vec3(0.0f);
    
    glm::vec3 local_pos = MapToLocal(glm::ivec2(x, y));
    glm::vec4 world_pos = node3d->GetGlobalTransform() * glm::vec4(local_pos, 1.0f);
    return glm::vec3(world_pos);
}

void Tilemap25D::ClearTilemap() {
    m_tilemap_data.Clear();
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_meshes_dirty = true;
    m_bounds_dirty = true;
}

void Tilemap25D::FillTilemap(int tile_id) {
    for (int y = 0; y < m_tilemap_data.size.y; ++y) {
        for (int x = 0; x < m_tilemap_data.size.x; ++x) {
            m_tilemap_data.SetTile(x, y, tile_id);
        }
    }
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_meshes_dirty = true;
    m_bounds_dirty = true;
}

void Tilemap25D::FloodFill(int x, int y, int tile_id) {
    if (x < 0 || x >= m_tilemap_data.size.x || y < 0 || y >= m_tilemap_data.size.y) {
        return;
    }
    
    int original_tile = m_tilemap_data.GetTile(x, y);
    if (original_tile == tile_id) return;
    
    // Simple flood fill implementation
    std::vector<glm::ivec2> stack;
    stack.push_back(glm::ivec2(x, y));
    
    while (!stack.empty()) {
        glm::ivec2 pos = stack.back();
        stack.pop_back();
        
        if (pos.x < 0 || pos.x >= m_tilemap_data.size.x || 
            pos.y < 0 || pos.y >= m_tilemap_data.size.y) {
            continue;
        }
        
        if (m_tilemap_data.GetTile(pos.x, pos.y) != original_tile) {
            continue;
        }
        
        m_tilemap_data.SetTile(pos.x, pos.y, tile_id);
        
        // Add neighbors
        stack.push_back(glm::ivec2(pos.x + 1, pos.y));
        stack.push_back(glm::ivec2(pos.x - 1, pos.y));
        stack.push_back(glm::ivec2(pos.x, pos.y + 1));
        stack.push_back(glm::ivec2(pos.x, pos.y - 1));
    }
    
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_meshes_dirty = true;
    m_bounds_dirty = true;
}

std::string Tilemap25D::SerializeTilemapData() const {
    json j;
    j["size"] = {m_tilemap_data.size.x, m_tilemap_data.size.y};
    j["tiles"] = m_tilemap_data.tiles;
    return j.dump();
}

bool Tilemap25D::DeserializeTilemapData(const std::string& data) {
    try {
        json j = json::parse(data);
        
        glm::ivec2 size(j["size"][0], j["size"][1]);
        std::vector<int> tiles = j["tiles"].get<std::vector<int>>();
        
        m_tilemap_data.size = size;
        m_tilemap_data.tiles = tiles;
        
        // Ensure tiles vector has correct size
        if (m_tilemap_data.tiles.size() != static_cast<size_t>(size.x * size.y)) {
            m_tilemap_data.tiles.resize(size.x * size.y, -1);
        }
        
        m_meshes_dirty = true;
        m_bounds_dirty = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing tilemap data: " << e.what() << std::endl;
        return false;
    }
}

void Tilemap25D::OnReady() {
    LoadTileset();
    InitializeTileMeshes();
}

void Tilemap25D::OnUpdate(float delta_time) {
    (void)delta_time;
    
    // Check if export variables have changed
    UpdateFromExportVariables();
    
    // Check rendering context - Tilemap2.5D should only render in 3D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor2D) {
        return; // Don't render 2.5D tilemaps in 2D editor view
    }
    
    // Update meshes if needed
    if (m_meshes_dirty) {
        InitializeTileMeshes();
        m_meshes_dirty = false;
    }
    
    // Update visible tile bounds if needed
    if (m_bounds_dirty) {
        UpdateVisibleTiles();
        m_bounds_dirty = false;
    }
    
    // Render the tilemap
    RenderTilemap();
    
    // Render grid if enabled
    if (m_show_grid) {
        RenderGrid();
    }
}

void Tilemap25D::InitializeExportVariables() {
    AddExportVariable("tileset_path", m_tileset_path, "Path to tileset resource file", ExportVariableType::FilePath);
    AddExportVariable("map_size", glm::vec2(m_tilemap_data.size.x, m_tilemap_data.size.y), "Map size in tiles", ExportVariableType::Vec2);
    AddExportVariable("tile_size", m_tile_size, "Tile size in world units", ExportVariableType::Vec2);
    AddExportVariable("tilemap_data", m_tilemap_data_serialized, "Serialized tilemap data", ExportVariableType::String);
    AddExportVariable("modulate", m_modulate, "Color modulation (RGBA)", ExportVariableType::Color);
    AddExportVariable("billboard_mode", static_cast<int>(m_billboard_mode), "Billboard mode (0=Disabled, 1=Enabled, 2=Y-Billboard, 3=Particles)", ExportVariableType::Int);
    AddExportVariable("transparent", m_transparent, "Enable transparency", ExportVariableType::Bool);
    AddExportVariable("double_sided", m_double_sided, "Render both sides", ExportVariableType::Bool);
    AddExportVariable("receives_lighting", m_receives_lighting, "Receive lighting", ExportVariableType::Bool);
    AddExportVariable("show_grid", m_show_grid, "Show tile grid", ExportVariableType::Bool);
    AddExportVariable("grid_color", m_grid_color, "Grid line color (RGBA)", ExportVariableType::Color);
    AddExportVariable("collision_enabled", m_collision_enabled, "Enable collision detection", ExportVariableType::Bool);
}

void Tilemap25D::LoadTileset() {
    if (m_tileset_path.empty()) {
        m_tileset_loaded = false;
        m_tileset_resource.reset();
        return;
    }

    try {
        m_tileset_resource = std::make_unique<Tileset2DResource>();
        if (m_tileset_resource->LoadFromFile(m_tileset_path)) {
            m_tileset_loaded = true;
            std::cout << "Loaded tileset for Tilemap2.5D: " << m_tileset_path << std::endl;
        } else {
            std::cerr << "Failed to load tileset for Tilemap2.5D: " << m_tileset_path << std::endl;
            m_tileset_loaded = false;
            m_tileset_resource.reset();
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception loading tileset " << m_tileset_path << ": " << e.what() << std::endl;
        m_tileset_loaded = false;
        m_tileset_resource.reset();
    }
}

void Tilemap25D::UpdateFromExportVariables() {
    // Check for changes in export variables
    std::string new_tileset_path = GetExportVariableValue<std::string>("tileset_path", "");
    if (new_tileset_path != m_tileset_path) {
        SetTilesetPath(new_tileset_path);
    }

    glm::vec2 new_map_size = GetExportVariableValue<glm::vec2>("map_size", glm::vec2(10, 10));
    glm::ivec2 new_map_size_int(static_cast<int>(new_map_size.x), static_cast<int>(new_map_size.y));
    if (new_map_size_int != m_tilemap_data.size) {
        SetMapSize(new_map_size_int);
    }

    glm::vec2 new_tile_size = GetExportVariableValue<glm::vec2>("tile_size", glm::vec2(1.0f, 1.0f));
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

    int billboard_mode_int = GetExportVariableValue<int>("billboard_mode", 0);
    m_billboard_mode = static_cast<BillboardMode>(billboard_mode_int);

    m_transparent = GetExportVariableValue<bool>("transparent", false);
    m_double_sided = GetExportVariableValue<bool>("double_sided", false);
    m_receives_lighting = GetExportVariableValue<bool>("receives_lighting", true);
    m_show_grid = GetExportVariableValue<bool>("show_grid", false);
    m_grid_color = GetExportVariableValue<glm::vec4>("grid_color", glm::vec4(1.0f, 1.0f, 1.0f, 0.5f));
    m_collision_enabled = GetExportVariableValue<bool>("collision_enabled", false);
}

void Tilemap25D::UpdateExportVariables() {
    SetExportVariable("tileset_path", m_tileset_path);
    SetExportVariable("map_size", glm::vec2(m_tilemap_data.size.x, m_tilemap_data.size.y));
    SetExportVariable("tile_size", m_tile_size);
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    SetExportVariable("modulate", m_modulate);
    SetExportVariable("billboard_mode", static_cast<int>(m_billboard_mode));
    SetExportVariable("transparent", m_transparent);
    SetExportVariable("double_sided", m_double_sided);
    SetExportVariable("receives_lighting", m_receives_lighting);
    SetExportVariable("show_grid", m_show_grid);
    SetExportVariable("grid_color", m_grid_color);
    SetExportVariable("collision_enabled", m_collision_enabled);
}

void Tilemap25D::RenderTilemap() {
    if (!m_tileset_loaded || !m_tileset_resource) {
        return;
    }

    Node3D* node3d = dynamic_cast<Node3D*>(GetOwner());
    if (!node3d) {
        return;
    }

    // Render visible tiles
    for (int y = m_visible_tile_bounds.y; y <= m_visible_tile_bounds.w; ++y) {
        for (int x = m_visible_tile_bounds.x; x <= m_visible_tile_bounds.z; ++x) {
            int tile_id = m_tilemap_data.GetTile(x, y);
            if (tile_id >= 0) {
                RenderTile(x, y, tile_id, node3d);
            }
        }
    }
}

void Tilemap25D::RenderGrid() {
    // TODO: Implement 3D grid rendering
    // This would render grid lines in 3D space for editing purposes
}

void Tilemap25D::RenderTile(int x, int y, int tile_id, Node3D* node3d) {
    if (!m_tileset_loaded || !m_tileset_resource) {
        return;
    }

    const TileData* tile = m_tileset_resource->GetTile(tile_id);
    if (!tile) {
        return;
    }

    // Calculate tile transform
    glm::mat4 tile_transform = CalculateTileTransform(x, y, node3d);

    // Apply billboard transformation if needed
    if (m_billboard_mode != BillboardMode::Disabled) {
        tile_transform = CalculateBillboardTransform(tile_transform);
    }

    // Get texture region for this tile
    glm::vec4 texture_region = GetTileTextureRegion(tile_id);

    // Load tileset texture if needed
    unsigned int texture_id = 0;
    if (!m_tileset_resource->GetTexturePath().empty()) {
        Texture texture = ResourceManager::LoadTexture(m_tileset_resource->GetTexturePath());
        if (texture.IsValid()) {
            texture_id = texture.id;
        }
    }

    // Use white texture if no tileset texture
    if (texture_id == 0) {
        texture_id = Renderer::GetWhiteTexture();
    }

    // Create a simple quad mesh for rendering
    // TODO: Use proper mesh rendering with VAO/VBO
    Renderer::RenderQuad(tile_transform, m_modulate, texture_id, texture_region, false, false);
}

void Tilemap25D::InitializeTileMeshes() {
    CleanupMeshes();

    // Initialize meshes for all tiles
    int total_tiles = m_tilemap_data.size.x * m_tilemap_data.size.y;
    m_tile_meshes.resize(total_tiles);

    for (int i = 0; i < total_tiles; ++i) {
        UpdateTileMesh(i);
    }
}

void Tilemap25D::UpdateTileMesh(int tile_index) {
    if (tile_index < 0 || tile_index >= static_cast<int>(m_tile_meshes.size())) {
        return;
    }

    TileMesh& mesh = m_tile_meshes[tile_index];

    // For now, we'll use simple quad rendering
    // In a full implementation, you'd create proper VAO/VBO here
    mesh.initialized = true;
}

void Tilemap25D::CleanupMeshes() {
    for (TileMesh& mesh : m_tile_meshes) {
        if (mesh.initialized) {
            // TODO: Use proper renderer cleanup methods instead of direct OpenGL calls
            // For now, just mark as uninitialized
            mesh.vertex_array_object = 0;
            mesh.vertex_buffer_object = 0;
            mesh.element_buffer_object = 0;
            mesh.initialized = false;
        }
    }
    m_tile_meshes.clear();
}

glm::mat4 Tilemap25D::CalculateTileTransform(int x, int y, Node3D* node3d) const {
    // Get base transform from node
    glm::mat4 base_transform = node3d->GetGlobalTransform();

    // Calculate tile position in local space
    glm::vec3 tile_pos = MapToLocal(glm::ivec2(x, y));

    // Create tile transform
    glm::mat4 tile_transform = glm::translate(base_transform, tile_pos);
    tile_transform = glm::scale(tile_transform, glm::vec3(m_tile_size.x, m_tile_size.y, 1.0f));

    return tile_transform;
}

glm::mat4 Tilemap25D::CalculateBillboardTransform(const glm::mat4& tile_transform) const {
    // Extract position from tile transform
    glm::vec3 position = glm::vec3(tile_transform[3]);

    // Create billboard transform based on mode
    glm::mat4 billboard_transform = glm::mat4(1.0f);

    switch (m_billboard_mode) {
        case BillboardMode::Enabled: {
            // Full billboard - always face camera
            // This is a simplified implementation
            billboard_transform = glm::translate(billboard_transform, position);
            break;
        }
        case BillboardMode::YBillboard: {
            // Y-axis billboard - only rotate around Y axis
            billboard_transform = glm::translate(billboard_transform, position);
            // Apply only Y rotation to face camera
            break;
        }
        case BillboardMode::ParticlesBillboard: {
            // Special billboard mode for particles
            billboard_transform = glm::translate(billboard_transform, position);
            break;
        }
        default:
            return tile_transform;
    }

    return billboard_transform;
}

glm::vec4 Tilemap25D::GetTileTextureRegion(int tile_id) const {
    if (!m_tileset_loaded || !m_tileset_resource) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    const TileData* tile = m_tileset_resource->GetTile(tile_id);
    if (!tile) {
        // Calculate texture region from tile ID if tile data doesn't exist
        glm::ivec2 grid_pos = m_tileset_resource->GetGridPositionFromTileId(tile_id);
        glm::vec4 pixel_region = m_tileset_resource->CalculateTextureRegion(grid_pos);

        // Normalize to UV coordinates
        Texture texture = ResourceManager::LoadTexture(m_tileset_resource->GetTexturePath());
        if (texture.IsValid()) {
            float u1 = pixel_region.x / texture.width;
            float v1 = pixel_region.y / texture.height;
            float u2 = (pixel_region.x + pixel_region.z) / texture.width;
            float v2 = (pixel_region.y + pixel_region.w) / texture.height;
            return glm::vec4(u1, v1, u2 - u1, v2 - v1);
        }
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    // If tile data exists but texture_region is not normalized, normalize it
    if (tile->texture_region.x > 1.0f || tile->texture_region.y > 1.0f ||
        tile->texture_region.z > 1.0f || tile->texture_region.w > 1.0f) {
        // Texture region is in pixel coordinates, normalize it
        Texture texture = ResourceManager::LoadTexture(m_tileset_resource->GetTexturePath());
        if (texture.IsValid()) {
            float u1 = tile->texture_region.x / texture.width;
            float v1 = tile->texture_region.y / texture.height;
            float u2 = (tile->texture_region.x + tile->texture_region.z) / texture.width;
            float v2 = (tile->texture_region.y + tile->texture_region.w) / texture.height;
            return glm::vec4(u1, v1, u2 - u1, v2 - v1);
        }
    }

    return tile->texture_region;
}

void Tilemap25D::UpdateVisibleTiles() {
    // For now, render all tiles
    // TODO: Implement proper frustum culling based on camera view
    m_visible_tile_bounds = glm::ivec4(
        0, 0,
        m_tilemap_data.size.x - 1,
        m_tilemap_data.size.y - 1
    );
}

} // namespace Lupine
