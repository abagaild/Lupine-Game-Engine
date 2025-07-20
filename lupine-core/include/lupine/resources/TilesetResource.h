#pragma once

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace Lupine {

using json = nlohmann::json;

/**
 * @brief Types of collision shapes for tiles
 */
enum class TileCollisionType {
    None,           // No collision
    Rectangle,      // Full tile rectangle
    Circle,         // Circle collision
    Polygon,        // Custom polygon
    Convex          // Convex hull
};

/**
 * @brief Custom data types for tiles
 */
enum class TileDataType {
    String,
    Integer,
    Float,
    Boolean,
    Color
};

/**
 * @brief Custom data value for tiles
 */
struct TileDataValue {
    TileDataType type;
    std::string string_value;
    int int_value;
    float float_value;
    bool bool_value;
    glm::vec4 color_value;

    TileDataValue() : type(TileDataType::String), string_value(""), int_value(0), float_value(0.0f), bool_value(false), color_value(1.0f) {}
};

/**
 * @brief Collision shape data for a tile
 */
struct TileCollisionShape {
    TileCollisionType type = TileCollisionType::None;
    glm::vec2 offset = glm::vec2(0.0f);     // Offset from tile center
    glm::vec2 size = glm::vec2(1.0f);       // Size for rectangle/circle
    std::vector<glm::vec2> points;          // Points for polygon/convex
    
    TileCollisionShape() = default;
};

/**
 * @brief Individual tile data
 */
struct TileData {
    int id = -1;                                        // Tile ID
    glm::ivec2 grid_position = glm::ivec2(0);          // Position in tileset grid
    glm::vec4 texture_region = glm::vec4(0.0f);       // UV coordinates (x, y, width, height)
    TileCollisionShape collision;                       // Collision shape
    std::map<std::string, TileDataValue> custom_data;  // Custom properties
    
    TileData() = default;
    TileData(int tile_id, const glm::ivec2& pos) : id(tile_id), grid_position(pos) {}
};

/**
 * @brief Tileset2D resource (.tileset files)
 * 
 * This resource manages a collection of tiles from a source image,
 * including collision data and custom properties for each tile.
 */
class Tileset2DResource {
public:
    Tileset2DResource() = default;
    ~Tileset2DResource() = default;
    
    // Basic properties
    void SetTexturePath(const std::string& path) { m_texture_path = path; }
    const std::string& GetTexturePath() const { return m_texture_path; }
    
    void SetTileSize(const glm::ivec2& size) { m_tile_size = size; }
    const glm::ivec2& GetTileSize() const { return m_tile_size; }
    
    void SetGridSize(const glm::ivec2& size) { m_grid_size = size; }
    const glm::ivec2& GetGridSize() const { return m_grid_size; }
    
    void SetSpacing(int spacing) { m_spacing = spacing; }
    int GetSpacing() const { return m_spacing; }
    
    void SetMargin(int margin) { m_margin = margin; }
    int GetMargin() const { return m_margin; }
    
    // Tile management
    void AddTile(const TileData& tile);
    void RemoveTile(int tile_id);
    TileData* GetTile(int tile_id);
    const TileData* GetTile(int tile_id) const;
    TileData* GetTileAt(const glm::ivec2& grid_pos);
    const TileData* GetTileAt(const glm::ivec2& grid_pos) const;
    
    // Get all tiles
    const std::map<int, TileData>& GetTiles() const { return m_tiles; }
    std::vector<int> GetTileIds() const;
    
    // Utility functions
    int GetTileCount() const { return static_cast<int>(m_tiles.size()); }
    glm::vec4 CalculateTextureRegion(const glm::ivec2& grid_pos) const;
    glm::vec4 CalculateNormalizedTextureRegion(const glm::ivec2& grid_pos, const glm::ivec2& texture_size) const;
    glm::ivec2 GetGridPositionFromTileId(int tile_id) const;
    int GetTileIdFromGridPosition(const glm::ivec2& grid_pos) const;
    
    // Auto-generate tiles from grid
    void GenerateTilesFromGrid();
    void ClearTiles();
    
    // Serialization
    bool SaveToFile(const std::string& filepath) const;
    bool LoadFromFile(const std::string& filepath);
    
    // JSON serialization
    std::string ToJSON() const;
    bool FromJSON(const std::string& json);
    
private:
    std::string m_texture_path;                 // Path to source texture
    glm::ivec2 m_tile_size = glm::ivec2(32, 32); // Size of each tile in pixels
    glm::ivec2 m_grid_size = glm::ivec2(1, 1);   // Number of tiles in grid (columns, rows)
    int m_spacing = 0;                          // Spacing between tiles in pixels
    int m_margin = 0;                           // Margin around tileset in pixels
    std::map<int, TileData> m_tiles;            // Tile data indexed by tile ID
    
    // Helper functions for JSON serialization
    json SerializeTileData(const TileData& tile) const;
    bool DeserializeTileData(const json& j, TileData& tile) const;
    json SerializeCollisionShape(const TileCollisionShape& shape) const;
    bool DeserializeCollisionShape(const json& j, TileCollisionShape& shape) const;
    json SerializeTileDataValue(const TileDataValue& value) const;
    bool DeserializeTileDataValue(const json& j, TileDataValue& value) const;
};

} // namespace Lupine
