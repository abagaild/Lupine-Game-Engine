#include "lupine/components/Tilemap2D.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/resources/ResourceManager.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace Lupine {

using json = nlohmann::json;

Tilemap2D::Tilemap2D()
    : Component("Tilemap2D")
    , m_tileset_path("")
    , m_tileset_resource(nullptr)
    , m_tileset_loaded(false)
    , m_tilemap_data(glm::ivec2(10, 10))
    , m_tilemap_data_serialized("")
    , m_tile_size(32, 32)
    , m_modulate(1.0f, 1.0f, 1.0f, 1.0f)
    , m_show_grid(false)
    , m_grid_color(1.0f, 1.0f, 1.0f, 0.5f)
    , m_collision_enabled(false)
    , m_visible_tile_bounds(0, 0, 0, 0)
    , m_bounds_dirty(true)
{
    InitializeExportVariables();
}

void Tilemap2D::SetTilesetPath(const std::string& path) {
    m_tileset_path = path;
    SetExportVariable("tileset_path", path);
    m_tileset_loaded = false;
    LoadTileset();
}

void Tilemap2D::SetTilemapData(const TilemapData& data) {
    m_tilemap_data = data;
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_bounds_dirty = true;
}

void Tilemap2D::SetMapSize(const glm::ivec2& size) {
    m_tilemap_data.Resize(size);
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("map_size", glm::vec2(size.x, size.y));
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_bounds_dirty = true;
}

void Tilemap2D::SetTileSize(const glm::ivec2& size) {
    m_tile_size = size;
    SetExportVariable("tile_size", glm::vec2(size.x, size.y));
    m_bounds_dirty = true;
}

void Tilemap2D::SetTile(int x, int y, int tile_id) {
    m_tilemap_data.SetTile(x, y, tile_id);
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_bounds_dirty = true;
}

void Tilemap2D::SetModulate(const glm::vec4& modulate) {
    m_modulate = modulate;
    SetExportVariable("modulate", modulate);
}

void Tilemap2D::SetShowGrid(bool show_grid) {
    m_show_grid = show_grid;
    SetExportVariable("show_grid", show_grid);
}

void Tilemap2D::SetGridColor(const glm::vec4& color) {
    m_grid_color = color;
    SetExportVariable("grid_color", color);
}

void Tilemap2D::SetCollisionEnabled(bool enabled) {
    m_collision_enabled = enabled;
    SetExportVariable("collision_enabled", enabled);
}

glm::vec2 Tilemap2D::MapToLocal(const glm::ivec2& map_pos) const {
    return glm::vec2(map_pos.x * m_tile_size.x, map_pos.y * m_tile_size.y);
}

glm::ivec2 Tilemap2D::LocalToMap(const glm::vec2& local_pos) const {
    return glm::ivec2(
        static_cast<int>(local_pos.x / m_tile_size.x),
        static_cast<int>(local_pos.y / m_tile_size.y)
    );
}

glm::vec2 Tilemap2D::GetTileWorldPosition(int x, int y) const {
    Node2D* node2d = dynamic_cast<Node2D*>(GetOwner());
    if (!node2d) return glm::vec2(0.0f);
    
    glm::vec2 local_pos = MapToLocal(glm::ivec2(x, y));
    glm::vec2 world_pos = node2d->GetGlobalPosition() + local_pos;
    return world_pos;
}

void Tilemap2D::ClearTilemap() {
    m_tilemap_data.Clear();
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_bounds_dirty = true;
}

void Tilemap2D::FillTilemap(int tile_id) {
    for (int y = 0; y < m_tilemap_data.size.y; ++y) {
        for (int x = 0; x < m_tilemap_data.size.x; ++x) {
            m_tilemap_data.SetTile(x, y, tile_id);
        }
    }
    m_tilemap_data_serialized = SerializeTilemapData();
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    m_bounds_dirty = true;
}

void Tilemap2D::FloodFill(int x, int y, int tile_id) {
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
    m_bounds_dirty = true;
}

