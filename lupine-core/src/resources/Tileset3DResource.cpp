#include "lupine/resources/Tileset3DResource.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace Lupine {

// Tile3DData copy constructor
Tile3DData::Tile3DData(const Tile3DData& other)
    : id(other.id)
    , name(other.name)
    , mesh_path(other.mesh_path)
    , preview_image_path(other.preview_image_path)
    , default_transform(other.default_transform)
    , collision(other.collision)
    , custom_data(other.custom_data)
    , model(nullptr)  // Don't copy the model, it will be loaded when needed
    , model_loaded(false)
{
}

// Tile3DData assignment operator
Tile3DData& Tile3DData::operator=(const Tile3DData& other) {
    if (this != &other) {
        id = other.id;
        name = other.name;
        mesh_path = other.mesh_path;
        preview_image_path = other.preview_image_path;
        default_transform = other.default_transform;
        collision = other.collision;
        custom_data = other.custom_data;
        model.reset();  // Reset the model, it will be loaded when needed
        model_loaded = false;
    }
    return *this;
}

// Tileset3DResource implementation
void Tileset3DResource::AddTile(const Tile3DData& tile) {
    m_tiles[tile.id] = tile;
}

void Tileset3DResource::RemoveTile(int tile_id) {
    // Remove from all categories first
    for (auto& [category_name, category] : m_categories) {
        auto it = std::find(category.tile_ids.begin(), category.tile_ids.end(), tile_id);
        if (it != category.tile_ids.end()) {
            category.tile_ids.erase(it);
        }
    }
    
    // Remove the tile
    m_tiles.erase(tile_id);
}

Tile3DData* Tileset3DResource::GetTile(int tile_id) {
    auto it = m_tiles.find(tile_id);
    return (it != m_tiles.end()) ? &it->second : nullptr;
}

const Tile3DData* Tileset3DResource::GetTile(int tile_id) const {
    auto it = m_tiles.find(tile_id);
    return (it != m_tiles.end()) ? &it->second : nullptr;
}

Tile3DData* Tileset3DResource::GetTileByName(const std::string& name) {
    for (auto& pair : m_tiles) {
        if (pair.second.name == name) {
            return &pair.second;
        }
    }
    return nullptr;
}

const Tile3DData* Tileset3DResource::GetTileByName(const std::string& name) const {
    for (const auto& pair : m_tiles) {
        if (pair.second.name == name) {
            return &pair.second;
        }
    }
    return nullptr;
}

std::vector<int> Tileset3DResource::GetTileIds() const {
    std::vector<int> ids;
    for (const auto& pair : m_tiles) {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<std::string> Tileset3DResource::GetTileNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_tiles) {
        names.push_back(pair.second.name);
    }
    return names;
}

void Tileset3DResource::AddCategory(const Tile3DCategory& category) {
    m_categories[category.name] = category;
}

void Tileset3DResource::RemoveCategory(const std::string& name) {
    m_categories.erase(name);
}

Tile3DCategory* Tileset3DResource::GetCategory(const std::string& name) {
    auto it = m_categories.find(name);
    return (it != m_categories.end()) ? &it->second : nullptr;
}

const Tile3DCategory* Tileset3DResource::GetCategory(const std::string& name) const {
    auto it = m_categories.find(name);
    return (it != m_categories.end()) ? &it->second : nullptr;
}

void Tileset3DResource::AddTileToCategory(int tile_id, const std::string& category_name) {
    auto* category = GetCategory(category_name);
    if (category) {
        // Check if tile is already in category
        auto it = std::find(category->tile_ids.begin(), category->tile_ids.end(), tile_id);
        if (it == category->tile_ids.end()) {
            category->tile_ids.push_back(tile_id);
        }
    }
}

void Tileset3DResource::RemoveTileFromCategory(int tile_id, const std::string& category_name) {
    auto* category = GetCategory(category_name);
    if (category) {
        auto it = std::find(category->tile_ids.begin(), category->tile_ids.end(), tile_id);
        if (it != category->tile_ids.end()) {
            category->tile_ids.erase(it);
        }
    }
}

std::vector<std::string> Tileset3DResource::GetTileCategories(int tile_id) const {
    std::vector<std::string> categories;
    for (const auto& [category_name, category] : m_categories) {
        auto it = std::find(category.tile_ids.begin(), category.tile_ids.end(), tile_id);
        if (it != category.tile_ids.end()) {
            categories.push_back(category_name);
        }
    }
    return categories;
}

int Tileset3DResource::GetNextTileId() const {
    int max_id = -1;
    for (const auto& pair : m_tiles) {
        max_id = std::max(max_id, pair.first);
    }
    return max_id + 1;
}

bool Tileset3DResource::HasTile(int tile_id) const {
    return m_tiles.find(tile_id) != m_tiles.end();
}

