#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>
#include "lupine/resources/MeshLoader.h"

namespace Lupine {

using json = nlohmann::json;

/**
 * @brief Types of collision shapes for 3D tiles
 */
enum class Tile3DCollisionType {
    None,           // No collision
    Box,            // Axis-aligned bounding box
    Sphere,         // Sphere collision
    Mesh,           // Use mesh geometry for collision
    ConvexHull,     // Convex hull of mesh
    Custom          // Custom collision mesh file
};

/**
 * @brief Custom data types for 3D tiles
 */
enum class Tile3DDataType {
    String,
    Integer,
    Float,
    Boolean,
    Vector3,
    Color
};

/**
 * @brief Custom data value for 3D tiles
 */
struct Tile3DDataValue {
    Tile3DDataType type;
    std::string string_value;
    int int_value;
    float float_value;
    bool bool_value;
    glm::vec3 vec3_value;
    glm::vec4 color_value;

    Tile3DDataValue() : type(Tile3DDataType::String), string_value(""), int_value(0), 
                        float_value(0.0f), bool_value(false), vec3_value(0.0f), color_value(1.0f) {}
};

/**
 * @brief Collision shape data for a 3D tile
 */
struct Tile3DCollisionShape {
    Tile3DCollisionType type = Tile3DCollisionType::None;
    glm::vec3 offset = glm::vec3(0.0f);         // Offset from tile center
    glm::vec3 size = glm::vec3(1.0f);           // Size for box/sphere
    std::string collision_mesh_path;            // Path to custom collision mesh
    float margin = 0.0f;                        // Collision margin for physics
    
    Tile3DCollisionShape() = default;
};

/**
 * @brief Transform data for a 3D tile
 */
struct Tile3DTransform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    
    Tile3DTransform() = default;
};

/**
 * @brief Individual 3D tile data
 */
struct Tile3DData {
    int id = -1;                                        // Tile ID
    std::string name;                                   // Tile name
    std::string mesh_path;                              // Path to 3D model file
    std::string preview_image_path;                     // Path to preview image
    Tile3DTransform default_transform;                  // Default transform
    Tile3DCollisionShape collision;                     // Collision shape
    std::map<std::string, Tile3DDataValue> custom_data; // Custom properties
    
    // Runtime data (not serialized)
    std::unique_ptr<Model> model;                       // Loaded 3D model
    bool model_loaded = false;                          // Model load status
    
    Tile3DData() = default;
    Tile3DData(int tile_id, const std::string& tile_name) : id(tile_id), name(tile_name) {}
    
    // Copy constructor and assignment operator for proper model handling
    Tile3DData(const Tile3DData& other);
    Tile3DData& operator=(const Tile3DData& other);
    
    // Move constructor and assignment operator
    Tile3DData(Tile3DData&& other) noexcept = default;
    Tile3DData& operator=(Tile3DData&& other) noexcept = default;
};

/**
 * @brief Category for organizing 3D tiles
 */
struct Tile3DCategory {
    std::string name;
    std::string description;
    std::vector<int> tile_ids;
    
    Tile3DCategory() = default;
    Tile3DCategory(const std::string& cat_name) : name(cat_name) {}
};

/**
 * @brief Tileset3D resource (.tileset3d files)
 * 
 * This resource manages a collection of 3D tiles (models) for map building,
 * including collision data, custom properties, and organization categories.
 */
class Tileset3DResource {
public:
    Tileset3DResource() = default;
    ~Tileset3DResource() = default;
    
    // Basic properties
    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }
    
    void SetDescription(const std::string& description) { m_description = description; }
    const std::string& GetDescription() const { return m_description; }
    
    void SetVersion(const std::string& version) { m_version = version; }
    const std::string& GetVersion() const { return m_version; }
    
    // Tile management
    void AddTile(const Tile3DData& tile);
    void RemoveTile(int tile_id);
    Tile3DData* GetTile(int tile_id);
    const Tile3DData* GetTile(int tile_id) const;
    Tile3DData* GetTileByName(const std::string& name);
    const Tile3DData* GetTileByName(const std::string& name) const;
    
    // Get all tiles
    const std::map<int, Tile3DData>& GetTiles() const { return m_tiles; }
    std::vector<int> GetTileIds() const;
    std::vector<std::string> GetTileNames() const;
    
    // Category management
    void AddCategory(const Tile3DCategory& category);
    void RemoveCategory(const std::string& name);
    Tile3DCategory* GetCategory(const std::string& name);
    const Tile3DCategory* GetCategory(const std::string& name) const;
    const std::map<std::string, Tile3DCategory>& GetCategories() const { return m_categories; }
    
    void AddTileToCategory(int tile_id, const std::string& category_name);
    void RemoveTileFromCategory(int tile_id, const std::string& category_name);
    std::vector<std::string> GetTileCategories(int tile_id) const;
    
    // Utility functions
    int GetTileCount() const { return static_cast<int>(m_tiles.size()); }
    int GetNextTileId() const;
    bool HasTile(int tile_id) const;
    bool HasTileWithName(const std::string& name) const;
    
    // Model loading
    void LoadTileModel(int tile_id);
    void LoadAllTileModels();
    void UnloadTileModel(int tile_id);
    void UnloadAllTileModels();
    
    // Clear all data
    void Clear();
    
    // Serialization
    bool SaveToFile(const std::string& filepath) const;
    bool LoadFromFile(const std::string& filepath);
    
    // JSON serialization
    std::string ToJSON() const;
    bool FromJSON(const std::string& json);
    
private:
    std::string m_name;                                 // Tileset name
    std::string m_description;                          // Tileset description
    std::string m_version = "1.0";                      // Tileset version
    std::map<int, Tile3DData> m_tiles;                  // Tiles indexed by ID
    std::map<std::string, Tile3DCategory> m_categories; // Categories indexed by name
    
    // Helper functions for JSON serialization
    json SerializeTile3DData(const Tile3DData& tile) const;
    bool DeserializeTile3DData(const json& j, Tile3DData& tile) const;
    json SerializeTile3DCollisionShape(const Tile3DCollisionShape& shape) const;
    bool DeserializeTile3DCollisionShape(const json& j, Tile3DCollisionShape& shape) const;
    json SerializeTile3DTransform(const Tile3DTransform& transform) const;
    bool DeserializeTile3DTransform(const json& j, Tile3DTransform& transform) const;
    json SerializeTile3DDataValue(const Tile3DDataValue& value) const;
    bool DeserializeTile3DDataValue(const json& j, Tile3DDataValue& value) const;
    json SerializeTile3DCategory(const Tile3DCategory& category) const;
    bool DeserializeTile3DCategory(const json& j, Tile3DCategory& category) const;
};

} // namespace Lupine