std::string Tilemap2D::SerializeTilemapData() const {
    json j;
    j["size"] = {m_tilemap_data.size.x, m_tilemap_data.size.y};
    j["tiles"] = m_tilemap_data.tiles;
    return j.dump();
}

bool Tilemap2D::DeserializeTilemapData(const std::string& data) {
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
        
        m_bounds_dirty = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing tilemap data: " << e.what() << std::endl;
        return false;
    }
}

void Tilemap2D::OnReady() {
    LoadTileset();
}

void Tilemap2D::OnUpdate(float delta_time) {
    (void)delta_time;
    
    // Check if export variables have changed
    UpdateFromExportVariables();
    
    // Check rendering context - Tilemap2D should only render in 2D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor3D) {
        return; // Don't render 2D tilemaps in 3D editor view
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

void Tilemap2D::InitializeExportVariables() {
    AddExportVariable("tileset_path", m_tileset_path, "Path to tileset resource file", ExportVariableType::FilePath);
    AddExportVariable("map_size", glm::vec2(m_tilemap_data.size.x, m_tilemap_data.size.y), "Map size in tiles", ExportVariableType::Vec2);
    AddExportVariable("tile_size", glm::vec2(m_tile_size.x, m_tile_size.y), "Tile size in pixels", ExportVariableType::Vec2);
    AddExportVariable("tilemap_data", m_tilemap_data_serialized, "Serialized tilemap data", ExportVariableType::String);
    AddExportVariable("modulate", m_modulate, "Color modulation (RGBA)", ExportVariableType::Color);
    AddExportVariable("show_grid", m_show_grid, "Show tile grid", ExportVariableType::Bool);
    AddExportVariable("grid_color", m_grid_color, "Grid line color (RGBA)", ExportVariableType::Color);
    AddExportVariable("collision_enabled", m_collision_enabled, "Enable collision detection", ExportVariableType::Bool);
}

void Tilemap2D::LoadTileset() {
    if (m_tileset_path.empty()) {
        m_tileset_loaded = false;
        m_tileset_resource.reset();
        return;
    }
    
    try {
        m_tileset_resource = std::make_unique<Tileset2DResource>();
        if (m_tileset_resource->LoadFromFile(m_tileset_path)) {
            m_tileset_loaded = true;
            std::cout << "Loaded tileset: " << m_tileset_path << std::endl;
        } else {
            std::cerr << "Failed to load tileset: " << m_tileset_path << std::endl;
            m_tileset_loaded = false;
            m_tileset_resource.reset();
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception loading tileset " << m_tileset_path << ": " << e.what() << std::endl;
        m_tileset_loaded = false;
        m_tileset_resource.reset();
    }
}

void Tilemap2D::UpdateFromExportVariables() {
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

    glm::vec2 new_tile_size = GetExportVariableValue<glm::vec2>("tile_size", glm::vec2(32, 32));
    glm::ivec2 new_tile_size_int(static_cast<int>(new_tile_size.x), static_cast<int>(new_tile_size.y));
    if (new_tile_size_int != m_tile_size) {
        SetTileSize(new_tile_size_int);
    }

    std::string new_tilemap_data = GetExportVariableValue<std::string>("tilemap_data", "");
    if (new_tilemap_data != m_tilemap_data_serialized && !new_tilemap_data.empty()) {
        if (DeserializeTilemapData(new_tilemap_data)) {
            m_tilemap_data_serialized = new_tilemap_data;
        }
    }

    m_modulate = GetExportVariableValue<glm::vec4>("modulate", glm::vec4(1.0f));
    m_show_grid = GetExportVariableValue<bool>("show_grid", false);
    m_grid_color = GetExportVariableValue<glm::vec4>("grid_color", glm::vec4(1.0f, 1.0f, 1.0f, 0.5f));
    m_collision_enabled = GetExportVariableValue<bool>("collision_enabled", false);
}

void Tilemap2D::UpdateExportVariables() {
    SetExportVariable("tileset_path", m_tileset_path);
    SetExportVariable("map_size", glm::vec2(m_tilemap_data.size.x, m_tilemap_data.size.y));
    SetExportVariable("tile_size", glm::vec2(m_tile_size.x, m_tile_size.y));
    SetExportVariable("tilemap_data", m_tilemap_data_serialized);
    SetExportVariable("modulate", m_modulate);
    SetExportVariable("show_grid", m_show_grid);
    SetExportVariable("grid_color", m_grid_color);
    SetExportVariable("collision_enabled", m_collision_enabled);
}

void Tilemap2D::RenderTilemap() {
    if (!m_tileset_loaded || !m_tileset_resource) {
        return;
    }

    // Get base transform
    glm::mat4 base_transform = CalculateBaseTransform();

    // Render visible tiles
    for (int y = m_visible_tile_bounds.y; y <= m_visible_tile_bounds.w; ++y) {
        for (int x = m_visible_tile_bounds.x; x <= m_visible_tile_bounds.z; ++x) {
            int tile_id = m_tilemap_data.GetTile(x, y);
            if (tile_id >= 0) {
                RenderTile(x, y, tile_id, base_transform);
            }
        }
    }
}

void Tilemap2D::RenderGrid() {
    // TODO: Implement grid rendering
    // This would render grid lines over the tilemap for editing purposes
}

void Tilemap2D::RenderTile(int x, int y, int tile_id, const glm::mat4& base_transform) {
    if (!m_tileset_loaded || !m_tileset_resource) {
        return;
    }

    const TileData* tile = m_tileset_resource->GetTile(tile_id);
    if (!tile) {
        return;
    }

    // Calculate tile position
    glm::vec2 tile_pos = MapToLocal(glm::ivec2(x, y));

    // Create transform for this tile
    glm::mat4 tile_transform = glm::translate(base_transform, glm::vec3(tile_pos.x, tile_pos.y, 0.0f));
    tile_transform = glm::scale(tile_transform, glm::vec3(m_tile_size.x, m_tile_size.y, 1.0f));

    // Get texture region for this tile
    glm::vec4 texture_region = GetTileTextureRegion(tile_id);

    // Load tileset texture if needed
    std::shared_ptr<GraphicsTexture> texture_to_use = Renderer::GetWhiteTexture();
    if (!m_tileset_resource->GetTexturePath().empty()) {
        Texture texture = ResourceManager::LoadTexture(m_tileset_resource->GetTexturePath());
        if (texture.IsValid()) {
            // TODO: Convert texture.id to GraphicsTexture - for now use white texture
            texture_to_use = Renderer::GetWhiteTexture();
        }
    }

    // Render the tile
    Renderer::RenderQuad(tile_transform, m_modulate, texture_to_use, texture_region, false, false);
}

glm::mat4 Tilemap2D::CalculateBaseTransform() const {
    Node2D* node2d = dynamic_cast<Node2D*>(GetOwner());
    if (!node2d) {
        return glm::mat4(1.0f);
    }

    glm::mat4 transform = glm::mat4(1.0f);

    // Apply node transform
    glm::vec2 position = node2d->GetGlobalPosition();
    transform = glm::translate(transform, glm::vec3(position.x, position.y, 0.0f));

    float rotation = node2d->GetGlobalRotation();
    transform = glm::rotate(transform, rotation, glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec2 scale = node2d->GetGlobalScale();
    transform = glm::scale(transform, glm::vec3(scale.x, scale.y, 1.0f));

    return transform;
}

glm::vec4 Tilemap2D::GetTileTextureRegion(int tile_id) const {
    if (!m_tileset_loaded || !m_tileset_resource) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    const TileData* tile = m_tileset_resource->GetTile(tile_id);
    if (!tile) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    return tile->texture_region;
}

void Tilemap2D::UpdateVisibleTiles() {
    // For now, render all tiles
    // TODO: Implement proper frustum culling based on camera view
    m_visible_tile_bounds = glm::ivec4(
        0, 0,
        m_tilemap_data.size.x - 1,
        m_tilemap_data.size.y - 1
    );
}

} // namespace Lupine
