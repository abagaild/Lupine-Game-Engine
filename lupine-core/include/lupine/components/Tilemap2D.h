#pragma once

#include "lupine/core/Component.h"
#include "lupine/resources/TilesetResource.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace Lupine {

/**
 * @brief Tilemap data structure for storing tile arrangements
 */
struct TilemapData {
    glm::ivec2 size = glm::ivec2(10, 10);           // Map size in tiles
    std::vector<int> tiles;                         // Tile IDs (-1 = empty)
    
    TilemapData() {
        tiles.resize(size.x * size.y, -1);
    }
    
    TilemapData(const glm::ivec2& map_size) : size(map_size) {
        tiles.resize(size.x * size.y, -1);
    }
    
    // Get/Set tile at position
    int GetTile(int x, int y) const {
        if (x < 0 || x >= size.x || y < 0 || y >= size.y) return -1;
        return tiles[y * size.x + x];
    }
    
    void SetTile(int x, int y, int tile_id) {
        if (x < 0 || x >= size.x || y < 0 || y >= size.y) return;
        tiles[y * size.x + x] = tile_id;
    }
    
    // Resize the tilemap
    void Resize(const glm::ivec2& new_size) {
        std::vector<int> new_tiles(new_size.x * new_size.y, -1);
        
        // Copy existing tiles
        for (int y = 0; y < std::min(size.y, new_size.y); ++y) {
            for (int x = 0; x < std::min(size.x, new_size.x); ++x) {
                new_tiles[y * new_size.x + x] = GetTile(x, y);
            }
        }
        
        size = new_size;
        tiles = std::move(new_tiles);
    }
    
    // Clear all tiles
    void Clear() {
        std::fill(tiles.begin(), tiles.end(), -1);
    }
};

/**
 * @brief 2D tilemap component for rendering tile-based maps
 * 
 * Tilemap2D component renders a grid of tiles using a tileset resource.
 * It's based on Sprite2D and should be attached to Node2D nodes.
 * Uses the tilemap painter system for editing tile arrangements.
 */
class Tilemap2D : public Component {
public:
    /**
     * @brief Constructor
     */
    Tilemap2D();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Tilemap2D() = default;
    
    // Tileset management
    const std::string& GetTilesetPath() const { return m_tileset_path; }
    void SetTilesetPath(const std::string& path);
    
    // Tilemap data
    const TilemapData& GetTilemapData() const { return m_tilemap_data; }
    void SetTilemapData(const TilemapData& data);
    
    // Map size
    const glm::ivec2& GetMapSize() const { return m_tilemap_data.size; }
    void SetMapSize(const glm::ivec2& size);
    
    // Tile size (in pixels)
    const glm::ivec2& GetTileSize() const { return m_tile_size; }
    void SetTileSize(const glm::ivec2& size);
    
    // Individual tile operations
    int GetTile(int x, int y) const { return m_tilemap_data.GetTile(x, y); }
    void SetTile(int x, int y, int tile_id);
    
    // Rendering properties
    const glm::vec4& GetModulate() const { return m_modulate; }
    void SetModulate(const glm::vec4& modulate);
    
    bool GetShowGrid() const { return m_show_grid; }
    void SetShowGrid(bool show_grid);
    
    const glm::vec4& GetGridColor() const { return m_grid_color; }
    void SetGridColor(const glm::vec4& color);
    
    // Collision
    bool GetCollisionEnabled() const { return m_collision_enabled; }
    void SetCollisionEnabled(bool enabled);
    
    // Utility functions
    glm::vec2 MapToLocal(const glm::ivec2& map_pos) const;
    glm::ivec2 LocalToMap(const glm::vec2& local_pos) const;
    glm::vec2 GetTileWorldPosition(int x, int y) const;
    
    // Tilemap operations
    void ClearTilemap();
    void FillTilemap(int tile_id);
    void FloodFill(int x, int y, int tile_id);
    
    // Serialization helpers
    std::string SerializeTilemapData() const;
    bool DeserializeTilemapData(const std::string& data);
    
    // Component interface
    std::string GetTypeName() const override { return "Tilemap2D"; }
    std::string GetCategory() const override { return "2D"; }
    
    // Lifecycle methods
    void OnReady() override;
    void OnUpdate(float delta_time) override;

protected:
    void InitializeExportVariables() override;

private:
    // Tileset resource
    std::string m_tileset_path;
    std::unique_ptr<Tileset2DResource> m_tileset_resource;
    bool m_tileset_loaded;
    
    // Tilemap data
    TilemapData m_tilemap_data;
    std::string m_tilemap_data_serialized;  // For export variables
    
    // Tile properties
    glm::ivec2 m_tile_size;
    
    // Rendering properties
    glm::vec4 m_modulate;
    bool m_show_grid;
    glm::vec4 m_grid_color;
    
    // Collision
    bool m_collision_enabled;
    
    // Internal methods
    void LoadTileset();
    void UpdateFromExportVariables();
    void UpdateExportVariables();
    void RenderTilemap();
    void RenderGrid();
    void RenderTile(int x, int y, int tile_id, const glm::mat4& base_transform);
    glm::mat4 CalculateBaseTransform() const;
    glm::vec4 GetTileTextureRegion(int tile_id) const;
    
    // Optimization
    void UpdateVisibleTiles();
    glm::ivec4 m_visible_tile_bounds;  // min_x, min_y, max_x, max_y
    bool m_bounds_dirty;
};

} // namespace Lupine
