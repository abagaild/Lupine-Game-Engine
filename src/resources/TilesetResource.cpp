#include "lupine/resources/TilesetResource.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace Lupine {

void Tileset2DResource::AddTile(const TileData& tile) {
    m_tiles[tile.id] = tile;
}

void Tileset2DResource::RemoveTile(int tile_id) {
    m_tiles.erase(tile_id);
}

TileData* Tileset2DResource::GetTile(int tile_id) {
    auto it = m_tiles.find(tile_id);
    return (it != m_tiles.end()) ? &it->second : nullptr;
}

const TileData* Tileset2DResource::GetTile(int tile_id) const {
    auto it = m_tiles.find(tile_id);
    return (it != m_tiles.end()) ? &it->second : nullptr;
}

TileData* Tileset2DResource::GetTileAt(const glm::ivec2& grid_pos) {
    for (auto& pair : m_tiles) {
        if (pair.second.grid_position == grid_pos) {
            return &pair.second;
        }
    }
    return nullptr;
}

const TileData* Tileset2DResource::GetTileAt(const glm::ivec2& grid_pos) const {
    for (const auto& pair : m_tiles) {
        if (pair.second.grid_position == grid_pos) {
            return &pair.second;
        }
    }
    return nullptr;
}

std::vector<int> Tileset2DResource::GetTileIds() const {
    std::vector<int> ids;
    for (const auto& pair : m_tiles) {
        ids.push_back(pair.first);
    }
    return ids;
}

glm::vec4 Tileset2DResource::CalculateTextureRegion(const glm::ivec2& grid_pos) const {
    // Calculate pixel coordinates for a tile at the given grid position
    float x = static_cast<float>(m_margin + grid_pos.x * (m_tile_size.x + m_spacing));
    float y = static_cast<float>(m_margin + grid_pos.y * (m_tile_size.y + m_spacing));
    float w = static_cast<float>(m_tile_size.x);
    float h = static_cast<float>(m_tile_size.y);

    return glm::vec4(x, y, w, h);
}

glm::vec4 Tileset2DResource::CalculateNormalizedTextureRegion(const glm::ivec2& grid_pos, const glm::ivec2& texture_size) const {
    // Calculate normalized UV coordinates for a tile at the given grid position
    glm::vec4 pixel_region = CalculateTextureRegion(grid_pos);

    if (texture_size.x <= 0 || texture_size.y <= 0) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    float u1 = pixel_region.x / texture_size.x;
    float v1 = pixel_region.y / texture_size.y;
    float u2 = (pixel_region.x + pixel_region.z) / texture_size.x;
    float v2 = (pixel_region.y + pixel_region.w) / texture_size.y;

    // Ensure UV coordinates are within [0,1] range
    u1 = std::max(0.0f, std::min(1.0f, u1));
    v1 = std::max(0.0f, std::min(1.0f, v1));
    u2 = std::max(0.0f, std::min(1.0f, u2));
    v2 = std::max(0.0f, std::min(1.0f, v2));

    return glm::vec4(u1, v1, u2 - u1, v2 - v1);
}

glm::ivec2 Tileset2DResource::GetGridPositionFromTileId(int tile_id) const {
    if (m_grid_size.x <= 0) return glm::ivec2(0);
    
    int x = tile_id % m_grid_size.x;
    int y = tile_id / m_grid_size.x;
    return glm::ivec2(x, y);
}

int Tileset2DResource::GetTileIdFromGridPosition(const glm::ivec2& grid_pos) const {
    return grid_pos.y * m_grid_size.x + grid_pos.x;
}

void Tileset2DResource::GenerateTilesFromGrid() {
    m_tiles.clear();
    
    for (int y = 0; y < m_grid_size.y; ++y) {
        for (int x = 0; x < m_grid_size.x; ++x) {
            glm::ivec2 grid_pos(x, y);
            int tile_id = GetTileIdFromGridPosition(grid_pos);
            
            TileData tile(tile_id, grid_pos);
            tile.texture_region = CalculateTextureRegion(grid_pos);
            
            m_tiles[tile_id] = tile;
        }
    }
}

void Tileset2DResource::ClearTiles() {
    m_tiles.clear();
}

bool Tileset2DResource::SaveToFile(const std::string& filepath) const {
    try {
        std::string jsonData = ToJSON();
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filepath << std::endl;
            return false;
        }
        file << jsonData;
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving tileset resource: " << e.what() << std::endl;
        return false;
    }
}

bool Tileset2DResource::LoadFromFile(const std::string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) {
            std::cerr << "Tileset file not found: " << filepath << std::endl;
            return false;
        }
        
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << filepath << std::endl;
            return false;
        }
        
        std::string jsonData((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();
        
        return FromJSON(jsonData);
    } catch (const std::exception& e) {
        std::cerr << "Error loading tileset resource: " << e.what() << std::endl;
        return false;
    }
}

