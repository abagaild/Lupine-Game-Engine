#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace Lupine {

using json = nlohmann::json;

/**
 * @brief Tile-specific metadata and properties
 */
struct TileMetadata {
    std::map<std::string, std::string> tags;           // Custom tags for the tile
    std::map<std::string, float> properties;           // Numeric properties
    std::map<std::string, std::string> data;           // String data
    bool collision_enabled = false;                    // Collision override
    float opacity = 1.0f;                             // Tile-specific opacity
    glm::vec4 tint = glm::vec4(1.0f);                 // Tile-specific tint
    
    TileMetadata() = default;
    
    // Serialization
    json ToJSON() const;
    bool FromJSON(const json& j);
};

/**
 * @brief Individual tile instance with metadata
 */
struct TileInstance {
    int tile_id = -1;                                  // Tile ID from tileset (-1 = empty)
    int tileset_id = 0;                               // Which tileset this tile comes from
    TileMetadata metadata;                             // Tile-specific metadata
    
    TileInstance() = default;
    TileInstance(int id, int tileset = 0) : tile_id(id), tileset_id(tileset) {}
    
    bool IsEmpty() const { return tile_id < 0; }
    
    // Serialization
    json ToJSON() const;
    bool FromJSON(const json& j);
};

/**
 * @brief Tilemap layer with transparency and visibility
 */
class TilemapLayer {
public:
    TilemapLayer(const std::string& name, const glm::ivec2& size);
    ~TilemapLayer() = default;
    
    // Basic properties
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    
    bool IsLocked() const { return m_locked; }
    void SetLocked(bool locked) { m_locked = locked; }
    
    float GetOpacity() const { return m_opacity; }
    void SetOpacity(float opacity) { m_opacity = glm::clamp(opacity, 0.0f, 1.0f); }
    
    // Size management
    const glm::ivec2& GetSize() const { return m_size; }
    void Resize(const glm::ivec2& new_size);
    
    // Tile operations
    const TileInstance& GetTile(int x, int y) const;
    void SetTile(int x, int y, const TileInstance& tile);
    void ClearTile(int x, int y);
    
    // Layer operations
    void Clear();
    void Fill(const TileInstance& tile);
    void FloodFill(int x, int y, const TileInstance& tile);
    
    // Serialization
    json ToJSON() const;
    bool FromJSON(const json& j);

private:
    std::string m_name;
    bool m_visible;
    bool m_locked;
    float m_opacity;
    glm::ivec2 m_size;
    std::vector<TileInstance> m_tiles;
    
    int GetIndex(int x, int y) const { return y * m_size.x + x; }
    bool IsValidPosition(int x, int y) const;
};

/**
 * @brief Tileset reference information
 */
struct TilesetReference {
    int id;                                           // Unique ID within this tilemap
    std::string name;                                 // Display name
    std::string path;                                 // Path to .tileset file
    glm::ivec2 tile_size = glm::ivec2(32, 32);       // Tile size in pixels
    glm::ivec2 grid_size = glm::ivec2(1, 1);         // Grid size
    int spacing = 0;                                  // Spacing between tiles
    int margin = 0;                                   // Margin around tileset
    
    TilesetReference() = default;
    TilesetReference(int ref_id, const std::string& ref_name, const std::string& ref_path)
        : id(ref_id), name(ref_name), path(ref_path) {}
    
    // Serialization
    json ToJSON() const;
    bool FromJSON(const json& j);
};

/**
 * @brief Complete tilemap project data with layers and multiple tilesets
 */
class TilemapProject {
public:
    TilemapProject();
    ~TilemapProject() = default;
    
    // Project properties
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    
    const glm::ivec2& GetSize() const { return m_size; }
    void SetSize(const glm::ivec2& size);
    
    const glm::ivec2& GetTileSize() const { return m_tile_size; }
    void SetTileSize(const glm::ivec2& size) { m_tile_size = size; }
    
    const glm::vec4& GetBackgroundColor() const { return m_background_color; }
    void SetBackgroundColor(const glm::vec4& color) { m_background_color = color; }
    
    // Tileset management
    int AddTileset(const TilesetReference& tileset);
    void RemoveTileset(int tileset_id);
    const TilesetReference* GetTileset(int tileset_id) const;
    TilesetReference* GetTileset(int tileset_id);
    const std::vector<TilesetReference>& GetTilesets() const { return m_tilesets; }
    
    // Layer management
    int AddLayer(const std::string& name);
    void RemoveLayer(int layer_index);
    void MoveLayer(int from_index, int to_index);
    TilemapLayer* GetLayer(int layer_index);
    const TilemapLayer* GetLayer(int layer_index) const;
    TilemapLayer* GetLayerByName(const std::string& name);
    const std::vector<std::unique_ptr<TilemapLayer>>& GetLayers() const { return m_layers; }
    int GetLayerCount() const { return static_cast<int>(m_layers.size()); }
    
    // Active layer
    int GetActiveLayerIndex() const { return m_active_layer; }
    void SetActiveLayerIndex(int index);
    TilemapLayer* GetActiveLayer();
    
    // Utility functions
    void Clear();
    bool IsEmpty() const;
    
    // File operations
    bool SaveToFile(const std::string& filepath) const;
    bool LoadFromFile(const std::string& filepath);
    
    // Serialization
    std::string ToJSON() const;
    bool FromJSON(const std::string& json_data);

private:
    std::string m_name;
    glm::ivec2 m_size;
    glm::ivec2 m_tile_size;
    glm::vec4 m_background_color;
    
    std::vector<TilesetReference> m_tilesets;
    std::vector<std::unique_ptr<TilemapLayer>> m_layers;
    int m_active_layer;
    int m_next_tileset_id;
    
    void ResizeAllLayers(const glm::ivec2& new_size);
};

} // namespace Lupine
