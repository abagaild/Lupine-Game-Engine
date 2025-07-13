#pragma once

#include "lupine/core/Scene.h"
#include "lupine/serialization/JsonUtils.h"
#include <string>
#include <memory>

namespace Lupine {

class Node;
class Component;

/**
 * @brief Scene serialization and deserialization
 */
class SceneSerializer {
public:
    /**
     * @brief Serialize scene to JSON file
     * @param scene Scene to serialize
     * @param file_path Output file path
     * @return True if successful
     */
    static bool SerializeToFile(const Scene* scene, const std::string& file_path);

    /**
     * @brief Deserialize scene from JSON file
     * @param file_path Input file path
     * @return Loaded scene or nullptr if failed
     */
    static std::unique_ptr<Scene> DeserializeFromFile(const std::string& file_path);

    /**
     * @brief Serialize scene to JSON string
     * @param scene Scene to serialize
     * @return JSON string
     */
    static std::string SerializeToString(const Scene* scene);

    /**
     * @brief Deserialize scene from JSON string
     * @param json_string JSON string
     * @return Loaded scene or nullptr if failed
     */
    static std::unique_ptr<Scene> DeserializeFromString(const std::string& json_string);

    /**
     * @brief Create node by type name
     * @param type_name Node type name
     * @param name Node name
     * @return Created node
     */
    static std::unique_ptr<Node> CreateNodeByType(const std::string& type_name, const std::string& name);

private:
    // Helper functions for JSON serialization
    static JsonNode SerializeNode(const Node* node);
    static JsonNode SerializeComponent(const Component* component);

    // Helper functions for JSON deserialization
    static std::unique_ptr<Node> DeserializeNode(const JsonNode& nodeData);
};

} // namespace Lupine