std::string Tileset2DResource::ToJSON() const {
    json j;
    j["type"] = "Tileset2D";
    j["version"] = "1.0";
    j["texture_path"] = m_texture_path;
    j["tile_size"] = {m_tile_size.x, m_tile_size.y};
    j["grid_size"] = {m_grid_size.x, m_grid_size.y};
    j["spacing"] = m_spacing;
    j["margin"] = m_margin;
    
    json tiles_json = json::array();
    for (const auto& pair : m_tiles) {
        tiles_json.push_back(SerializeTileData(pair.second));
    }
    j["tiles"] = tiles_json;
    
    return j.dump(4);
}

bool Tileset2DResource::FromJSON(const std::string& jsonData) {
    try {
        json j = json::parse(jsonData);
        
        if (j["type"] != "Tileset2D") {
            std::cerr << "Invalid tileset resource type" << std::endl;
            return false;
        }
        
        m_texture_path = j["texture_path"];
        m_tile_size = glm::ivec2(j["tile_size"][0], j["tile_size"][1]);
        m_grid_size = glm::ivec2(j["grid_size"][0], j["grid_size"][1]);
        m_spacing = j["spacing"];
        m_margin = j["margin"];
        
        m_tiles.clear();
        
        for (const auto& tile_json : j["tiles"]) {
            TileData tile;
            if (DeserializeTileData(tile_json, tile)) {
                m_tiles[tile.id] = tile;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing tileset JSON: " << e.what() << std::endl;
        return false;
    }
}

json Tileset2DResource::SerializeTileData(const TileData& tile) const {
    json j;
    j["id"] = tile.id;
    j["grid_position"] = {tile.grid_position.x, tile.grid_position.y};
    j["texture_region"] = {tile.texture_region.x, tile.texture_region.y, tile.texture_region.z, tile.texture_region.w};
    j["collision"] = SerializeCollisionShape(tile.collision);
    
    json custom_data_json = json::object();
    for (const auto& pair : tile.custom_data) {
        custom_data_json[pair.first] = SerializeTileDataValue(pair.second);
    }
    j["custom_data"] = custom_data_json;
    
    return j;
}

bool Tileset2DResource::DeserializeTileData(const json& j, TileData& tile) const {
    try {
        tile.id = j["id"];
        tile.grid_position = glm::ivec2(j["grid_position"][0], j["grid_position"][1]);
        tile.texture_region = glm::vec4(j["texture_region"][0], j["texture_region"][1], j["texture_region"][2], j["texture_region"][3]);
        
        if (!DeserializeCollisionShape(j["collision"], tile.collision)) {
            return false;
        }
        
        tile.custom_data.clear();
        for (const auto& pair : j["custom_data"].items()) {
            TileDataValue value;
            if (DeserializeTileDataValue(pair.value(), value)) {
                tile.custom_data[pair.key()] = value;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing tile data: " << e.what() << std::endl;
        return false;
    }
}

json Tileset2DResource::SerializeCollisionShape(const TileCollisionShape& shape) const {
    json j;
    j["type"] = static_cast<int>(shape.type);
    j["offset"] = {shape.offset.x, shape.offset.y};
    j["size"] = {shape.size.x, shape.size.y};

    json points_json = json::array();
    for (const auto& point : shape.points) {
        points_json.push_back({point.x, point.y});
    }
    j["points"] = points_json;

    return j;
}

bool Tileset2DResource::DeserializeCollisionShape(const json& j, TileCollisionShape& shape) const {
    try {
        shape.type = static_cast<TileCollisionType>(j["type"]);
        shape.offset = glm::vec2(j["offset"][0], j["offset"][1]);
        shape.size = glm::vec2(j["size"][0], j["size"][1]);

        shape.points.clear();
        for (const auto& point_json : j["points"]) {
            shape.points.push_back(glm::vec2(point_json[0], point_json[1]));
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing collision shape: " << e.what() << std::endl;
        return false;
    }
}

json Tileset2DResource::SerializeTileDataValue(const TileDataValue& value) const {
    json j;
    j["type"] = static_cast<int>(value.type);

    switch (value.type) {
        case TileDataType::String:
            j["value"] = value.string_value;
            break;
        case TileDataType::Integer:
            j["value"] = value.int_value;
            break;
        case TileDataType::Float:
            j["value"] = value.float_value;
            break;
        case TileDataType::Boolean:
            j["value"] = value.bool_value;
            break;
        case TileDataType::Color:
            j["value"] = {value.color_value.x, value.color_value.y, value.color_value.z, value.color_value.w};
            break;
    }

    return j;
}

bool Tileset2DResource::DeserializeTileDataValue(const json& j, TileDataValue& value) const {
    try {
        value.type = static_cast<TileDataType>(j["type"]);

        switch (value.type) {
            case TileDataType::String:
                value.string_value = j["value"];
                break;
            case TileDataType::Integer:
                value.int_value = j["value"];
                break;
            case TileDataType::Float:
                value.float_value = j["value"];
                break;
            case TileDataType::Boolean:
                value.bool_value = j["value"];
                break;
            case TileDataType::Color:
                value.color_value = glm::vec4(j["value"][0], j["value"][1], j["value"][2], j["value"][3]);
                break;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing tile data value: " << e.what() << std::endl;
        return false;
    }
}

} // namespace Lupine
