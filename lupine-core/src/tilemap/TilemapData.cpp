#include "lupine/tilemap/TilemapData.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace Lupine {

// TileMetadata implementation
json TileMetadata::ToJSON() const {
    json j;
    j["tags"] = tags;
    j["properties"] = properties;
    j["data"] = data;
    j["collision_enabled"] = collision_enabled;
    j["opacity"] = opacity;
    j["tint"] = {tint.x, tint.y, tint.z, tint.w};
    return j;
}

bool TileMetadata::FromJSON(const json& j) {
    try {
        tags = j.value("tags", std::map<std::string, std::string>());
        properties = j.value("properties", std::map<std::string, float>());
        data = j.value("data", std::map<std::string, std::string>());
        collision_enabled = j.value("collision_enabled", false);
        opacity = j.value("opacity", 1.0f);
        
        if (j.contains("tint") && j["tint"].is_array() && j["tint"].size() >= 4) {
            tint = glm::vec4(j["tint"][0], j["tint"][1], j["tint"][2], j["tint"][3]);
        } else {
            tint = glm::vec4(1.0f);
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing tile metadata: " << e.what() << std::endl;
        return false;
    }
}

// TileInstance implementation
json TileInstance::ToJSON() const {
    json j;
    j["tile_id"] = tile_id;
    j["tileset_id"] = tileset_id;
    j["metadata"] = metadata.ToJSON();
    return j;
}

bool TileInstance::FromJSON(const json& j) {
    try {
        tile_id = j.value("tile_id", -1);
        tileset_id = j.value("tileset_id", 0);
        
        if (j.contains("metadata")) {
            return metadata.FromJSON(j["metadata"]);
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing tile instance: " << e.what() << std::endl;
        return false;
    }
}

// TilemapLayer implementation
TilemapLayer::TilemapLayer(const std::string& name, const glm::ivec2& size)
    : m_name(name)
    , m_visible(true)
    , m_locked(false)
    , m_opacity(1.0f)
    , m_size(size)
{
    m_tiles.resize(size.x * size.y);
}

void TilemapLayer::Resize(const glm::ivec2& new_size) {
    std::vector<TileInstance> new_tiles(new_size.x * new_size.y);
    
    // Copy existing tiles
    for (int y = 0; y < std::min(m_size.y, new_size.y); ++y) {
        for (int x = 0; x < std::min(m_size.x, new_size.x); ++x) {
            int old_index = y * m_size.x + x;
            int new_index = y * new_size.x + x;
            new_tiles[new_index] = m_tiles[old_index];
        }
    }
    
    m_size = new_size;
    m_tiles = std::move(new_tiles);
}

const TileInstance& TilemapLayer::GetTile(int x, int y) const {
    static TileInstance empty_tile;
    if (!IsValidPosition(x, y)) {
        return empty_tile;
    }
    return m_tiles[GetIndex(x, y)];
}

void TilemapLayer::SetTile(int x, int y, const TileInstance& tile) {
    if (IsValidPosition(x, y)) {
        m_tiles[GetIndex(x, y)] = tile;
    }
}

void TilemapLayer::ClearTile(int x, int y) {
    if (IsValidPosition(x, y)) {
        m_tiles[GetIndex(x, y)] = TileInstance();
    }
}

void TilemapLayer::Clear() {
    std::fill(m_tiles.begin(), m_tiles.end(), TileInstance());
}

void TilemapLayer::Fill(const TileInstance& tile) {
    std::fill(m_tiles.begin(), m_tiles.end(), tile);
}

void TilemapLayer::FloodFill(int x, int y, const TileInstance& tile) {
    if (!IsValidPosition(x, y)) return;
    
    TileInstance original_tile = GetTile(x, y);
    if (original_tile.tile_id == tile.tile_id && original_tile.tileset_id == tile.tileset_id) {
        return; // Same tile, no need to fill
    }
    
    // Simple flood fill implementation
    std::vector<glm::ivec2> stack;
    stack.push_back(glm::ivec2(x, y));
    
    while (!stack.empty()) {
        glm::ivec2 pos = stack.back();
        stack.pop_back();
        
        if (!IsValidPosition(pos.x, pos.y)) continue;
        
        TileInstance current_tile = GetTile(pos.x, pos.y);
        if (current_tile.tile_id != original_tile.tile_id || 
            current_tile.tileset_id != original_tile.tileset_id) {
            continue;
        }
        
        SetTile(pos.x, pos.y, tile);
        
        // Add neighbors
        stack.push_back(glm::ivec2(pos.x + 1, pos.y));
        stack.push_back(glm::ivec2(pos.x - 1, pos.y));
        stack.push_back(glm::ivec2(pos.x, pos.y + 1));
        stack.push_back(glm::ivec2(pos.x, pos.y - 1));
    }
}

json TilemapLayer::ToJSON() const {
    json j;
    j["name"] = m_name;
    j["visible"] = m_visible;
    j["locked"] = m_locked;
    j["opacity"] = m_opacity;
    j["size"] = {m_size.x, m_size.y};
    
    json tiles_json = json::array();
    for (const auto& tile : m_tiles) {
        if (!tile.IsEmpty()) {
            tiles_json.push_back(tile.ToJSON());
        } else {
            tiles_json.push_back(nullptr); // Empty tile
        }
    }
    j["tiles"] = tiles_json;
    
    return j;
}

bool TilemapLayer::FromJSON(const json& j) {
    try {
        m_name = j.value("name", "Layer");
        m_visible = j.value("visible", true);
        m_locked = j.value("locked", false);
        m_opacity = j.value("opacity", 1.0f);
        
        if (j.contains("size") && j["size"].is_array() && j["size"].size() >= 2) {
            glm::ivec2 size(j["size"][0], j["size"][1]);
            m_size = size;
            m_tiles.resize(size.x * size.y);
        }
        
        if (j.contains("tiles") && j["tiles"].is_array()) {
            const auto& tiles_json = j["tiles"];
            for (size_t i = 0; i < tiles_json.size() && i < m_tiles.size(); ++i) {
                if (tiles_json[i].is_null()) {
                    m_tiles[i] = TileInstance(); // Empty tile
                } else {
                    m_tiles[i].FromJSON(tiles_json[i]);
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing tilemap layer: " << e.what() << std::endl;
        return false;
    }
}

bool TilemapLayer::IsValidPosition(int x, int y) const {
    return x >= 0 && x < m_size.x && y >= 0 && y < m_size.y;
}

// TilesetReference implementation
json TilesetReference::ToJSON() const {
    json j;
    j["id"] = id;
    j["name"] = name;
    j["path"] = path;
    j["tile_size"] = {tile_size.x, tile_size.y};
    j["grid_size"] = {grid_size.x, grid_size.y};
    j["spacing"] = spacing;
    j["margin"] = margin;
    return j;
}

bool TilesetReference::FromJSON(const json& j) {
    try {
        id = j.value("id", 0);
        name = j.value("name", "");
        path = j.value("path", "");
        
        if (j.contains("tile_size") && j["tile_size"].is_array() && j["tile_size"].size() >= 2) {
            tile_size = glm::ivec2(j["tile_size"][0], j["tile_size"][1]);
        }
        
        if (j.contains("grid_size") && j["grid_size"].is_array() && j["grid_size"].size() >= 2) {
            grid_size = glm::ivec2(j["grid_size"][0], j["grid_size"][1]);
        }
        
        spacing = j.value("spacing", 0);
        margin = j.value("margin", 0);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing tileset reference: " << e.what() << std::endl;
        return false;
    }
}

// TilemapProject implementation
TilemapProject::TilemapProject()
    : m_name("Untitled Tilemap")
    , m_size(20, 15)
    , m_tile_size(32, 32)
    , m_background_color(0.2f, 0.2f, 0.2f, 1.0f)
    , m_active_layer(0)
    , m_next_tileset_id(1)
{
    // Create default layer
    AddLayer("Background");
}

void TilemapProject::SetSize(const glm::ivec2& size) {
    m_size = size;
    ResizeAllLayers(size);
}

int TilemapProject::AddTileset(const TilesetReference& tileset) {
    TilesetReference new_tileset = tileset;
    new_tileset.id = m_next_tileset_id++;
    m_tilesets.push_back(new_tileset);
    return new_tileset.id;
}

void TilemapProject::RemoveTileset(int tileset_id) {
    auto it = std::find_if(m_tilesets.begin(), m_tilesets.end(),
        [tileset_id](const TilesetReference& ref) { return ref.id == tileset_id; });

    if (it != m_tilesets.end()) {
        m_tilesets.erase(it);

        // TODO: Remove all tiles using this tileset from all layers
        // This would require iterating through all layers and clearing tiles with this tileset_id
    }
}

const TilesetReference* TilemapProject::GetTileset(int tileset_id) const {
    auto it = std::find_if(m_tilesets.begin(), m_tilesets.end(),
        [tileset_id](const TilesetReference& ref) { return ref.id == tileset_id; });

    return (it != m_tilesets.end()) ? &(*it) : nullptr;
}

TilesetReference* TilemapProject::GetTileset(int tileset_id) {
    auto it = std::find_if(m_tilesets.begin(), m_tilesets.end(),
        [tileset_id](const TilesetReference& ref) { return ref.id == tileset_id; });

    return (it != m_tilesets.end()) ? &(*it) : nullptr;
}

int TilemapProject::AddLayer(const std::string& name) {
    auto layer = std::make_unique<TilemapLayer>(name, m_size);
    m_layers.push_back(std::move(layer));
    return static_cast<int>(m_layers.size() - 1);
}

void TilemapProject::RemoveLayer(int layer_index) {
    if (layer_index >= 0 && layer_index < static_cast<int>(m_layers.size())) {
        m_layers.erase(m_layers.begin() + layer_index);

        // Adjust active layer index
        if (m_active_layer >= layer_index && m_active_layer > 0) {
            m_active_layer--;
        }

        // Ensure we always have at least one layer
        if (m_layers.empty()) {
            AddLayer("Background");
            m_active_layer = 0;
        }
    }
}

void TilemapProject::MoveLayer(int from_index, int to_index) {
    if (from_index >= 0 && from_index < static_cast<int>(m_layers.size()) &&
        to_index >= 0 && to_index < static_cast<int>(m_layers.size()) &&
        from_index != to_index) {

        auto layer = std::move(m_layers[from_index]);
        m_layers.erase(m_layers.begin() + from_index);
        m_layers.insert(m_layers.begin() + to_index, std::move(layer));

        // Update active layer index
        if (m_active_layer == from_index) {
            m_active_layer = to_index;
        } else if (from_index < m_active_layer && to_index >= m_active_layer) {
            m_active_layer--;
        } else if (from_index > m_active_layer && to_index <= m_active_layer) {
            m_active_layer++;
        }
    }
}

TilemapLayer* TilemapProject::GetLayer(int layer_index) {
    if (layer_index >= 0 && layer_index < static_cast<int>(m_layers.size())) {
        return m_layers[layer_index].get();
    }
    return nullptr;
}

const TilemapLayer* TilemapProject::GetLayer(int layer_index) const {
    if (layer_index >= 0 && layer_index < static_cast<int>(m_layers.size())) {
        return m_layers[layer_index].get();
    }
    return nullptr;
}

TilemapLayer* TilemapProject::GetLayerByName(const std::string& name) {
    for (auto& layer : m_layers) {
        if (layer->GetName() == name) {
            return layer.get();
        }
    }
    return nullptr;
}

void TilemapProject::SetActiveLayerIndex(int index) {
    if (index >= 0 && index < static_cast<int>(m_layers.size())) {
        m_active_layer = index;
    }
}

TilemapLayer* TilemapProject::GetActiveLayer() {
    return GetLayer(m_active_layer);
}

void TilemapProject::Clear() {
    for (auto& layer : m_layers) {
        layer->Clear();
    }
}

bool TilemapProject::IsEmpty() const {
    for (const auto& layer : m_layers) {
        // Check if layer has any non-empty tiles
        for (int y = 0; y < m_size.y; ++y) {
            for (int x = 0; x < m_size.x; ++x) {
                if (!layer->GetTile(x, y).IsEmpty()) {
                    return false;
                }
            }
        }
    }
    return true;
}

void TilemapProject::ResizeAllLayers(const glm::ivec2& new_size) {
    for (auto& layer : m_layers) {
        layer->Resize(new_size);
    }
}

bool TilemapProject::SaveToFile(const std::string& filepath) const {
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
        std::cerr << "Error saving tilemap project: " << e.what() << std::endl;
        return false;
    }
}

bool TilemapProject::LoadFromFile(const std::string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) {
            std::cerr << "Tilemap file not found: " << filepath << std::endl;
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
        std::cerr << "Error loading tilemap project: " << e.what() << std::endl;
        return false;
    }
}

std::string TilemapProject::ToJSON() const {
    json j;
    j["type"] = "TilemapProject";
    j["version"] = "1.0";
    j["name"] = m_name;
    j["size"] = {m_size.x, m_size.y};
    j["tile_size"] = {m_tile_size.x, m_tile_size.y};
    j["background_color"] = {m_background_color.x, m_background_color.y, m_background_color.z, m_background_color.w};
    j["active_layer"] = m_active_layer;

    // Serialize tilesets
    json tilesets_json = json::array();
    for (const auto& tileset : m_tilesets) {
        tilesets_json.push_back(tileset.ToJSON());
    }
    j["tilesets"] = tilesets_json;

    // Serialize layers
    json layers_json = json::array();
    for (const auto& layer : m_layers) {
        layers_json.push_back(layer->ToJSON());
    }
    j["layers"] = layers_json;

    return j.dump(4);
}

bool TilemapProject::FromJSON(const std::string& json_data) {
    try {
        json j = json::parse(json_data);

        if (j["type"] != "TilemapProject") {
            std::cerr << "Invalid tilemap project type" << std::endl;
            return false;
        }

        m_name = j.value("name", "Untitled Tilemap");

        if (j.contains("size") && j["size"].is_array() && j["size"].size() >= 2) {
            m_size = glm::ivec2(j["size"][0], j["size"][1]);
        }

        if (j.contains("tile_size") && j["tile_size"].is_array() && j["tile_size"].size() >= 2) {
            m_tile_size = glm::ivec2(j["tile_size"][0], j["tile_size"][1]);
        }

        if (j.contains("background_color") && j["background_color"].is_array() && j["background_color"].size() >= 4) {
            m_background_color = glm::vec4(j["background_color"][0], j["background_color"][1],
                                         j["background_color"][2], j["background_color"][3]);
        }

        m_active_layer = j.value("active_layer", 0);

        // Load tilesets
        m_tilesets.clear();
        m_next_tileset_id = 1;
        if (j.contains("tilesets")) {
            for (const auto& tileset_json : j["tilesets"]) {
                TilesetReference tileset;
                if (tileset.FromJSON(tileset_json)) {
                    m_tilesets.push_back(tileset);
                    m_next_tileset_id = std::max(m_next_tileset_id, tileset.id + 1);
                }
            }
        }

        // Load layers
        m_layers.clear();
        if (j.contains("layers")) {
            for (const auto& layer_json : j["layers"]) {
                auto layer = std::make_unique<TilemapLayer>("Layer", m_size);
                if (layer->FromJSON(layer_json)) {
                    m_layers.push_back(std::move(layer));
                }
            }
        }

        // Ensure we have at least one layer
        if (m_layers.empty()) {
            AddLayer("Background");
            m_active_layer = 0;
        }

        // Validate active layer index
        if (m_active_layer >= static_cast<int>(m_layers.size())) {
            m_active_layer = 0;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing tilemap project JSON: " << e.what() << std::endl;
        return false;
    }
}

} // namespace Lupine