bool Tileset3DResource::HasTileWithName(const std::string& name) const {
    return GetTileByName(name) != nullptr;
}

void Tileset3DResource::LoadTileModel(int tile_id) {
    Tile3DData* tile = GetTile(tile_id);
    if (!tile || tile->mesh_path.empty()) {
        return;
    }
    
    try {
        tile->model = MeshLoader::LoadModel(tile->mesh_path);
        if (tile->model && tile->model->IsLoaded()) {
            tile->model_loaded = true;
            std::cout << "Loaded 3D tile model: " << tile->mesh_path << std::endl;
        } else {
            std::cerr << "Failed to load 3D tile model: " << tile->mesh_path << std::endl;
            tile->model_loaded = false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception loading 3D tile model " << tile->mesh_path << ": " << e.what() << std::endl;
        tile->model.reset();
        tile->model_loaded = false;
    }
}

void Tileset3DResource::LoadAllTileModels() {
    for (auto& pair : m_tiles) {
        LoadTileModel(pair.first);
    }
}

void Tileset3DResource::UnloadTileModel(int tile_id) {
    Tile3DData* tile = GetTile(tile_id);
    if (tile) {
        tile->model.reset();
        tile->model_loaded = false;
    }
}

void Tileset3DResource::UnloadAllTileModels() {
    for (auto& pair : m_tiles) {
        pair.second.model.reset();
        pair.second.model_loaded = false;
    }
}

void Tileset3DResource::Clear() {
    m_tiles.clear();
    m_categories.clear();
    m_name.clear();
    m_description.clear();
    m_version = "1.0";
}

bool Tileset3DResource::SaveToFile(const std::string& filepath) const {
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
        std::cerr << "Error saving tileset3D resource: " << e.what() << std::endl;
        return false;
    }
}

bool Tileset3DResource::LoadFromFile(const std::string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) {
            std::cerr << "Tileset3D file not found: " << filepath << std::endl;
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
        std::cerr << "Error loading tileset3D resource: " << e.what() << std::endl;
        return false;
    }
}

std::string Tileset3DResource::ToJSON() const {
    json j;
    j["type"] = "Tileset3D";
    j["version"] = m_version;
    j["name"] = m_name;
    j["description"] = m_description;
    
    json tiles_json = json::array();
    for (const auto& pair : m_tiles) {
        tiles_json.push_back(SerializeTile3DData(pair.second));
    }
    j["tiles"] = tiles_json;
    
    json categories_json = json::array();
    for (const auto& pair : m_categories) {
        categories_json.push_back(SerializeTile3DCategory(pair.second));
    }
    j["categories"] = categories_json;
    
    return j.dump(4);
}

bool Tileset3DResource::FromJSON(const std::string& jsonData) {
    try {
        json j = json::parse(jsonData);
        
        if (j["type"] != "Tileset3D") {
            std::cerr << "Invalid tileset3D resource type" << std::endl;
            return false;
        }
        
        m_version = j.value("version", "1.0");
        m_name = j.value("name", "");
        m_description = j.value("description", "");
        
        m_tiles.clear();
        for (const auto& tile_json : j["tiles"]) {
            Tile3DData tile;
            if (DeserializeTile3DData(tile_json, tile)) {
                m_tiles[tile.id] = std::move(tile);
            }
        }
        
        m_categories.clear();
        if (j.contains("categories")) {
            for (const auto& category_json : j["categories"]) {
                Tile3DCategory category;
                if (DeserializeTile3DCategory(category_json, category)) {
                    m_categories[category.name] = std::move(category);
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing tileset3D JSON: " << e.what() << std::endl;
        return false;
    }
}

json Tileset3DResource::SerializeTile3DData(const Tile3DData& tile) const {
    json j;
    j["id"] = tile.id;
    j["name"] = tile.name;
    j["mesh_path"] = tile.mesh_path;
    j["preview_image_path"] = tile.preview_image_path;
    j["default_transform"] = SerializeTile3DTransform(tile.default_transform);
    j["collision"] = SerializeTile3DCollisionShape(tile.collision);

    json custom_data_json = json::object();
    for (const auto& pair : tile.custom_data) {
        custom_data_json[pair.first] = SerializeTile3DDataValue(pair.second);
    }
    j["custom_data"] = custom_data_json;

    return j;
}

bool Tileset3DResource::DeserializeTile3DData(const json& j, Tile3DData& tile) const {
    try {
        tile.id = j["id"];
        tile.name = j["name"];
        tile.mesh_path = j["mesh_path"];
        tile.preview_image_path = j.value("preview_image_path", "");

        if (!DeserializeTile3DTransform(j["default_transform"], tile.default_transform)) {
            return false;
        }

        if (!DeserializeTile3DCollisionShape(j["collision"], tile.collision)) {
            return false;
        }

        tile.custom_data.clear();
        if (j.contains("custom_data")) {
            for (const auto& pair : j["custom_data"].items()) {
                Tile3DDataValue value;
                if (DeserializeTile3DDataValue(pair.value(), value)) {
                    tile.custom_data[pair.key()] = value;
                }
            }
        }

        // Runtime data is not deserialized
        tile.model.reset();
        tile.model_loaded = false;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing tile3D data: " << e.what() << std::endl;
        return false;
    }
}

json Tileset3DResource::SerializeTile3DCollisionShape(const Tile3DCollisionShape& shape) const {
    json j;
    j["type"] = static_cast<int>(shape.type);
    j["offset"] = {shape.offset.x, shape.offset.y, shape.offset.z};
    j["size"] = {shape.size.x, shape.size.y, shape.size.z};
    j["collision_mesh_path"] = shape.collision_mesh_path;
    j["margin"] = shape.margin;

    return j;
}

bool Tileset3DResource::DeserializeTile3DCollisionShape(const json& j, Tile3DCollisionShape& shape) const {
    try {
        shape.type = static_cast<Tile3DCollisionType>(j["type"]);
        shape.offset = glm::vec3(j["offset"][0], j["offset"][1], j["offset"][2]);
        shape.size = glm::vec3(j["size"][0], j["size"][1], j["size"][2]);
        shape.collision_mesh_path = j.value("collision_mesh_path", "");
        shape.margin = j.value("margin", 0.0f);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing collision shape: " << e.what() << std::endl;
        return false;
    }
}

json Tileset3DResource::SerializeTile3DTransform(const Tile3DTransform& transform) const {
    json j;
    j["position"] = {transform.position.x, transform.position.y, transform.position.z};
    j["rotation"] = {transform.rotation.w, transform.rotation.x, transform.rotation.y, transform.rotation.z};
    j["scale"] = {transform.scale.x, transform.scale.y, transform.scale.z};

    return j;
}

bool Tileset3DResource::DeserializeTile3DTransform(const json& j, Tile3DTransform& transform) const {
    try {
        transform.position = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
        transform.rotation = glm::quat(j["rotation"][0], j["rotation"][1], j["rotation"][2], j["rotation"][3]);
        transform.scale = glm::vec3(j["scale"][0], j["scale"][1], j["scale"][2]);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing transform: " << e.what() << std::endl;
        return false;
    }
}

json Tileset3DResource::SerializeTile3DDataValue(const Tile3DDataValue& value) const {
    json j;
    j["type"] = static_cast<int>(value.type);

    switch (value.type) {
        case Tile3DDataType::String:
            j["value"] = value.string_value;
            break;
        case Tile3DDataType::Integer:
            j["value"] = value.int_value;
            break;
        case Tile3DDataType::Float:
            j["value"] = value.float_value;
            break;
        case Tile3DDataType::Boolean:
            j["value"] = value.bool_value;
            break;
        case Tile3DDataType::Vector3:
            j["value"] = {value.vec3_value.x, value.vec3_value.y, value.vec3_value.z};
            break;
        case Tile3DDataType::Color:
            j["value"] = {value.color_value.x, value.color_value.y, value.color_value.z, value.color_value.w};
            break;
    }

    return j;
}

bool Tileset3DResource::DeserializeTile3DDataValue(const json& j, Tile3DDataValue& value) const {
    try {
        value.type = static_cast<Tile3DDataType>(j["type"]);

        switch (value.type) {
            case Tile3DDataType::String:
                value.string_value = j["value"];
                break;
            case Tile3DDataType::Integer:
                value.int_value = j["value"];
                break;
            case Tile3DDataType::Float:
                value.float_value = j["value"];
                break;
            case Tile3DDataType::Boolean:
                value.bool_value = j["value"];
                break;
            case Tile3DDataType::Vector3:
                value.vec3_value = glm::vec3(j["value"][0], j["value"][1], j["value"][2]);
                break;
            case Tile3DDataType::Color:
                value.color_value = glm::vec4(j["value"][0], j["value"][1], j["value"][2], j["value"][3]);
                break;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing tile3D data value: " << e.what() << std::endl;
        return false;
    }
}

json Tileset3DResource::SerializeTile3DCategory(const Tile3DCategory& category) const {
    json j;
    j["name"] = category.name;
    j["description"] = category.description;
    j["tile_ids"] = category.tile_ids;

    return j;
}

bool Tileset3DResource::DeserializeTile3DCategory(const json& j, Tile3DCategory& category) const {
    try {
        category.name = j["name"];
        category.description = j.value("description", "");
        category.tile_ids = j["tile_ids"].get<std::vector<int>>();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing tile3D category: " << e.what() << std::endl;
        return false;
    }
}

} // namespace Lupine
